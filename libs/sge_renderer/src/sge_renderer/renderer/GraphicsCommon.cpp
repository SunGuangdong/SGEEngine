#include "GraphicsCommon.h"
#include <algorithm>

namespace sge {

//--------------------------------------------------------
// TextureFormat
//--------------------------------------------------------
size_t TextureFormat::GetSizeBits(const TextureFormat::Enum format)
{
	switch (format) {
		case R32G32B32A32_FLOAT:
		case R32G32B32A32_UINT:
		case R32G32B32A32_SINT:
			return 128;

		case R32G32B32_FLOAT:
		case R32G32B32_UINT:
		case R32G32B32_SINT:
			return 96;

		case R16G16B16A16_FLOAT:
		case R16G16B16A16_UNORM:
		case R16G16B16A16_UINT:
		case R16G16B16A16_SNORM:
		case R16G16B16A16_SINT:
		case R32G32_FLOAT:
		case R32G32_UINT:
		case R32G32_SINT:
			return 64;

		case R10G10B10A2_UNORM:
		case R10G10B10A2_UINT:
		case R11G11B10_FLOAT:

		case R8G8B8A8_UNORM:
		case R8G8B8A8_UNORM_SRGB:
		case R8G8B8A8_UINT:
		case R8G8B8A8_SNORM:
		case R8G8B8A8_SINT:

		case R16G16_FLOAT:
		case R16G16_UNORM:
		case R16G16_UINT:
		case R16G16_SNORM:
		case R16G16_SINT:

		case R32_FLOAT:
		case R32_UINT:
		case R32_SINT:

		case D24_UNORM_S8_UINT:
			return 32;
		case D32_FLOAT:
			return 32;

		case R8G8_UNORM:
		case R8G8_UINT:
		case R8G8_SNORM:
		case R8G8_SINT:

		case R16_FLOAT:

		case R16_UNORM:
		case R16_UINT:
		case R16_SNORM:
		case R16_SINT:
			return 16;

		case R8_UNORM:
		case R8_UINT:
		case R8_SNORM:
		case R8_SINT:
			return 8;

		case A8_UNORM:
			return 8;

		// Note that the block compression sizes could be wrong!
		case BC1_UNORM:
			return 8;
		case BC2_UNORM:
			return 8;
		case BC3_UNORM:
			return 8;

		case BC4_UNORM:
			return 8;
		case BC4_SNORM:
			return 8;

		case BC5_UNORM:
			return 16;
		case BC5_SNORM:
			return 16;
	};

	if (format != Unknown) {
		// [TODO] Unimplemented/unknown format.
		sgeAssert(false);
	}

	return 0;
}

size_t TextureFormat::GetSizeBytes(const TextureFormat::Enum format)
{
	return (TextureFormat::GetSizeBits(format) + 7) / 8;
}

//------------------------------------------------------------------------------
// TextureDesc
//------------------------------------------------------------------------------
TextureDesc TextureDesc::GetDefaultRenderTarget(int width, int height, TextureFormat::Enum format)
{
	TextureDesc retval;

	retval.usage = TextureUsage::RenderTargetResource;
	retval.textureType = UniformType::Texture2D;
	retval.format = format;

	retval.texture2D.width = width;
	retval.texture2D.height = height;
	retval.texture2D.arraySize = 1;
	retval.texture2D.numMips = 1;
	retval.texture2D.numSamples = 1;
	retval.texture2D.sampleQuality = 0;

	return retval;
}

TextureDesc TextureDesc::GetDefaultDepthStencil(int width, int height, TextureFormat::Enum format)
{
	TextureDesc retval;

	retval.usage = TextureUsage::DepthStencilResource;
	retval.textureType = UniformType::Texture2D;
	retval.format = format;

	retval.texture2D.width = width;
	retval.texture2D.height = height;
	retval.texture2D.arraySize = 1;
	retval.texture2D.numMips = 1;
	retval.texture2D.numSamples = 1;
	retval.texture2D.sampleQuality = 0;

	return retval;
}

//------------------------------------------------------------------------------
// UniformType
//------------------------------------------------------------------------------
UniformType::Enum UniformType::PickType(UniformType::Enum type, const int lanes, const int registers)
{
	if (registers == 1) {
		// [TODO] EXPAND THIS DON'T be a HACKER.
		return (UniformType::Enum)((int)(type) + (lanes - 1));
	}
	else if (registers == 4 && lanes == 4 && type == Float) {
		return Float4x4;
	}
	else if (registers == 3 && lanes == 3 && type == Float) {
		return Float3x3;
	}

	sgeAssert(false);
	return UniformType::Unknown;
}

int UniformType::GetSizeBytes(const UniformType::Enum uniformType)
{
	switch (uniformType) {
		case Unknown:
			return 0;

		case Float:
			return 4;
		case Float2:
			return 8;
		case Float3:
			return 12;
		case Float4:
			return 16;

		case Double:
			return 8;
		case Double2:
			return 16;
		case Double3:
			return 24;
		case Double4:
			return 32;

		case Int:
			return 4;
		case Int2:
			return 8;
		case Int3:
			return 12;
		case Int4:
			return 16;

		case Uint16:
			return 2;

		case Uint:
			return 4;
		case Uint2:
			return 8;
		case Uint3:
			return 12;
		case Uint4:
			return 16;

		case Float3x3:
			return 36;
		case Float4x4:
			return 64;

		case Int_RGBA_Unorm_IA:
			return 4;
	};

	sgeAssert(false);
	return 0;
}

//-------------------------------------------------------------------------------
// Buffers
//-------------------------------------------------------------------------------
BufferDesc BufferDesc::GetDefaultVertexBuffer(const size_t sizeBytes, const ResourceUsage::Enum usage)
{
	BufferDesc result;

	result.sizeBytes = sizeBytes;
	result.usage = usage;
	result.bindFlags = ResourceBindFlags::VertexBuffer;
	result.structByteStride = 0;

	return result;
}

BufferDesc BufferDesc::GetDefaultIndexBuffer(const size_t sizeBytes, const ResourceUsage::Enum usage)
{
	BufferDesc result;

	result.bindFlags = ResourceBindFlags::IndexBuffer;
	result.sizeBytes = sizeBytes;
	result.structByteStride = 0;
	result.usage = usage;

	return result;
}

BufferDesc BufferDesc::GetDefaultConstantBuffer(const size_t sizeBytes, const ResourceUsage::Enum usage)
{
	BufferDesc result;

	result.sizeBytes = sizeBytes;
	result.usage = usage;
	result.bindFlags = ResourceBindFlags::ConstantBuffer;
	result.structByteStride = 0;

	return result;
}

//-------------------------------------------------------------------
// PrimitiveTopology
//-------------------------------------------------------------------
int PrimitiveTopology::GetNumPrimitivesByPoints(const PrimitiveTopology::Enum topology, const int numPoints)
{
	switch (topology) {
		case Unknown:
			sgeAssert(false);
			return 0;
		case TriangleList:
			return numPoints / 3;
		case TriangleStrip:
			return numPoints > 2 ? numPoints - 2 : 0;
		case LineList:
			return numPoints / 2;
		case LineStrip:
			return numPoints - 1;
		case PointList:
			return numPoints;
	}

	// Unimplemented primitive type
	sgeAssert(false);
	return 0;
}

int PrimitiveTopology::GetNumPointsByPrimitives(const PrimitiveTopology::Enum topology, const int numPrimitives)
{
	if (numPrimitives == 0)
		return 0;

	switch (topology) {
		case Unknown:
			sgeAssert(false);
			return 0;
		case TriangleList:
			return numPrimitives * 3;
		case TriangleStrip:
			return numPrimitives + 2;
		case LineList:
			return numPrimitives * 2;
		case LineStrip:
			return numPrimitives + 1;
		case PointList:
			return numPrimitives;
	}

	// Unimplemented primitive type
	sgeAssert(false);
	return 0;
}

//-------------------------------------------------------------------
// VertexDecl
//-------------------------------------------------------------------
std::vector<VertexDecl> VertexDecl::NormalizeDecl(const VertexDecl* pDecl, const int numDeclElems)
{
	sgeAssert(numDeclElems > 0);
	if (numDeclElems <= 0)
		return std::vector<VertexDecl>();

	// Remove any automatic offseting (eg VertexDecl::byteOffset == -1)
	// [TODO] Change the '16' with GraphicsCaps::kVertexBufferSlotsCount
	int accumOffset[16] = {0};

	// int accumOffset = 0;
	std::vector<VertexDecl> result;
	result.resize(numDeclElems);
	for (int t = 0; t < numDeclElems; ++t) {
		const auto slot = pDecl[t].bufferSlot;
		result[t] = pDecl[t];

		if (result[t].byteOffset == -1) {
			result[t].byteOffset = accumOffset[slot];
		}
		const int formatSize = UniformType::GetSizeBytes(result[t].format);
		accumOffset[slot] = result[t].byteOffset + formatSize;

		// vertex buffeer elements must be 4 byte aligned
		if (accumOffset[slot] % 4 != 0) {
			accumOffset[slot] += 4 - (accumOffset[slot] % 4);
		}
	}

	// sort the declaration element by buffer slot and then by offset value
	std::sort(result.begin(), result.end(), [](const VertexDecl& a, const VertexDecl& b) -> bool {
		// remember how || operator evaluates its arguments
		return a.bufferSlot < b.bufferSlot || a.byteOffset < b.byteOffset;
	});

	return std::move(result);
}

//-------------------------------------------------------------------
// Blending
//-------------------------------------------------------------------
bool BlendDesc::operator==(const BlendDesc& other) const
{
	return enabled == other.enabled && srcBlend == other.srcBlend && destBlend == other.destBlend && blendOp == other.blendOp &&
	       alphaSrcBlend == other.alphaSrcBlend && alphaDestBlend == other.alphaDestBlend && alphaBlendOp == other.alphaBlendOp;
}

bool BlendDesc::operator!=(const BlendDesc& other) const
{
	return !(*this == other);
}

BlendDesc BlendDesc::GetDefaultBackToFrontAlpha()
{
	BlendDesc blendDesc;

	blendDesc.enabled = true;
	blendDesc.srcBlend = Blend::Alpha_Src;
	blendDesc.destBlend = Blend::Alpha_InvSrc;
	blendDesc.blendOp = BlendOp::Add;
	blendDesc.alphaSrcBlend = Blend::Alpha_Src;
	blendDesc.alphaDestBlend = Blend::Alpha_InvSrc;
	blendDesc.alphaBlendOp = BlendOp::Add;

	return blendDesc;
}

BlendDesc BlendDesc::getColorAdditiveBlending()
{
	BlendDesc blendDesc;

	blendDesc.enabled = true;
	blendDesc.srcBlend = Blend::One;
	blendDesc.destBlend = Blend::One;
	blendDesc.blendOp = BlendOp::Add;
	blendDesc.alphaSrcBlend = Blend::One;
	blendDesc.alphaDestBlend = Blend::One;
	blendDesc.alphaBlendOp = BlendOp::Max;

	return blendDesc;
}

BlendStateDesc BlendStateDesc::GetDefaultBackToFrontAlpha()
{
	BlendStateDesc retval;

	retval.independentBlend = false;
	retval.blendDesc[0] = BlendDesc::GetDefaultBackToFrontAlpha();

	return retval;
}

BlendStateDesc BlendStateDesc::getColorAdditiveBlending()
{
	BlendStateDesc retval;

	retval.independentBlend = false;
	retval.blendDesc[0] = BlendDesc::getColorAdditiveBlending();

	return retval;
}

bool BlendStateDesc::operator==(const BlendStateDesc& other) const
{
	if (independentBlend == false) {
		return blendDesc[0] == other.blendDesc[0];
	}

	for (size_t t = 0; t < blendDesc.size(); ++t) {
		if (blendDesc[t] != other.blendDesc[t]) {
			return false;
		}
	}

	return true;
}

bool BlendStateDesc::operator!=(const BlendStateDesc& other) const
{
	return !(*this == other);
}
} // namespace sge
