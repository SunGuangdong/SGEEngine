#pragma once

#include "sge_core/sgecore_api.h"
#include "sge_renderer/renderer/renderer.h"

namespace sge {

struct Geometry {
	Geometry() = default;
	Geometry(Buffer* vertexBuffer,
	         Buffer* indexBuffer,
	         Texture* skinningBoneTransforms,
	         int firstBoneOffset, ///< The offset of the 1st bone of the mesh in @skinningBoneTransforms. -1 if there are no bones.
	         VertexDeclIndex vertexDeclIndex,
	         bool vertexDeclHasVertexColor,
	         bool vertexDeclHasUv,
	         bool vertexDeclHasNormals,
	         bool vertexDeclHasTangentSpace,
	         PrimitiveTopology::Enum topology,
	         int vbByteOffset,
	         int ibByteOffset,
	         int stride,
	         UniformType::Enum ibFmt,
	         int numElements)
	    : vertexBuffer(vertexBuffer)
	    , indexBuffer(indexBuffer)
	    , skinningBoneTransforms(skinningBoneTransforms)
	    , firstBoneOffset(firstBoneOffset)
	    , vertexDeclIndex(vertexDeclIndex)
	    , vertexDeclHasVertexColor(vertexDeclHasVertexColor)
	    , vertexDeclHasUv(vertexDeclHasUv)
	    , vertexDeclHasNormals(vertexDeclHasNormals)
	    , vertexDeclHasTangentSpace(vertexDeclHasTangentSpace)
	    , topology(topology)
	    , vbByteOffset(vbByteOffset)
	    , ibByteOffset(ibByteOffset)
	    , stride(stride)
	    , ibFmt(ibFmt)
	    , numElements(numElements)
	{
	}

	bool hasData() const { return vertexBuffer != nullptr; }

	bool hasVertexSkinning() const { return skinningBoneTransforms != nullptr && firstBoneOffset >= 0; }

  public:
	Buffer* vertexBuffer = nullptr;
	Buffer* indexBuffer = nullptr;
	Texture* skinningBoneTransforms = nullptr;
	int firstBoneOffset = -1;
	VertexDeclIndex vertexDeclIndex = VertexDeclIndex_Null; // The index of the vertex memory layout. See SGEDevice for more info.
	bool vertexDeclHasVertexColor = false;
	bool vertexDeclHasUv = false;
	bool vertexDeclHasNormals = false;
	bool vertexDeclHasTangentSpace = false;
	PrimitiveTopology::Enum topology = PrimitiveTopology::Unknown;

	uint32 vbByteOffset = 0;                        ///< 1st vertex byte offset into the vertex buffer
	uint32 ibByteOffset = 0;                        ///< 1st index byte offse int the index buffer
	int stride = 0;                                 ///< The size of a whole vertex in bytes.
	UniformType::Enum ibFmt = UniformType::Unknown; ///< The format the index buffer, if unknown this mesh doesn't use index buffers.
	uint32 numElements = 0;                         ///< The number of vertices/indices used by this mesh.
};


} // namespace sge
