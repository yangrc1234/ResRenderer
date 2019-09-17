#include <ResRenderer.hpp>
#include <map>
#include <iostream>
#include <ctime>
#include <ResRendererImpl_DX.hpp>
#include <wrl/client.h>
#include <dxgi1_3.h>
#include <queue>

#pragma comment (lib, "d3d11.lib")

using namespace Microsoft::WRL;

namespace ResRenderer {

	struct DxFrameBuffer
	{
		ComPtr<ID3D11RenderTargetView> rtview;
		ComPtr<ID3D11DepthStencilView> depth;
	};

	struct WindowProcMsg
	{
		HWND hWnd;
		UINT uMsg;
		WPARAM wParam;
		LPARAM lParam;
	};

	class WindowImpl {
	public:
		HWND handle = NULL;
		WindowResizeCallback callback = nullptr;
		bool isClosing = false;
		ComPtr<IDXGISwapChain> swapChain = NULL;
		DxFrameBuffer backbufferFrame;

		WindowImpl(HWND _handle, ComPtr<IDXGIFactory> factory, ComPtr<IDXGIDevice> device, ComPtr<ID3D11Device> d3ddevice)
		{
			maps.insert(std::pair<HWND, WindowImpl*>(handle, this));
			handle = _handle;

			DXGI_SWAP_CHAIN_DESC desc;
			ZeroMemory(&desc, sizeof(DXGI_SWAP_CHAIN_DESC));
			desc.Windowed = TRUE; // Sets the initial state of full-screen mode.
			desc.BufferCount = 2;
			desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			desc.SampleDesc.Count = 1;      //multisampling setting
			desc.SampleDesc.Quality = 0;    //vendor-specific flag
			desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
			desc.OutputWindow = handle;

			CHECKED(factory->CreateSwapChain(
				device.Get(),
				&desc,
				&swapChain
			));

			ComPtr<ID3D11Texture2D> backbufferTexture;
			swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &backbufferTexture);
			d3ddevice->CreateRenderTargetView(backbufferTexture.Get(), NULL, &backbufferFrame.rtview);
		}

		~WindowImpl()
		{
			maps.erase(handle);
			DestroyWindow(handle);
		}

		static std::map<HWND, WindowImpl*> maps;

