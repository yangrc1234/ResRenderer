#pragma once
#include <windows.h>
#include <GL\glew.h>
#include <ResRenderer.hpp>
//This header places common used functions.
//Not all classes are required to be here.
#include <iostream>

namespace ResRenderer {

#define CHECKED(x) x; \
					CheckOpenGLErrorAndThrow(__FILE__, __LINE__, __FUNCTION__);\

	inline void CheckOpenGLErrorAndThrow(const char* file, int line, const char* func) {
		GLenum err;
		if ((err = glGetError()) != GL_NO_ERROR) {
			std::cerr << "OpenGL error happened in function: " << func << " Line: " << line << " File: " << file << std::endl;
			throw err;
		}
	}
}