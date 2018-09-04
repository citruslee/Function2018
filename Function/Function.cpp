#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <stdio.h>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "D3DCompiler.lib")
#pragma comment (lib, "dxguid.lib")
#pragma comment (lib, "Mmdevapi.lib")
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
#define MINI_AL_IMPLEMENTATION
#include "mini_al.h"

#include "Audio.hpp"

#define DEVELOPMENT
#define D3D_COMPILE_STANDARD_FILE_INCLUDE ((ID3DInclude*)(UINT_PTR)1)

#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 720

IDXGISwapChain *swapchain;          
ID3D11Device *dev;                  
ID3D11DeviceContext *devcon;        
ID3D11RenderTargetView *backbuffer; 
ID3D11VertexShader *pVS;            
ID3D11PixelShader *pPS;
ID3D11ShaderReflection * pShaderReflection = NULL;
D3D11_VIEWPORT viewport;
ID3D11Buffer* cbPerObjectBuffer;

bool keyState[256];
bool keyPressed[256];

void InitD3D(HWND hWnd);
void RenderFrame(void); 
void CleanD3D(void);    
#if defined(DEVELOPMENT)
char szError[4096];
#endif

struct SConstantBuffer
{
	float fTime;
	float temp[3];
};

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

bool ReloadShader(ID3D11PixelShader** PixelShader, ID3D11ShaderReflectionConstantBuffer** pCBuf, const char * szShaderFile, char* szErrorBuffer, int nErrorBufferSize)
{
	ID3DBlob * pCode = NULL;
	ID3DBlob * pErrors = NULL;
	auto returncode = D3DCompileFromFile(L"PixelShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &pCode, &pErrors);
	if (returncode != S_OK && pErrors != NULL)
	{
		memset(szErrorBuffer, 0, nErrorBufferSize);
		strncpy(szErrorBuffer, (const char*)pErrors->GetBufferPointer(), nErrorBufferSize - 1);
		return false;
	}

	if (*PixelShader)
	{
		(*PixelShader)->Release();
		(*PixelShader) = NULL;
	}

	if (dev->CreatePixelShader(pCode->GetBufferPointer(), pCode->GetBufferSize(), NULL, &*PixelShader) != S_OK)
	{
		return false;
	}
	//D3DReflect(pCode->GetBufferPointer(), pCode->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&pShaderReflection);
	//*pCBuf = pShaderReflection->GetConstantBufferByIndex(0);

	return true;
}

FAudioPlayer audioPlayer;

//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
int main()
{
	HWND hWnd;
	WNDCLASSEX wc = WNDCLASSEX();
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = L"WindowClass";
	RegisterClassEx(&wc);

	DWORD wStyle = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	wStyle |= WS_OVERLAPPED | WS_CAPTION;
	RECT wr = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
	AdjustWindowRect(&wr, wStyle, FALSE);
	hWnd = CreateWindowExW(NULL, wc.lpszClassName, L"Punci", wStyle, 300, 300, wr.right - wr.left, wr.bottom - wr.top, NULL, NULL, wc.hInstance, NULL);
	ShowWindow(hWnd, SW_SHOW);
	InitD3D(hWnd);
	MSG msg;

	audioPlayer.Initialize("selfishcrab.wav");
	audioPlayer.Start();

	ID3D11Buffer*   g_pConstantBuffer11 = NULL;

	

	// Supply the vertex shader constant data.
	SConstantBuffer VsConstData;
	VsConstData.fTime = 0.0f;

	D3D11_BUFFER_DESC cbbd;
	ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));

	cbbd.Usage = D3D11_USAGE_DEFAULT;
	cbbd.ByteWidth = sizeof(SConstantBuffer);
	cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbbd.CPUAccessFlags = 0;
	cbbd.MiscFlags = 0;

	auto hr = dev->CreateBuffer(&cbbd, NULL, &cbPerObjectBuffer);
	
	while (TRUE)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
			{
				break;
			}
			
		}
