#include <ResRenderer.hpp>
#include <map>
#include <iostream>
#include <ctime>
#include <ResRendererImpl_DX.hpp>
#include <wrl/client.h>
#include <dxgi1_3.h>
using namespace Microsoft::WRL;

namespace ResRenderer {


	class WindowImpl {
	public:
		HWND handle = NULL;
		WindowResizeCallback callback = nullptr;
		bool isClosing = false;
		ComPtr<IDXGISwapChain1> swapChain = NULL;
		
		WindowImpl(HWND _handle)
		{
			maps.insert(std::pair<HWND, WindowImpl*>(handle, this));
			handle = _handle;
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

	class D3dApp {
	public:
		bool Init() {
			HRESULT hr;

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
			catch (HRESULT hr)
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
				auto result = new WindowImpl(hwnd);
				result->handle = hwnd;
				*outWindow = result;
				return ErrorCode::RES_NO_ERROR;
			}
			catch (HRESULT err)
			{
				return ErrorCode::INTERNAL_ERROR;
			}
		}

		void CreateSwapChain(WindowImpl* window) {
			DXGI_SWAP_CHAIN_DESC desc;
			ZeroMemory(&desc, sizeof(DXGI_SWAP_CHAIN_DESC));
			desc.Windowed = TRUE; // Sets the initial state of full-screen mode.
			desc.BufferCount = 2;
			desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			desc.SampleDesc.Count = 1;      //multisampling setting
			desc.SampleDesc.Quality = 0;    //vendor-specific flag
			desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
			desc.OutputWindow = window->handle;

			IDXGISwapChain* t;
			CHECKED(factory->CreateSwapChain(
				dxgiDevice.Get(),
				&desc,
				&t
			));
			window->swapChain = t;
		}

		~D3dApp()
		{
			UnregisterClass(WindowClassName, m_hInstance);
		}

		// Create the DX11 API device object, and get a corresponding context.
		ComPtr<ID3D11Device> device;
		ComPtr<IDXGIDevice3> dxgiDevice;
		ComPtr<IDXGIAdapter> adapter;
		ComPtr<IDXGIFactory> factory;
		ComPtr<ID3D11DeviceContext> context;
		HINSTANCE m_hInstance = NULL;
		const char* WindowClassName = "Main Window";
		time_t startTime = 0;
		D3D_FEATURE_LEVEL m_featureLevel;
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
		switch (uMsg)
		{
		case WM_SIZE:
			RECT clipRect;
			GetClientRect(hWnd, &clipRect);
			auto pWimpl = WindowImpl::GetWindow(hWnd);
			if (pWimpl->callback != nullptr)
				pWimpl->callback(clipRect.right - clipRect.left, clipRect.top - clipRect.bottom);
			break;
		case WM_QUIT:
			auto pWimpl = WindowImpl::GetWindow(hWnd);
			pWimpl->isClosing = true;
		default:
			break;
		}
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

	}

	void RES_RENDERER_API PollEvents() {

	}

	void RES_RENDERER_API Clear(Color color, ClearType clearType) {

	}
}