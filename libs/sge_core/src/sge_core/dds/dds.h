#pragma once

#include "sge_core/sgecore_api.h"
#include "sge_renderer/renderer/GraphicsCommon.h"
#include "sge_utils/sge_utils.h"

namespace sge {

//-----------------------------------------------------------------------
// Taken directly form msdn.
// Follow https://msdn.microsoft.com/en-us/library/windows/desktop/bb943991(v=vs.85).aspx for more details.
//-----------------------------------------------------------------------
enum DDS_DXGI_FORMAT : uint32 {
	DDS_DXGI_FORMAT_UNKNOWN = 0,
	DDS_DXGI_FORMAT_R32G32B32A32_TYPELESS = 1,
	DDS_DXGI_FORMAT_R32G32B32A32_FLOAT = 2,
	DDS_DXGI_FORMAT_R32G32B32A32_UINT = 3,
	DDS_DXGI_FORMAT_R32G32B32A32_SINT = 4,
	DDS_DXGI_FORMAT_R32G32B32_TYPELESS = 5,
	DDS_DXGI_FORMAT_R32G32B32_FLOAT = 6,
	DDS_DXGI_FORMAT_R32G32B32_UINT = 7,
	DDS_DXGI_FORMAT_R32G32B32_SINT = 8,
	DDS_DXGI_FORMAT_R16G16B16A16_TYPELESS = 9,
	DDS_DXGI_FORMAT_R16G16B16A16_FLOAT = 10,
	DDS_DXGI_FORMAT_R16G16B16A16_UNORM = 11,
	DDS_DXGI_FORMAT_R16G16B16A16_UINT = 12,
	DDS_DXGI_FORMAT_R16G16B16A16_SNORM = 13,
	DDS_DXGI_FORMAT_R16G16B16A16_SINT = 14,
	DDS_DXGI_FORMAT_R32G32_TYPELESS = 15,
	DDS_DXGI_FORMAT_R32G32_FLOAT = 16,
	DDS_DXGI_FORMAT_R32G32_UINT = 17,
	DDS_DXGI_FORMAT_R32G32_SINT = 18,
	DDS_DXGI_FORMAT_R32G8X24_TYPELESS = 19,
	DDS_DXGI_FORMAT_D32_FLOAT_S8X24_UINT = 20,
	DDS_DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
	DDS_DXGI_FORMAT_X32_TYPELESS_G8X24_UINT = 22,
	DDS_DXGI_FORMAT_R10G10B10A2_TYPELESS = 23,
	DDS_DXGI_FORMAT_R10G10B10A2_UNORM = 24,
	DDS_DXGI_FORMAT_R10G10B10A2_UINT = 25,
	DDS_DXGI_FORMAT_R11G11B10_FLOAT = 26,
	DDS_DXGI_FORMAT_R8G8B8A8_TYPELESS = 27,
	DDS_DXGI_FORMAT_R8G8B8A8_UNORM = 28,
	DDS_DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
	DDS_DXGI_FORMAT_R8G8B8A8_UINT = 30,
	DDS_DXGI_FORMAT_R8G8B8A8_SNORM = 31,
	DDS_DXGI_FORMAT_R8G8B8A8_SINT = 32,
	DDS_DXGI_FORMAT_R16G16_TYPELESS = 33,
	DDS_DXGI_FORMAT_R16G16_FLOAT = 34,
	DDS_DXGI_FORMAT_R16G16_UNORM = 35,
	DDS_DXGI_FORMAT_R16G16_UINT = 36,
	DDS_DXGI_FORMAT_R16G16_SNORM = 37,
	DDS_DXGI_FORMAT_R16G16_SINT = 38,
	DDS_DXGI_FORMAT_R32_TYPELESS = 39,
	DDS_DXGI_FORMAT_D32_FLOAT = 40,
	DDS_DXGI_FORMAT_R32_FLOAT = 41,
	DDS_DXGI_FORMAT_R32_UINT = 42,
	DDS_DXGI_FORMAT_R32_SINT = 43,
	DDS_DXGI_FORMAT_R24G8_TYPELESS = 44,
	DDS_DXGI_FORMAT_D24_UNORM_S8_UINT = 45,
	DDS_DXGI_FORMAT_R24_UNORM_X8_TYPELESS = 46,
	DDS_DXGI_FORMAT_X24_TYPELESS_G8_UINT = 47,
	DDS_DXGI_FORMAT_R8G8_TYPELESS = 48,
	DDS_DXGI_FORMAT_R8G8_UNORM = 49,
	DDS_DXGI_FORMAT_R8G8_UINT = 50,
	DDS_DXGI_FORMAT_R8G8_SNORM = 51,
	DDS_DXGI_FORMAT_R8G8_SINT = 52,
	DDS_DXGI_FORMAT_R16_TYPELESS = 53,
	DDS_DXGI_FORMAT_R16_FLOAT = 54,
	DDS_DXGI_FORMAT_D16_UNORM = 55,
	DDS_DXGI_FORMAT_R16_UNORM = 56,
	DDS_DXGI_FORMAT_R16_UINT = 57,
	DDS_DXGI_FORMAT_R16_SNORM = 58,
	DDS_DXGI_FORMAT_R16_SINT = 59,
	DDS_DXGI_FORMAT_R8_TYPELESS = 60,
	DDS_DXGI_FORMAT_R8_UNORM = 61,
	DDS_DXGI_FORMAT_R8_UINT = 62,
	DDS_DXGI_FORMAT_R8_SNORM = 63,
	DDS_DXGI_FORMAT_R8_SINT = 64,
	DDS_DXGI_FORMAT_A8_UNORM = 65,
	DDS_DXGI_FORMAT_R1_UNORM = 66,
	DDS_DXGI_FORMAT_R9G9B9E5_SHAREDEXP = 67,
	DDS_DXGI_FORMAT_R8G8_B8G8_UNORM = 68,
	DDS_DXGI_FORMAT_G8R8_G8B8_UNORM = 69,
	DDS_DXGI_FORMAT_BC1_TYPELESS = 70,
	DDS_DXGI_FORMAT_BC1_UNORM = 71,
	DDS_DXGI_FORMAT_BC1_UNORM_SRGB = 72,
	DDS_DXGI_FORMAT_BC2_TYPELESS = 73,
	DDS_DXGI_FORMAT_BC2_UNORM = 74,
	DDS_DXGI_FORMAT_BC2_UNORM_SRGB = 75,
	DDS_DXGI_FORMAT_BC3_TYPELESS = 76,
	DDS_DXGI_FORMAT_BC3_UNORM = 77,
	DDS_DXGI_FORMAT_BC3_UNORM_SRGB = 78,
	DDS_DXGI_FORMAT_BC4_TYPELESS = 79,
	DDS_DXGI_FORMAT_BC4_UNORM = 80,
	DDS_DXGI_FORMAT_BC4_SNORM = 81,
	DDS_DXGI_FORMAT_BC5_TYPELESS = 82,
	DDS_DXGI_FORMAT_BC5_UNORM = 83,
	DDS_DXGI_FORMAT_BC5_SNORM = 84,
	DDS_DXGI_FORMAT_B5G6R5_UNORM = 85,
	DDS_DXGI_FORMAT_B5G5R5A1_UNORM = 86,
	DDS_DXGI_FORMAT_B8G8R8A8_UNORM = 87,
	DDS_DXGI_FORMAT_B8G8R8X8_UNORM = 88,
	DDS_DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
	DDS_DXGI_FORMAT_B8G8R8A8_TYPELESS = 90,
	DDS_DXGI_FORMAT_B8G8R8A8_UNORM_SRGB = 91,
	DDS_DXGI_FORMAT_B8G8R8X8_TYPELESS = 92,
	DDS_DXGI_FORMAT_B8G8R8X8_UNORM_SRGB = 93,
	DDS_DXGI_FORMAT_BC6H_TYPELESS = 94,
	DDS_DXGI_FORMAT_BC6H_UF16 = 95,
	DDS_DXGI_FORMAT_BC6H_SF16 = 96,
	DDS_DXGI_FORMAT_BC7_TYPELESS = 97,
	DDS_DXGI_FORMAT_BC7_UNORM = 98,
	DDS_DXGI_FORMAT_BC7_UNORM_SRGB = 99,
	DDS_DXGI_FORMAT_AYUV = 100,
	DDS_DXGI_FORMAT_Y410 = 101,
	DDS_DXGI_FORMAT_Y416 = 102,
	DDS_DXGI_FORMAT_NV12 = 103,
	DDS_DXGI_FORMAT_P010 = 104,
	DDS_DXGI_FORMAT_P016 = 105,
	DDS_DXGI_FORMAT_420_OPAQUE = 106,
	DDS_DXGI_FORMAT_YUY2 = 107,
	DDS_DXGI_FORMAT_Y210 = 108,
	DDS_DXGI_FORMAT_Y216 = 109,
	DDS_DXGI_FORMAT_NV11 = 110,
	DDS_DXGI_FORMAT_AI44 = 111,
	DDS_DXGI_FORMAT_IA44 = 112,
	DDS_DXGI_FORMAT_P8 = 113,
	DDS_DXGI_FORMAT_A8P8 = 114,
	DDS_DXGI_FORMAT_B4G4R4A4_UNORM = 115,
	DDS_DXGI_FORMAT_P208 = 130,
	DDS_DXGI_FORMAT_V208 = 131,
	DDS_DXGI_FORMAT_V408 = 132,
	DDS_DXGI_FORMAT_FORCE_UINT = 0xffffffff
};

enum DDS_RESOURCE_DIMENSION : uint32 {
	DDS_RESOURCE_DIMENSION_TEXTURE1D = 2,
	DDS_RESOURCE_DIMENSION_TEXTURE2D = 3,
	DDS_RESOURCE_DIMENSION_TEXTURE3D = 4
};

enum DDPF_FLAGS : uint32 {
	DDPF_ALPHAPIXELS = 0x1, // Texture contains alpha data; dwRGBAlphaBitMask contains valid data.
	DDPF_ALPHA = 0x2,  // Used in some older DDS files for alpha channel only uncompressed data (dwRGBBitCount contains the alpha channel
	                   // bitcount; dwABitMask contains valid data)
	DDPF_FOURCC = 0x4, // Texture contains compressed RGB data; dwFourCC contains valid data.
	DDPF_RGB = 0x40, // Texture contains uncompressed RGB data; dwRGBBitCount and the RGB masks (dwRBitMask, dwGBitMask, dwBBitMask) contain
	                 // valid data.
	DDPF_YUV = 0x200, // Used in some older DDS files for YUV uncompressed data (dwRGBBitCount contains the YUV bit count; dwRBitMask
	                  // contains the Y mask, dwGBitMask contains the U mask, dwBBitMask contains the V mask)
	DDPF_LUMINANCE =
	    0x20000, // Used in some older DDS files for single channel color uncompressed data (dwRGBBitCount contains the luminance channel
	             // bit count; dwRBitMask contains the channel mask). Can be combined with DDPF_ALPHAPIXELS for a two channel DDS file.
	DDPF_BUMPDUDV = 0x00080000
};

struct DDS_PIXELFORMAT {
	uint32 dwSize;
	uint32 dwFlags; // one of DDPF_FLAGS
	uint32 dwFourCC;
	uint32 dwRGBBitCount;
	uint32 dwRBitMask;
	uint32 dwGBitMask;
	uint32 dwBBitMask;
	uint32 dwABitMask;
};

enum {
	DDS_D3D11_RESOURCE_MISC_TEXTURECUBE = 0x4L,
};

enum DDSD_FLAGS {
	DDSD_CAPS = 0x1,            // Required in every .dds file.
	DDSD_HEIGHT = 0x2,          // Required in every .dds file.
	DDSD_WIDTH = 0x4,           // Required in every .dds file.
	DDSD_PITCH = 0x8,           // Required when pitch is provided for an uncompressed texture.
	DDSD_PIXELFORMAT = 0x1000,  // Required in every .dds file.
	DDSD_MIPMAPCOUNT = 0x20000, // Required in a mipmapped texture.
	DDSD_LINEARSIZE = 0x80000,  // Required when pitch is provided for a compressed texture.
	DDSD_DEPTH = 0x800000,      // Required in a depth texture.

