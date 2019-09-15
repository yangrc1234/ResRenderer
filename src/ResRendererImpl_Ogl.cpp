#include <ResRenderer.hpp>
#include <ResRendererImpl_Ogl.hpp>
#include <GL\glew.h>
namespace ResRenderer {

	static GLenum GetGLAttribType(VertexAttribType otype) {
		switch (otype)
		{
		case ResRenderer::VertexAttribType::ResFloat:
			return GL_FLOAT;
			break;
		default:
			return GL_FLOAT;
			break;
		}
	}

	class MeshImpl
	{
	public:
		MeshImpl() {
			VBO = 0;	
			EBO = 0;
			VAO = 0;
			CHECKED(glGenBuffers(1, &VBO));
			CHECKED(glGenBuffers(1, &EBO));
			CHECKED(glGenVertexArrays(1, &VAO));
		}
		
		~MeshImpl() {	
			glDeleteBuffers(1, &VBO);
			glDeleteBuffers(1, &EBO);
			glDeleteVertexArrays(1, &VAO);
		}
		
		void UploadMeshData(const MeshData* data) {
			meshInitialized = false;
			CHECKED(glBindVertexArray(VAO));
			CHECKED(glBindBuffer(GL_ARRAY_BUFFER, VBO));
			auto vertSize = static_cast<GLsizei>(GetMeshVertexSize(data));
			CHECKED(glBufferData(GL_ARRAY_BUFFER, data->vertCount * vertSize, data->data, GL_STATIC_DRAW));

			CHECKED(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO));
			CHECKED(glBufferData(GL_ELEMENT_ARRAY_BUFFER, data->indiciesCount * sizeof(VertexIndex_t), data->indicies, GL_STATIC_DRAW));

			char* startOffset = 0;
			for (GLuint i = 0; i < (GLuint)data->attribCount; i++)
			{
				auto desc = data->attribDescriptions[i];
				CHECKED(glEnableVertexAttribArray(i));
				CHECKED(glVertexAttribPointer(i, desc.count, GetGLAttribType(desc.type), desc.normalize, vertSize, startOffset));
				startOffset += desc.count * GetVertexAttribSize(desc.type);
			}
			vertCount = data->vertCount;
			meshInitialized = true;
		}

		ErrorCode Draw() {
			if (!meshInitialized)
				return ErrorCode::MESH_NOT_CREATED;
			try
			{
				CHECKED(glBindVertexArray(VAO));
				CHECKED(glDrawArrays(GL_TRIANGLES, 0, vertCount));
				return ErrorCode::RES_NO_ERROR;
			}
			catch (GLenum)
			{
				return ErrorCode::INTERNAL_ERROR;
			}
		}