#if defined(DEVELOPMENT)
		if (GetAsyncKeyState(VK_CONTROL) & 0x8000 && GetAsyncKeyState(0x53) & 0x8000)
		{
			if (ReloadShader(&pPS, nullptr , "PixelShader.hlsl", szError, 4069))
			{
				printf("Last shader works fine.\n");
			}
			else 
			{
				printf("Shader error:\n%s\n", szError);
			}
		}

		if (keyState[VK_LEFT])
		{
			audioPlayer.Seek(audioPlayer.GetCurrentFrame() - 20000);
		}
		else if (keyState[VK_RIGHT])
		{
			audioPlayer.Seek(audioPlayer.GetCurrentFrame() + 20000);
		}
		else if (keyPressed[VK_SPACE])
		{
			if (audioPlayer.IsPlaying())
			{
				audioPlayer.Stop();
			}
			else
			{
				audioPlayer.Start();
			}
		}

		for (int i = 0; i < 256; i++)
		{
			keyPressed[i] = false;
		}
#endif
		if (GetKeyState(VK_ESCAPE) & 0x8000)
		{
			break;
		}
		VsConstData.fTime = audioPlayer.GetTime();
		devcon->UpdateSubresource(cbPerObjectBuffer, 0, nullptr, &VsConstData, 0, 0);
		devcon->PSSetConstantBuffers(0, 1, &cbPerObjectBuffer);
		RenderFrame();
	}
	audioPlayer.Stop();
	audioPlayer.Cleanup();
	CleanD3D();
	return msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}
	case WM_KEYUP:
	{
		if (wParam < 256)
		{
			keyPressed[wParam] = keyState[wParam] = false;
		}
		break;
	}
	case WM_KEYDOWN:
	{
		if (wParam < 256 && !(lParam & 0x40000000))
		{
			keyPressed[wParam] = keyState[wParam] = true;
		}

		if (wParam < 256)
		{
			keyState[wParam] = true;
		}

		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
			return 0;
		}
		break;
	}
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

void InitD3D(HWND hWnd)
{
	DXGI_SWAP_CHAIN_DESC scd = DXGI_SWAP_CHAIN_DESC();
	scd.BufferCount = 1;                                   
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;    
	scd.BufferDesc.Width = SCREEN_WIDTH;                   
	scd.BufferDesc.Height = SCREEN_HEIGHT;                 
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;     
	scd.OutputWindow = hWnd;                               
	scd.SampleDesc.Count = 4;                              
	scd.Windowed = TRUE;                                   
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;    
	D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, NULL, NULL, D3D11_SDK_VERSION, &scd, &swapchain, &dev, NULL, &devcon);
	ID3D11Texture2D *pBackBuffer;
	swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	dev->CreateRenderTargetView(pBackBuffer, NULL, &backbuffer);
	pBackBuffer->Release();

	viewport = D3D11_VIEWPORT();
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = SCREEN_WIDTH;
	viewport.Height = SCREEN_HEIGHT;
	
	ID3DBlob *VS;
	D3DCompileFromFile(L"VertexShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &VS, nullptr);
	ReloadShader(&pPS, nullptr, "PixelShader.hlsl", szError, 4069);
	

	dev->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &pVS);
}

void RenderFrame(void)
{
	float clearcolour[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
	devcon->ClearRenderTargetView(backbuffer, clearcolour);
	devcon->OMSetRenderTargets(1, &backbuffer, NULL);
	devcon->RSSetViewports(1, &viewport);
	devcon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	devcon->VSSetShader(pVS, 0, 0);
	devcon->PSSetShader(pPS, 0, 0);
	devcon->Draw(3, 0);
	swapchain->Present(1, 0);
}

void CleanD3D(void)
{
	swapchain->SetFullscreenState(FALSE, NULL);
	pVS->Release();
	pPS->Release();
	swapchain->Release();
	backbuffer->Release();
	dev->Release();
	devcon->Release();
}