	DDSD_HELPER_TEXTURE = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT
};

enum DDSCAPS_FALGS {
	DDSCAPS_COMPLEX = 0x8, // Optional; must be used on any file that contains more than one surface (a mipmap, a cubic environment map, or
	                       // mipmapped volume texture).
	DDSCAPS_MIPMAP = 0x400000, // Optional; should be used for a mipmap.
	DDSCAPS_TEXTURE = 0x1000,  // Required.
};

enum DDSCAPS2_FLAG {
	DDSCAPS2_CUBEMAP = 0x200,
	DDSCAPS2_CUBEMAP_POSITIVEX = 0x400,  // Required when these surfaces are stored in a cube map.
	DDSCAPS2_CUBEMAP_NEGATIVEX = 0x800,  // Required when these surfaces are stored in a cube map.
	DDSCAPS2_CUBEMAP_POSITIVEY = 0x1000, // Required when these surfaces are stored in a cube map.
	DDSCAPS2_CUBEMAP_NEGATIVEY = 0x2000, // Required when these surfaces are stored in a cube map.
	DDSCAPS2_CUBEMAP_POSITIVEZ = 0x4000, // Required when these surfaces are stored in a cube map.
	DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x8000, // Required when these surfaces are stored in a cube map.
	DDSCAPS2_VOLUME = 0x200000,          // Required for a volume texture.

