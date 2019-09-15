#include <ResRenderer.hpp>
#include <algorithm>

namespace ResRenderer {

	size_t GetVertexAttribSize(VertexAttribType type) {
		switch (type)
		{
		case ResRenderer::VertexAttribType::ResFloat:
			return 4;
			break;
		default:
			return 0;
			break;
		}
	}

	ErrorCode RES_RENDERER_API MeshDataAppendAttrib(MeshData* data, VertexAttribType type, int count, bool normalize){
		if (data->attribCount >= MESH_DATA_MAX_ATTRIB_COUNT) {
			return ErrorCode::MESH_DATA_ATTRIB_OVERFLOW;
		}
		if (data->attribCount < 0) {
			return ErrorCode::MESH_DATA_BROKEN;
		}

		VertexAttribDescription vad;
		vad.type = type;
		vad.normalize = normalize;
		vad.count = count;
		data->attribDescriptions[data->attribCount++] = vad;

		return ErrorCode::RES_NO_ERROR;
	}

	size_t RES_RENDERER_API GetMeshVertexSize(const MeshData* data) {
		size_t vertSize = 0;
		for (size_t i = 0; i < data->attribCount; i++)
		{
			auto desc = data->attribDescriptions[i];
			vertSize += desc.count * GetVertexAttribSize(desc.type);
		}
		return vertSize;
	}

	ErrorCode RES_RENDERER_API MeshDataVerify(const MeshData* data) {
		if (data->attribCount >= MESH_DATA_MAX_ATTRIB_COUNT || data->vertCount <= 0 || data->data == nullptr || data->indicies == nullptr || data->dataSize <= 0 || data->indiciesCount <= 0) {
			return ErrorCode::MESH_DATA_BROKEN;
		}
		if (data->attribCount < 0) {
			return ErrorCode::MESH_DATA_BROKEN;
		}
		
		//Check data length.
		if (GetMeshVertexSize(data) * data->vertCount != data->dataSize) {
			return ErrorCode::MESH_DATA_LENGTH_ERROR;
		}

		return ErrorCode::RES_NO_ERROR;
	}
}