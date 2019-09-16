#include <ResRenderer.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cmath>

using namespace std;
using namespace ResRenderer;
#define CHECKERROR(x) CheckError(x, __LINE__);

void CheckError(ErrorCode code, int line) {
	if (code != ErrorCode::RES_NO_ERROR) {
		std::cerr << "Error at line: " << line << std::endl;
	}
}

std::string LoadSourceFromFile(const char* filePath) {
	ifstream shaderFile(filePath);
	if (!shaderFile)
		return "";

	ostringstream ss;
	ss << shaderFile.rdbuf();
	
	return ss.str();
}

int main(){
	if (!ResRenderer::Init()) {
		cout << "Failed" << endl;
		return 0;
	}

	ResRenderer::Window pWindow;
    if (ResRenderer::CreateResWindow(800, 600, "233", &pWindow) == ErrorCode::RES_NO_ERROR){

		Mesh mesh;
		CHECKERROR(CreateMesh(&mesh));
		MeshData meshData;
		CHECKERROR(MeshDataAppendAttrib(&meshData, VertexAttribType::ResFloat, 3, false));
		CHECKERROR(MeshDataAppendAttrib(&meshData, VertexAttribType::ResFloat, 3, false));	//Color
		VertexIndex_t indicies[] = { 0, 1, 2 };
		meshData.indicies = indicies;
		meshData.indiciesCount = 3;
		meshData.vertCount = 3;
		float vertices[] = {
			// positions         // colors
			0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,   // bottom right
			-0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,   // bottom left
			0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f    // top 
		};
		meshData.data = vertices;
		meshData.dataSize = sizeof(vertices);
		CHECKERROR(UploadMeshData(mesh, &meshData));

		Shader shader;
		auto shaderSource = LoadSourceFromFile("../resource/shader.glsl");
		char compileError[2000];
		compileError[0] = '\0';
		CHECKERROR(CreateShader(&shader));
		CHECKERROR(CompileShader(shader, shaderSource.c_str(), compileError, 2000, nullptr));
		cout << compileError << endl;

		int location;
		CHECKERROR(GetUniformLocation(shader, "_Tint", &location));
        while (!ResRenderer::ShouldCloseWindow(pWindow)){
			CHECKERROR(UseShader(shader));
			CHECKERROR(DrawMesh(mesh));
			auto Time = fmod(GetTime(), 1.0f);
			SetUniformVec(shader, location, Color(Time, Time, Time, Time));
            SwapBuffer(pWindow);
            PollEvents();
        }

		DestroyMesh(mesh);
		
		ResRenderer::Terminate();
    }else{
        return 0;
    }
}