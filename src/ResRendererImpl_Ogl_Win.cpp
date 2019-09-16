#include <ResRenderer.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <map>
#include <iostream>

namespace ResRenderer {
	static void GLFWOnFrameSizeChanged(GLFWwindow* _window, int width, int height);
	bool contextInitialized = false;
	class WindowImpl {
	public:
		WindowImpl(int width, int height, const char* title) {
			window = glfwCreateWindow(width, height, title, NULL, NULL);
			glfwMakeContextCurrent(window);
            if (!contextInitialized){
                contextInitialized = true;
                glewInit();
            }
			mMap.insert(std::pair<GLFWwindow*, WindowImpl*>(window, this));
			glfwSetFramebufferSizeCallback(window, GLFWOnFrameSizeChanged);
		}

		~WindowImpl()
		{
			mMap.erase(this->window);
			glfwDestroyWindow(this->window);
		}

		void RegisterResizeFunc(WindowResizeCallback _callback) {
			this->callback = _callback;
		}

		bool ShouldCloseWindow() {
			return glfwWindowShouldClose(window);
		}

		void Swapbuffer() {
			glfwSwapBuffers(window);
		}
		static std::map<GLFWwindow*, WindowImpl*> mMap;
		WindowResizeCallback callback = nullptr;
	private:
		GLFWwindow* window = nullptr;
	};

	std::map<GLFWwindow*, WindowImpl*> WindowImpl::mMap;

	static void GLFWOnFrameSizeChanged(GLFWwindow* _window, int width, int height) {
		static_cast<GLFWwindow*>(_window);
		auto iteWindowImpl = WindowImpl::mMap.find(_window);
		if (iteWindowImpl == WindowImpl::mMap.end()) {
			std::cerr << "Glfw window error!" << std::endl;
			return;
		}
		if (iteWindowImpl->second->callback != nullptr) {
			iteWindowImpl->second->callback(width, height);
		}
	}

	bool RES_RENDERER_API Init() {
		if (!glfwInit())
			return false;
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		
		return true;
	}

	float RES_RENDERER_API GetTime() {
		return static_cast<float>(glfwGetTime());
	}

	void RES_RENDERER_API Terminate() {
		glfwTerminate();
	}

	ErrorCode RES_RENDERER_API CreateResWindow(int width, int height, const char* title, Window* outWindow) {
		try {
			auto t = static_cast<Window>(new WindowImpl(width, height, title));
			*outWindow = t;
			return ErrorCode::RES_NO_ERROR;
		}
		catch (std::exception&) {
			return ErrorCode::INTERNAL_ERROR;
		}
	}

	void RES_RENDERER_API RegisterWindowResizeCallback(Window window, WindowResizeCallback callback) {
		auto pWindow = static_cast<WindowImpl*>(window);
		pWindow->RegisterResizeFunc(callback);
	}

	bool RES_RENDERER_API ShouldCloseWindow(Window window) {
		auto pWindow = static_cast<WindowImpl*>(window);
		return pWindow->ShouldCloseWindow();
	}

	void RES_RENDERER_API SwapBuffer(Window window) {
		auto pWindow = static_cast<WindowImpl*>(window);
		pWindow->Swapbuffer();
	}

	void RES_RENDERER_API PollEvents() {
		glfwPollEvents();
	}

	void RES_RENDERER_API Clear(Color color, ClearType clearType) {
		glClearColor(color.r, color.g, color.b, color.a);
		GLbitfield mask = 0;
		if (clearType == ClearType::Color || clearType == ClearType::ColorAndDepth) {
			mask |= GL_COLOR_BUFFER_BIT;
		}
		if (clearType == ClearType::Depth || clearType == ClearType::ColorAndDepth) {
			mask |= GL_DEPTH_BUFFER_BIT;
		}
		glClear(mask);
	}
}