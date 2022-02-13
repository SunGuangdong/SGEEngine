#include "ShaderReflection.h"
#include "renderer.h"

namespace sge {

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void CBufferFiller::DeleteValue(const int idx)
{
	const auto data_begin = data.begin() + values[idx].offsetBytes;
	const auto data_end = data.begin() + values[idx].offsetBytes + UniformType::GetSizeBytes(values[idx].type);

	data.erase(data_begin, data_end);
	values.erase(values.begin() + idx);
}

void CBufferFiller::SetData(const unsigned nameStrIdx, const UniformType::Enum type, const void* data_arg, const int inDataSizeBytes)
{
	sgeAssert(nameStrIdx != 0);

	const ValueDesc& value = getValueDesc(nameStrIdx, type);
	const size_t numBytes = UniformType::GetSizeBytes(type);
	sgeAssert(inDataSizeBytes == numBytes);
	memcpy(data.data() + value.offsetBytes, data_arg, inDataSizeBytes);
}

CBufferFiller::ValueDesc& CBufferFiller::getValueDesc(const unsigned nameStrIdx, const UniformType::Enum type)
{
	int valueWithSameNameIdx = -1;
	for (size_t t = 0; t < values.size(); ++t) {
		if (values[t].nameStrIdx == nameStrIdx) {
			if (values[t].type == type) {
				return values[t];
			}

			valueWithSameNameIdx = (int)t;
			// break;
		}
	}

	if (valueWithSameNameIdx != -1) {
		const int idx = valueWithSameNameIdx;

		// Change the type of the value.
		values[idx].type = type;

		// Delete the old data.
		const auto data_end = data.begin() + values[idx].offsetBytes + UniformType::GetSizeBytes(values[idx].type);

		// Change the offset and allocate new data
		values[idx].offsetBytes = data.size();
		data.resize(data.size() + UniformType::GetSizeBytes(type));
		return values[idx];
	}

	// At this point value with name "name" doesn't exist.
	ValueDesc value;
	value.nameStrIdx = nameStrIdx;
	value.type = type;
	value.offsetBytes = data.size();
	values.emplace_back(value);

	// Allocate the data for the new value.
	data.resize(data.size() + UniformType::GetSizeBytes(type));

	return values.back();
}

void CBufferFiller::updateCBuffer(SGEContext* context, Buffer* cb, const CBufferFiller& cbRefl)
{
	char* const mappedBuffer = (char*)context->map(cb, Map::WriteDiscard);
	sgeAssert(mappedBuffer);

	for (const auto& cbufferVar : cbRefl.values) {
		sgeAssert(cbufferVar.nameStrIdx != 0);

		for (int iBound = 0; iBound < values.size(); ++iBound) {
			if (values[iBound].nameStrIdx == cbufferVar.nameStrIdx) {
				const size_t numBytes = UniformType::GetSizeBytes(cbufferVar.type);
				const char* const src = data.data() + values[iBound].offsetBytes;

				const char* start = src;
				const char* end = start + numBytes;
				std::copy(start, end, mappedBuffer + cbufferVar.offsetBytes);

				break;
			}
		}
	}

	context->unMap(cb);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void StateGroup::setProgram(ShadingProgram* pShadingProgram)
{
	m_shadingProg = pShadingProgram;
}

void StateGroup::setVB(const int slot, Buffer* pBuffer, const uint32 byteOffset, const uint32 stride)
{
	m_vertexBuffers[slot] = pBuffer;
	m_vbOffsets[slot] = byteOffset;
	m_vbStrides[slot] = stride;
}


void StateGroup::setVBDeclIndex(const VertexDeclIndex idx)
{
	m_vertDeclIndex = idx;
}

void StateGroup::setPrimitiveTopology(const PrimitiveTopology::Enum pt)
{
	m_primTopology = pt;
}

void StateGroup::setIB(Buffer* pBuffer, const UniformType::Enum format, const uint32 byteOffset)
{
	m_indexBufferFormat = format;
	m_indexBuffer = pBuffer;
	m_indexBufferByteOffset = byteOffset;
}

void StateGroup::setRenderState(RasterizerState* rasterState, DepthStencilState* depthStencilState, BlendState* blendState)
{
	m_rasterState = rasterState;
	m_depthStencilState = depthStencilState;
	m_blendState = blendState;
}

// regular draw call with no index buffer
void DrawCall::draw(const uint32 numVerts, const uint32 startVert, const uint32 numInstances)
{
	m_drawExec.Draw(numVerts, startVert, numInstances);
}

// draw call with index buffer
void DrawCall::drawIndexed(const uint32 numIndices, const uint32 startIndex, const uint32 startVert, const uint32 numInstances)
{
	m_drawExec.DrawIndexed(numIndices, startIndex, startVert, numInstances);
}

#if 0
bool DrawCall::ValidateDrawCall() const
{
	return true;

	// Unfortunatley I wrote those check a bit hastefully as they could contain an unused data that has been used in a previous draw call.

	#define SGE_RET_NULL_OR_INVALID(p)                  \
		if ((p) == nullptr || (p).isValid() == false) { \
			sgeAssert(false);                           \
			return false;                               \
		}
	#define SGE_DCV_FAIL_IF(expr) \
		if (expr) {               \
			sgeAssert(false);     \
			return false;         \
		}

	m_shadingProg.isValid();
	SGE_RET_NULL_OR_INVALID(m_shadingProg);
	
	// Check if the bound vertex buffers are valid.
	// [TODO] Check the bound vertex buffers agains the vertex declaration and against the shader refection.
	for(int t = 0; t < SGE_ARRSZ(m_vertexBuffers); ++t) {
		const Buffer buffer = m_vertexBuffers[t];

		if(buffer != nullptr) {
			SGE_DCV_FAIL_IF(buffer.isValid() == false); // Having bound invalid buffer is an error.
			SGE_DCV_FAIL_IF(m_vbStrides[t] == 0); // The strde size in bytes cannot be 0.
		}
	}

	// Vertex declaration must be specified.
	SGE_DCV_FAIL_IF(m_vertDeclIndex == VertexDeclIndex_Null);

	// The primitive topology has to be specified.
	SGE_DCV_FAIL_IF(m_primTopology == PrimitiveTopology::Unknown);

	// Check if there is bound index buffer and if that buffer is valid.
	if(m_indexBuffer != nullptr)
	{
		SGE_DCV_FAIL_IF(m_indexBuffer.isValid() == false);
		SGE_DCV_FAIL_IF(m_indexBufferFormat == UniformType::Unknown); // The format if the index buffer must be specified.

		// An additional check. Usually there are 2 types for index buffer data :
		// uint16 and uint32 indices. Assume that everyting else is not valid
		// (until a new version is added to this world :)).
		SGE_DCV_FAIL_IF(m_indexBufferFormat != UniformType::Uint16 && m_indexBufferFormat != UniformType::Uint);
	}

	// [TODO] Check the cbuffers agains the shaders reflection.
	for(auto& itr : m_boundCbuffers) {
		const Buffer buffer = itr.second;
		SGE_DCV_FAIL_IF(buffer!=nullptr && buffer.isValid() == false);
	}

	// Textures... There is some check that I know I'm missing here.........
	for(auto& itr : m_boundTextures) {
		const Texture texture = itr.second;
		SGE_DCV_FAIL_IF(texture!=nullptr && texture.isValid() == false);
	}

	// Samplers ....
	for(auto& itr : m_boundSamplers) {
		const SamplerState sampler = itr.second;
		SGE_DCV_FAIL_IF(sampler!=nullptr && sampler.isValid() == false);
	}

	// Having null for:
	//		rasterizer state
	//		depth stencil state
	//		blend state
	// is valid (means use the defaults).
	
	SGE_DCV_FAIL_IF(m_rasterState!=nullptr && m_rasterState.isValid() == false);
	SGE_DCV_FAIL_IF(m_depthStencilState!=nullptr && m_depthStencilState.isValid() == false);
	SGE_DCV_FAIL_IF(m_blendState!=nullptr && m_blendState.isValid() == false);

	// The draw settings must be specified.
	SGE_DCV_FAIL_IF(m_drawExec.IsValid() == false);

	#undef SGE_ASSERT_AND_RET_FALSE
	#undef SGE_NOT_NULL_VALID

	// The draw call shoud be valid.
	return true;
}
#endif

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
ClearColorCmd::ClearColorCmd(FrameTarget* frameTarget, int index, const float rgba[4])
{
	m_frameTarget = frameTarget;
	m_index = index;

	std::copy(rgba, rgba + 4, m_rgba);
}

ClearDepthStencilCmd::ClearDepthStencilCmd(FrameTarget* frameTarget, float depth)
{
	m_frameTarget = frameTarget;
	m_depth = depth;
}

} // namespace sge
