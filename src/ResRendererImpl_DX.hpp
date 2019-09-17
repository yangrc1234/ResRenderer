#include <cassert>
#include <Windows.h>
#include <d3d11.h>
#include <iostream>

#define CHECKED(x) CheckWindowsError(x, false, __FILE__, __LINE__, __FUNCTION__)
//#define NOERROR(x) CheckWindowsError(x, true,  __FILE__, __LINE__, __FUNCTION__)
inline void CheckWindowsError(HRESULT hr, bool noerror, const char* file, int line, const char* func) {
	if (FAILED(hr)) {
		std::cerr << "DX error happened in function: " << func << " Line: " << line << " File: " << file << std::endl;
		assert(!noerror);
		throw hr;
	}
}