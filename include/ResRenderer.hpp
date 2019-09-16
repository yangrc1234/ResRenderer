#pragma once

#include <string>
#include "ResCommon.hpp"
#include "ResMath.hpp"
#include <exception>

//It's planned to provide as DLL(because it's cool!), but shouldn't stuck with C.
//So we use lots of C++ features that don't break dll boundary.

namespace ResRenderer {

	/*
	Error Handling:
	those errors from low-level API is not forced to have corresponding error code.(That will be too much work.)
	all these errors will be returned as INTERNAL_ERROR. Also printing the error to std::cerr is encouraged.

	For other errors, a specific error code is used.
	*/

	enum class ErrorCode
	{
		RES_NO_ERROR = 0,
		INTERNAL_ERROR,
		MESH_DATA_BROKEN,
		MESH_DATA_ATTRIB_OVERFLOW,
		MESH_DATA_LENGTH_ERROR,
		MESH_NOT_CREATED,
	};

	typedef void* Mesh;
	typedef void* Texture;
	typedef void* Shader;
	typedef void* Window;
	typedef void* FrameBuffer;

	bool RES_RENDERER_API Init();
	void RES_RENDERER_API Terminate();
	float RES_RENDERER_API GetTime();

	#define MESH_DATA_MAX_ATTRIB_COUNT 10

	enum class VertexAttribType {
		ResFloat,
	};

	size_t GetVertexAttribSize(VertexAttribType type);

	//MeshData, as helper class. It's not implemented platform-specific, so just use class style.
	struct VertexAttribDescription {
		VertexAttribType type;
		int count;
		bool normalize;
	};

	typedef unsigned int VertexIndex_t;
	struct MeshData {
		VertexAttribDescription attribDescriptions[10];
		int vertCount = 0;
		int attribCount = 0;
		void* data = nullptr;
		size_t dataSize = 0;
		VertexIndex_t *indicies = nullptr;
		size_t indiciesCount = 0;
	};
	ErrorCode RES_RENDERER_API MeshDataAppendAttrib(MeshData* data, VertexAttribType type, int count, bool normalize);
	ErrorCode RES_RENDERER_API MeshDataVerify(const MeshData* data);
	size_t RES_RENDERER_API GetMeshVertexSize(const MeshData* data);
	
	//Mesh API.
	ErrorCode RES_RENDERER_API CreateMesh(Mesh* outMesh);
	ErrorCode RES_RENDERER_API UploadMeshData(Mesh mesh, const MeshData* data);
	ErrorCode RES_RENDERER_API DestroyMesh(Mesh mesh);

	//Texture
	enum TextureFormat {
		RGBA32,
		RGBAFloat
	};

	struct TextureDescriptor {
		TextureFormat format;
		int width;
		int height;
	};

	Texture RES_RENDERER_API CreateTexture(const TextureDescriptor& descriptor);
	void RES_RENDERER_API DestroyTexture(Texture texture);


	//Shader
	//To be simple, shader source code exists in one source file.
	//Underlying should process single source into different(vert/frag) programs.
	//Also, upper application should be aware of the underlying shading language.
	ErrorCode RES_RENDERER_API CreateShader(Shader* outShader);
	ErrorCode RES_RENDERER_API CompileShader(Shader shader, const char* source, char* compileErrorLog, size_t compileErrorMaxLength, size_t* compileErrorLength);
	ErrorCode RES_RENDERER_API GetUniformLocation(Shader shader, const char* name, int* outLocation);
	ErrorCode RES_RENDERER_API SetUniformVec(Shader shader, int location, Vector4 v);
	ErrorCode RES_RENDERER_API UseShader(Shader shader);
	ErrorCode RES_RENDERER_API DestroyShader(Shader shader);

	//FrameBuffer
	struct FrameBufferDescriptor {
		int width;
		int height;
		TextureFormat colorBufferFormat;
	};
	FrameBuffer RES_RENDERER_API CreateFrameBuffer(FrameBufferDescriptor& descriptor);
	void RES_RENDERER_API DestroyFrameBuffer(FrameBuffer frameBuffer);


	//Control
	void RES_RENDERER_API SetViewPort(int x, int y, int width, int height);
	void RES_RENDERER_API SetRenderWindow(Window window);   //wglMakeCurrent
	void RES_RENDERER_API SetRenderTarget(FrameBuffer frameBuffer);
	ErrorCode RES_RENDERER_API DrawMesh(Mesh mesh);
	
	enum class ClearType
	{
		Color = 1,
		Depth = 2,
		ColorAndDepth = 3, 
	};
	void RES_RENDERER_API Clear(Color color, ClearType clearType);


	typedef void(*WindowResizeCallback)(int, int);
	ErrorCode RES_RENDERER_API CreateResWindow(int width, int height, const char* title, Window* outWindow);
	void RES_RENDERER_API RegisterWindowResizeCallback(Window window, WindowResizeCallback callback);
	bool RES_RENDERER_API ShouldCloseWindow(Window window);
	void RES_RENDERER_API SwapBuffer(Window window);
	void RES_RENDERER_API PollEvents();

}
