#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <stdio.h>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "D3DCompiler.lib")
#pragma comment (lib, "dxguid.lib")

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
            	
void InitD3D(HWND hWnd);
void RenderFrame(void); 
void CleanD3D(void);    
#if defined(DEVELOPMENT)
char szError[4096];
#endif
ID3D11ShaderReflectionConstantBuffer* constantBufferReflection;
ID3D11Buffer* constantBuffer;
unsigned char constantBufferData[512];

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

bool ReloadShader(ID3D11PixelShader** PixelShader, ID3D11ShaderReflectionConstantBuffer** pCBuf, const char * szShaderFile, char* szErrorBuffer, int nErrorBufferSize)
{
	ID3DBlob * pCode = NULL;
	ID3DBlob * pErrors = NULL;
	if (D3DCompileFromFile(L"PixelShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", 0, 0, &pCode, &pErrors) != S_OK)
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
	D3DReflect(pCode->GetBufferPointer(), pCode->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&pShaderReflection);
	*pCBuf = pShaderReflection->GetConstantBufferByIndex(0);

	return true;
}

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
	//SetShaderConstant(constantBufferReflection, constantBuffer, constantBufferData, "Resolution", SCREEN_WIDTH, SCREEN_HEIGHT);
	MSG msg;
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
			if (ReloadShader(&pPS, &constantBufferReflection, "PixelShader.hlsl", szError, 4069))
			{
				printf("Last shader works fine.\n");
			}
			else 
			{
				printf("Shader error:\n%s\n", szError);
			}
		}
#endif
		if (GetKeyState(VK_ESCAPE) & 0x8000)
		{
			break;
		}
		RenderFrame();
	}
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
	case WM_KEYDOWN:
	{
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
	
	ID3DBlob *VS, *PS;
	D3DCompileFromFile(L"VertexShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0", 0, 0, &VS, nullptr);
	D3DCompileFromFile(L"PixelShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", 0, 0, &PS, nullptr);

	dev->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &pVS);
	dev->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &pPS);
}

void RenderFrame(void)
{
	float clearcolour[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
	devcon->ClearRenderTargetView(backbuffer, clearcolour);
	devcon->OMSetRenderTargets(1, &backbuffer, NULL);
	devcon->RSSetViewports(1, &viewport);
	devcon->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	devcon->VSSetShader(pVS, 0, 0);
	devcon->PSSetShader(pPS, 0, 0);
	devcon->Draw(3, 0);
	swapchain->Present(0, 0);
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
