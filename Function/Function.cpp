#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "D3DCompiler.lib")

#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 720

IDXGISwapChain *swapchain;          
ID3D11Device *dev;                  
ID3D11DeviceContext *devcon;        
ID3D11RenderTargetView *backbuffer; 
ID3D11VertexShader *pVS;            
ID3D11PixelShader *pPS;
D3D11_VIEWPORT viewport;
            	
void InitD3D(HWND hWnd);
void RenderFrame(void); 
void CleanD3D(void);    

ID3D11ShaderReflectionConstantBuffer* constantBufferReflection;
ID3D11Buffer* constantBuffer;
unsigned char constantBufferData[512];

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


//ID3D11Buffer * pFullscreenQuadConstantBuffer = NULL;

void __UpdateConstants(ID3D11ShaderReflectionConstantBuffer* pCBuf, ID3D11Buffer* cbuffer, unsigned char* pFullscreenQuadConstants)
{
	D3D11_MAPPED_SUBRESOURCE subRes;
	devcon->Map(cbuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &subRes);
	CopyMemory(subRes.pData, &pFullscreenQuadConstants, sizeof(pFullscreenQuadConstants));
	devcon->Unmap(cbuffer, NULL);
}

void SetShaderConstant(ID3D11ShaderReflectionConstantBuffer* pCBuf, ID3D11Buffer* cbuffer, unsigned char* data, const char * szConstName, float x)
{
	ID3D11ShaderReflectionVariable * pCVar = pCBuf->GetVariableByName(szConstName);
	D3D11_SHADER_VARIABLE_DESC pDesc;
	if (pCVar->GetDesc(&pDesc) != S_OK)
		return;
	((float*)(((unsigned char*)&data) + pDesc.StartOffset))[0] = x;

	__UpdateConstants(pCBuf, cbuffer, data);
}

void SetShaderConstant(ID3D11ShaderReflectionConstantBuffer* pCBuf, ID3D11Buffer* cbuffer, unsigned char* data, const char * szConstName, float x, float y)
{
	ID3D11ShaderReflectionVariable * pCVar = pCBuf->GetVariableByName(szConstName);
	D3D11_SHADER_VARIABLE_DESC pDesc;
	if (pCVar->GetDesc(&pDesc) != S_OK)
		return;

	((float*)(((unsigned char*)&data) + pDesc.StartOffset))[0] = x;
	((float*)(((unsigned char*)&data) + pDesc.StartOffset))[1] = y;

	__UpdateConstants(pCBuf, cbuffer, data);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND hWnd;
	WNDCLASSEX wc = WNDCLASSEX();
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = L"WindowClass";
	RegisterClassEx(&wc);
	RECT wr = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
	hWnd = CreateWindowEx(NULL, wc.lpszClassName, L"Punci", WS_OVERLAPPEDWINDOW, 300, 300, wr.right - wr.left, wr.bottom - wr.top, NULL, NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);
	InitD3D(hWnd);
	SetShaderConstant(constantBufferReflection, constantBuffer, constantBufferData, "Resolution", SCREEN_WIDTH, SCREEN_HEIGHT);
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
	} break;
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
	D3DCompileFromFile(L"VertexShader.hlsl", nullptr, nullptr, "main", "vs_5_0", 0, 0, &VS, nullptr);
	D3DCompileFromFile(L"PixelShader.hlsl", nullptr, nullptr, "main", "ps_5_0", 0, 0, &PS, nullptr);

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