		static WindowImpl* GetWindow(HWND hwnd) {
			auto t = WindowImpl::maps.find(hwnd);
			if (t != maps.end()) {
				return t->second;
			}
			return nullptr;
		}
	};
	std::map<HWND, WindowImpl*> WindowImpl::maps;

	
	LRESULT CALLBACK StaticWindowProc(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam
	);

	class D3dApp {
	public:
		bool Init() {
			m_hInstance = (HINSTANCE)GetModuleHandle(NULL);
			time(&startTime);
			// Register the windows class
			WNDCLASS wndClass;
			wndClass.style = CS_DBLCLKS;
			wndClass.lpfnWndProc = StaticWindowProc;
			wndClass.cbClsExtra = 0;
			wndClass.cbWndExtra = 0;
			wndClass.hInstance = m_hInstance;
			wndClass.hIcon = NULL;
			wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
			wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
			wndClass.lpszMenuName = NULL;
			wndClass.lpszClassName = WindowClassName;

			if (!RegisterClass(&wndClass))
			{
				DWORD dwError = GetLastError();
				if (dwError != ERROR_CLASS_ALREADY_EXISTS)
					return false;
			}

			D3D_FEATURE_LEVEL featureLevels[] =
			{
				D3D_FEATURE_LEVEL_11_1,
				D3D_FEATURE_LEVEL_11_0,
				D3D_FEATURE_LEVEL_10_1,
				D3D_FEATURE_LEVEL_10_0,
				D3D_FEATURE_LEVEL_9_3,
				D3D_FEATURE_LEVEL_9_2,
				D3D_FEATURE_LEVEL_9_1
			};

			// This flag adds support for surfaces with a color-channel ordering different
			// from the API default. It is required for compatibility with Direct2D.
			UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(DEBUG) || defined(_DEBUG)
			deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

			try
			{
				CHECKED(D3D11CreateDevice(
					nullptr,                    // specify null to use the default adapter
					D3D_DRIVER_TYPE_HARDWARE,
					0,                          // leave as 0 unless software device
					deviceFlags,              // optionally set debug and Direct2D compatibility flags
					featureLevels,              // list of feature levels this app can support
					sizeof(featureLevels) / sizeof(D3D_FEATURE_LEVEL),   // number of entries in above list
					D3D11_SDK_VERSION,          // always set this to D3D11_SDK_VERSION for Windows Store apps
					&device,                    // returns the Direct3D device created
					&m_featureLevel,            // returns feature level of device created
					&context                    // returns the device immediate context
				));
				CHECKED(device.As(&dxgiDevice));
				CHECKED(dxgiDevice->GetAdapter(&adapter));
				CHECKED(adapter->GetParent(IID_PPV_ARGS(&factory)));
			}
			catch (HRESULT)
			{
				return false;
			}
			return true;
		}

		float GetTime() {
			time_t now;
			time(&now);
			return static_cast<float>(difftime(now, startTime));
		}

		ErrorCode CreateRWindow(const char* windowNmae, int width, int height, WindowImpl** outWindow) {
			try
			{
				auto hwnd = CreateWindow(
					WindowClassName,
					windowNmae,
					WS_OVERLAPPEDWINDOW,
					0, 0,
					width, height,
					0,
					0,
					m_hInstance,
					0
				);
				ShowWindow(hwnd, 5);
				auto result = new WindowImpl(hwnd, factory, dxgiDevice, device);
				*outWindow = result;
				if (currentWindow == nullptr) {
					SetWindow(result);
				}
				return ErrorCode::RES_NO_ERROR;
			}
			catch (HRESULT)
			{
				return ErrorCode::INTERNAL_ERROR;
			}
		}

		void SetWindow(WindowImpl* window) {
			this->renderTarget = window->backbufferFrame;
			this->currentWindow = window;
		}

		~D3dApp()
		{
			UnregisterClass(WindowClassName, m_hInstance);
		}

		// Create the DX11 API device object, and get a corresponding context.
		ComPtr<ID3D11Device> device;
		ComPtr<IDXGIDevice> dxgiDevice;
		ComPtr<IDXGIAdapter> adapter;
		ComPtr<IDXGIFactory> factory;
		WindowImpl* currentWindow;
		DxFrameBuffer renderTarget;
		ComPtr<ID3D11DeviceContext> context;
		HINSTANCE m_hInstance = NULL;
		LPCSTR WindowClassName = TEXT("Main Window");
		time_t startTime = 0;
		D3D_FEATURE_LEVEL m_featureLevel;
		std::queue<WindowProcMsg> msgQueue;
	};

	static D3dApp *app = nullptr;

	bool RES_RENDERER_API Init() {
		if (app != nullptr) {
			return true; 
		}
		app = new D3dApp();
		if (!app->Init()) {
			app = nullptr;
			return false;
		}
		return true;
	}

	LRESULT CALLBACK StaticWindowProc(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam
	) {
		static_cast<LPARAM>(wParam);
		static_cast<LPARAM>(lParam);
		auto pWimpl = WindowImpl::GetWindow(hWnd);
		if (pWimpl != nullptr) {
			switch (uMsg)
			{
			case WM_SIZE:
			{
				RECT clipRect;
				GetClientRect(hWnd, &clipRect);
				if (pWimpl->callback != nullptr)
					pWimpl->callback(clipRect.right - clipRect.left, clipRect.top - clipRect.bottom);
				return 0;
			}
			case WM_QUIT:
			{
				pWimpl->isClosing = true;
				return 0;
			}
			default:
				break;
			}
		}
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	float RES_RENDERER_API GetTime() {
		return app->GetTime();
	}

	void RES_RENDERER_API Terminate() {
		delete app;
	}

	ErrorCode RES_RENDERER_API CreateResWindow(int width, int height, const char* title, Window* outWindow) {
		WindowImpl* result;
		app->CreateRWindow(title, width, height, &result);
		*outWindow = static_cast<Window>(result);
		return ErrorCode::RES_NO_ERROR;
	}

	void RES_RENDERER_API RegisterWindowResizeCallback(Window window, WindowResizeCallback callback) {
		auto pWindow = static_cast<WindowImpl*>(window);
		pWindow->callback = callback;
	}

	bool RES_RENDERER_API ShouldCloseWindow(Window window) {
		auto pWindow = static_cast<WindowImpl*>(window);
		return pWindow->isClosing;
	}

	void RES_RENDERER_API SwapBuffer(Window window) {
		auto pWindow = static_cast<WindowImpl*>(window);
		pWindow->swapChain->Present(0, 0);
	}

	void RES_RENDERER_API PollEvents() {
		MSG msg;
		// wait for the next message in the queue, store the result in 'msg'
		while (GetMessage(&msg, NULL, 0, 0))
		{
			// translate keystroke messages into the right format
			TranslateMessage(&msg);

			// send the message to the WindowProc function
			DispatchMessage(&msg);
		}

		// return this part of the WM_QUIT message to Windows
		return;
	}
	 
	void RES_RENDERER_API Clear(Color color, ClearType clearType) {
		auto CurrentRt = app->renderTarget;
		if (clearType == ClearType::Color || clearType == ClearType::ColorAndDepth) {
			if (CurrentRt.rtview != nullptr)
				app->context->ClearRenderTargetView(CurrentRt.rtview.Get(), static_cast<float*>(&color.r));
		}

		if (clearType == ClearType::Depth || clearType == ClearType::ColorAndDepth) {
			if (CurrentRt.depth != nullptr)
				app->context->ClearDepthStencilView(CurrentRt.depth.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.0f, 0);
		}
	}

	//Control
	void RES_RENDERER_API SetViewPort(int x, int y, int width, int height) {
		D3D11_VIEWPORT viewport;
		ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

		viewport.TopLeftX = static_cast<float>(x);
		viewport.TopLeftY = static_cast<float>(y);
		viewport.Width = static_cast<float>(width);
		viewport.Height = static_cast<float>(height);

		app->context->RSSetViewports(1, &viewport);
	}

	void RES_RENDERER_API SetRenderWindow(Window window) {
		auto pWindow = static_cast<WindowImpl*>(window);
		app->context->OMSetRenderTargets(1, pWindow->backbufferFrame.rtview.GetAddressOf(), pWindow->backbufferFrame.depth.Get());
	}

	//ErrorCode RES_RENDERER_API DrawMesh(Mesh mesh);

	////Mesh API.
	//ErrorCode RES_RENDERER_API CreateMesh(Mesh* outMesh) {

	//}
	//ErrorCode RES_RENDERER_API UploadMeshData(Mesh mesh, const MeshData* data);
	//ErrorCode RES_RENDERER_API DestroyMesh(Mesh mesh);
}