	DDSCAPS2_CUBEMAP_ALL = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX | DDSCAPS2_CUBEMAP_NEGATIVEX | DDSCAPS2_CUBEMAP_POSITIVEY |
	                       DDSCAPS2_CUBEMAP_NEGATIVEY | DDSCAPS2_CUBEMAP_POSITIVEZ | DDSCAPS2_CUBEMAP_NEGATIVEZ
};

struct DDS_HEADER {
	uint32 dwSize;
	uint32 dwFlags;
	uint32 dwHeight;
	uint32 dwWidth;
	uint32 dwPitchOrLinearSize;
	uint32 dwDepth;
	uint32 dwMipMapCount;
	uint32 dwReserved1[11];
	DDS_PIXELFORMAT ddspf;
	uint32 dwCaps;
	uint32 dwCaps2;
	uint32 dwCaps3;
	uint32 dwCaps4;
	uint32 dwReserved2;
};

struct DDS_HEADER_DXT10 {
	DDS_DXGI_FORMAT dxgiFormat;
	DDS_RESOURCE_DIMENSION resourceDimension;
	uint32 miscFlag;
	uint32 arraySize;
	uint32 miscFlags2;

	static_assert(sizeof(DDS_RESOURCE_DIMENSION) == sizeof(uint32), "By default resourceDimension is a DWORD");
};

enum : uint32 { DDS_MAGIC_NUMBER = 0x20534444 };

//-----------------------------------------------------------
//
//-----------------------------------------------------------
struct SGE_CORE_API DDSLoader {
  public:
	DDSLoader()
	    : m_ddsData(nullptr)
	    , m_ddsDataSizeBytes(0)
	    , m_ddsDataPointer(0)
	{
	}

	bool load(const char* inputData, const size_t inputDataSizeBytes, TextureDesc& desc, std::vector<TextureData>& initalData);

  private:
	struct SurfaceInfo {
		size_t sliceSizeBytes;
		size_t rowSizeBytes;
	};

	static SurfaceInfo getSurfaceInfo(const int width, const int height, const TextureFormat::Enum textureFormat);

	// Returns the memory that is pointed by m_ddsDataPointer.
	const char* getDataPointer() const { return m_ddsData + m_ddsDataPointer; }

	// Returns the data size that hasn't been read so far.
	size_t getRemainingBytesCount() const
	{
		sgeAssert(m_ddsDataPointer <= m_ddsDataSizeBytes);
		return m_ddsDataSizeBytes - m_ddsDataPointer;
	}

	template <typename T>
	T readNextAs()
	{
		sgeAssert(m_ddsDataPointer <= m_ddsDataSizeBytes);
		T res;
		memcpy(&res, getDataPointer(), sizeof(res));
		m_ddsDataPointer += sizeof(res);
		return res;
	}

  private:
	const char* m_ddsData;
	size_t m_ddsDataSizeBytes;
	size_t m_ddsDataPointer;
};

} // namespace sge