	private:
		bool meshInitialized = false;
		int vertCount = 0;
		GLuint VAO, VBO, EBO;
	};

	void RES_RENDERER_API SetViewPort(int x, int y, int width, int height) {
		glViewport(x, y, width, height);
	}

	ErrorCode RES_RENDERER_API CreateMesh(Mesh* outMesh){
		try
		{
			auto t = static_cast<Mesh>(new MeshImpl());
			*outMesh = t;
			return ErrorCode::RES_NO_ERROR;
		}
		catch (GLenum)
		{
			return ErrorCode::INTERNAL_ERROR;
		}
	}

	ErrorCode RES_RENDERER_API UploadMeshData(Mesh mesh, const MeshData* data) {
		auto t = MeshDataVerify(data);
		if (t != ErrorCode::RES_NO_ERROR)
			return t;

		auto pMesh = static_cast<MeshImpl*>(mesh);
		try
		{
			pMesh->UploadMeshData(data);
			return ErrorCode::RES_NO_ERROR;
		}
		catch (GLenum)
		{
			return ErrorCode::INTERNAL_ERROR;
		}
	}

	ErrorCode RES_RENDERER_API DestroyMesh(Mesh mesh) {
		delete static_cast<MeshImpl*>(mesh);
		return ErrorCode::RES_NO_ERROR;
	}

	const char* OpenGLShaderVersion = "#version 330 core\n";

	const char* OpenGLShaderHeader = ""
		"#ifdef VERTEX\n"
		"#define INTERP out\n"
		"#define VertData(name,i,type) layout (location = i) in type name;\n"
		"vec4 vertex();\n"
		"	void main()\n"
		"	{\n"
		"		gl_Position = vertex();\n"
		"	}\n"
		"#endif\n"
		"#ifdef FRAGMENT\n"
		"vec4 fragment();\n"
		"#define INTERP in\n"
		"#define VertData(name, i, type)\n"
		"	out vec4 FragColor;\n"
		"	void main()\n"
		"	{\n"
		"		FragColor = fragment();\n"
		"	}\n"
		"#endif\n";

	class ShaderImpl {
	public:
		ShaderImpl()
		{
			vs = 0; ps = 0; program = 0;
			vs = CHECKED(glCreateShader(GL_VERTEX_SHADER));
			ps = CHECKED(glCreateShader(GL_FRAGMENT_SHADER));
			program = CHECKED(glCreateProgram());
		}

		bool CompileVertexShader(const char* source, char* compileErrorLog, size_t compileErrorLogSize, size_t* logLength) {
			int success;

			const char* sourcesvs[] = { OpenGLShaderVersion, "#define VERTEX\n", OpenGLShaderHeader, source };
			glShaderSource(vs, 4, sourcesvs, NULL);
			glCompileShader(vs);
			glGetShaderiv(vs, GL_COMPILE_STATUS, &success);

			GLsizei mlogLength;
			if (!success) {
				glGetShaderInfoLog(vs, static_cast<GLsizei>(compileErrorLogSize), &mlogLength, compileErrorLog);
				*logLength = static_cast<size_t>(mlogLength);
			}
			return success;
		}

		bool CompileFragShader(const char* source, char* compileErrorLog, size_t compileErrorLogSize, size_t* logLength) {
			int success;

			const char* sourcesvs[] = { OpenGLShaderVersion, "#define FRAGMENT\n",OpenGLShaderHeader, source };
			glShaderSource(ps, 4, sourcesvs, NULL);
			glCompileShader(ps);
			glGetShaderiv(ps, GL_COMPILE_STATUS, &success);

			GLsizei mlogLength;
			if (!success) {
				glGetShaderInfoLog(ps, static_cast<GLsizei>(compileErrorLogSize), &mlogLength, compileErrorLog);
				*logLength = static_cast<size_t>(mlogLength);
			}
			return success;
		}

		bool Link(char* errorLog, size_t logBufferSize, size_t* logLength) {
			glAttachShader(program, vs);
			glAttachShader(program, ps);
			glLinkProgram(program);

			int success;
			glGetProgramiv(program, GL_LINK_STATUS, &success);
			GLsizei mlogLength;
			if (!success) {
				glGetProgramInfoLog(program, static_cast<GLsizei>(logBufferSize), &mlogLength, errorLog);
				*logLength = static_cast<size_t>(mlogLength);
			}
			return success;
		}

		bool Compile(const char* source, char* compileErrorLog, size_t compileErrorLogSize, size_t* errorLength) {
			bool success = true;
			size_t errorLogStart = 0;
			size_t partErrLogSize = 0;
			success &= CompileVertexShader(source, compileErrorLog, compileErrorLogSize, &partErrLogSize);
			errorLogStart += partErrLogSize;
			success &= CompileFragShader(source, compileErrorLog + errorLogStart, compileErrorLogSize - errorLogStart, &partErrLogSize);
			errorLogStart += partErrLogSize;
			success &= Link(compileErrorLog + errorLogStart, compileErrorLogSize - errorLogStart, &partErrLogSize);
			errorLogStart += partErrLogSize;

			if (errorLength != nullptr) {
				*errorLength = errorLogStart;
			}

			return success;
		}

		int GetUniformLocation(const char* name) {
			return glGetUniformLocation(program, name);
		}

		void Use() {
			CHECKED(glUseProgram(program));
		}

		~ShaderImpl()
		{
			glDeleteShader(vs);
			glDeleteShader(ps);
			glDeleteProgram(program);
		}
	private:
		GLuint vs, ps, program;
	};

	ErrorCode RES_RENDERER_API CreateShader(Shader* outShader) {
		try
		{
			auto t = new ShaderImpl();
			*outShader = static_cast<Shader>(t);
			return ErrorCode::RES_NO_ERROR;
		}
		catch (GLenum)
		{
			return ErrorCode::INTERNAL_ERROR;
		}
	}

	ErrorCode RES_RENDERER_API CompileShader(Shader shader, const char* source, char* compileErrorLog, size_t compileErrorMaxLength, size_t* compileErrorLength) {
		auto pShader = static_cast<ShaderImpl*>(shader);
		if (!pShader->Compile(source, compileErrorLog, compileErrorMaxLength, compileErrorLength)) {
			return ErrorCode::INTERNAL_ERROR;
		}
		return ErrorCode::RES_NO_ERROR;
	}

	ErrorCode RES_RENDERER_API GetUniformLocation(Shader shader, const char* name, int* outLocation) {
		auto pShader = static_cast<ShaderImpl*>(shader);
		try
		{
			*outLocation = pShader->GetUniformLocation(name);
			return ErrorCode::RES_NO_ERROR;
		}
		catch (GLenum)
		{
			return ErrorCode::INTERNAL_ERROR;
		}
	}

	ErrorCode RES_RENDERER_API SetUniformVec(Shader shader, int location, Vector4 v) {
		auto pShader = static_cast<ShaderImpl*>(shader);
		pShader->Use();
		glUniform4fv(location, 1, (GLfloat*)&v);
		return ErrorCode::RES_NO_ERROR;
	}

	ErrorCode RES_RENDERER_API UseShader(Shader shader) {
		auto pShader = static_cast<ShaderImpl*>(shader);
		try
		{
			pShader->Use();
			return ErrorCode::RES_NO_ERROR;
		}
		catch (GLenum)
		{
			return ErrorCode::INTERNAL_ERROR;
		}
	}


	ErrorCode RES_RENDERER_API DestroyShader(Shader shader) {
		auto pShader = static_cast<ShaderImpl*>(shader);
		delete pShader;
		return ErrorCode::RES_NO_ERROR;
	}
	

	ErrorCode RES_RENDERER_API DrawMesh(Mesh mesh) {

		auto pMesh = static_cast<MeshImpl*>(mesh);
		return pMesh->Draw();
	}
}