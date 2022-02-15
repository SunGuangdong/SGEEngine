#include "dds.h"
#include "sge_renderer/renderer/GraphicsCommon.h"

namespace sge {

namespace {

	template <typename T>
	T dds_min(T a, T b)
	{
		if (a < b)
			return a;
		return b;
	}

	template <typename T>
	T dds_max(T a, T b)
	{
		if (a > b)
			return a;
		return b;
	}

	TextureFormat::Enum DDS_DXGI_FORMAT_to_TextureFormat(const DDS_DXGI_FORMAT& format)
	{
		switch (format) {
			case DDS_DXGI_FORMAT_UNKNOWN:
				return TextureFormat::Unknown;

			case DDS_DXGI_FORMAT_R32G32B32A32_TYPELESS:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_R32G32B32A32_FLOAT:
				return TextureFormat::R32G32B32A32_FLOAT;
			case DDS_DXGI_FORMAT_R32G32B32A32_UINT:
				return TextureFormat::R32G32B32A32_UINT;
			case DDS_DXGI_FORMAT_R32G32B32A32_SINT:
				return TextureFormat::R32G32B32A32_SINT;

			case DDS_DXGI_FORMAT_R32G32B32_TYPELESS:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_R32G32B32_FLOAT:
				return TextureFormat::R32G32B32_FLOAT;
			case DDS_DXGI_FORMAT_R32G32B32_UINT:
				return TextureFormat::R32G32B32_UINT;
			case DDS_DXGI_FORMAT_R32G32B32_SINT:
				return TextureFormat::R32G32B32_SINT;

			case DDS_DXGI_FORMAT_R16G16B16A16_TYPELESS:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_R16G16B16A16_FLOAT:
				return TextureFormat::R16G16B16A16_FLOAT;
			case DDS_DXGI_FORMAT_R16G16B16A16_UNORM:
				return TextureFormat::R16G16B16A16_UNORM;
			case DDS_DXGI_FORMAT_R16G16B16A16_UINT:
				return TextureFormat::R16G16B16A16_UINT;
			case DDS_DXGI_FORMAT_R16G16B16A16_SNORM:
				return TextureFormat::R16G16B16A16_SNORM;
			case DDS_DXGI_FORMAT_R16G16B16A16_SINT:
				return TextureFormat::R16G16B16A16_SINT;

			case DDS_DXGI_FORMAT_R32G32_TYPELESS:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_R32G32_FLOAT:
				return TextureFormat::R32G32_FLOAT;
			case DDS_DXGI_FORMAT_R32G32_UINT:
				return TextureFormat::R32G32_UINT;
			case DDS_DXGI_FORMAT_R32G32_SINT:
				return TextureFormat::R32G32_SINT;

			case DDS_DXGI_FORMAT_R32G8X24_TYPELESS:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
				return TextureFormat::Unknown; // Currently not supported.
			case DDS_DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
				return TextureFormat::Unknown; // Currently not supported.
			case DDS_DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
				return TextureFormat::Unknown; // Currently not supported.

			case DDS_DXGI_FORMAT_R10G10B10A2_TYPELESS:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_R10G10B10A2_UNORM:
				return TextureFormat::R10G10B10A2_UNORM;
			case DDS_DXGI_FORMAT_R10G10B10A2_UINT:
				return TextureFormat::R10G10B10A2_UINT;

			case DDS_DXGI_FORMAT_R11G11B10_FLOAT:
				return TextureFormat::R11G11B10_FLOAT;

			case DDS_DXGI_FORMAT_R8G8B8A8_TYPELESS:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_R8G8B8A8_UNORM:
				return TextureFormat::R8G8B8A8_UNORM;
			case DDS_DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
				return TextureFormat::R8G8B8A8_UNORM_SRGB;
			case DDS_DXGI_FORMAT_R8G8B8A8_UINT:
				return TextureFormat::R8G8B8A8_UINT;
			case DDS_DXGI_FORMAT_R8G8B8A8_SNORM:
				return TextureFormat::R8G8B8A8_SNORM;
			case DDS_DXGI_FORMAT_R8G8B8A8_SINT:
				return TextureFormat::R8G8B8A8_SINT;

			case DDS_DXGI_FORMAT_R16G16_TYPELESS:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_R16G16_FLOAT:
				return TextureFormat::R16G16_FLOAT;
			case DDS_DXGI_FORMAT_R16G16_UNORM:
				return TextureFormat::R16G16_UNORM;
			case DDS_DXGI_FORMAT_R16G16_UINT:
				return TextureFormat::R16G16_UINT;
			case DDS_DXGI_FORMAT_R16G16_SNORM:
				return TextureFormat::R16G16_SNORM;
			case DDS_DXGI_FORMAT_R16G16_SINT:
				return TextureFormat::R16G16_SINT;

			case DDS_DXGI_FORMAT_R32_TYPELESS:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_D32_FLOAT:
				return TextureFormat::Unknown; // Currently not supported.
			case DDS_DXGI_FORMAT_R32_FLOAT:
				return TextureFormat::R32_FLOAT;
			case DDS_DXGI_FORMAT_R32_UINT:
				return TextureFormat::R32_UINT;
			case DDS_DXGI_FORMAT_R32_SINT:
				return TextureFormat::R32_SINT;

			case DDS_DXGI_FORMAT_R24G8_TYPELESS:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_D24_UNORM_S8_UINT:
				return TextureFormat::D24_UNORM_S8_UINT;

			case DDS_DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_X24_TYPELESS_G8_UINT:
				return TextureFormat::Unknown; // Currently not supported.

			case DDS_DXGI_FORMAT_R8G8_TYPELESS:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_R8G8_UNORM:
				return TextureFormat::R8G8_UNORM;
			case DDS_DXGI_FORMAT_R8G8_UINT:
				return TextureFormat::R8G8_UINT;
			case DDS_DXGI_FORMAT_R8G8_SNORM:
				return TextureFormat::R8G8_SNORM;
			case DDS_DXGI_FORMAT_R8G8_SINT:
				return TextureFormat::R8G8_SINT;

			case DDS_DXGI_FORMAT_R16_TYPELESS:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_R16_FLOAT:
				return TextureFormat::R16_FLOAT;
			case DDS_DXGI_FORMAT_D16_UNORM:
				return TextureFormat::Unknown; // Currently not supported.
			case DDS_DXGI_FORMAT_R16_UNORM:
				return TextureFormat::R16_UNORM;
			case DDS_DXGI_FORMAT_R16_UINT:
				return TextureFormat::R16_UINT;
			case DDS_DXGI_FORMAT_R16_SNORM:
				return TextureFormat::R16_SNORM;
			case DDS_DXGI_FORMAT_R16_SINT:
				return TextureFormat::R16_SINT;

			case DDS_DXGI_FORMAT_R8_TYPELESS:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_R8_UNORM:
				return TextureFormat::R8_UNORM;
			case DDS_DXGI_FORMAT_R8_UINT:
				return TextureFormat::R8_UINT;
			case DDS_DXGI_FORMAT_R8_SNORM:
				return TextureFormat::R8_SNORM;
			case DDS_DXGI_FORMAT_R8_SINT:
				return TextureFormat::R8_SINT;
			case DDS_DXGI_FORMAT_A8_UNORM:
				return TextureFormat::A8_UNORM;

			case DDS_DXGI_FORMAT_R1_UNORM:
				return TextureFormat::Unknown; // Currently not supported.

			case DDS_DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
				return TextureFormat::Unknown; // Currently not supported.
			case DDS_DXGI_FORMAT_R8G8_B8G8_UNORM:
				return TextureFormat::Unknown; // Currently not supported.
			case DDS_DXGI_FORMAT_G8R8_G8B8_UNORM:
				return TextureFormat::Unknown; // Currently not supported.

			case DDS_DXGI_FORMAT_BC1_TYPELESS:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_BC1_UNORM:
				return TextureFormat::BC1_UNORM;
			case DDS_DXGI_FORMAT_BC1_UNORM_SRGB:
				return TextureFormat::Unknown; // [TODO] Currently not supported.

			case DDS_DXGI_FORMAT_BC2_TYPELESS:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_BC2_UNORM:
				return TextureFormat::BC2_UNORM;
			case DDS_DXGI_FORMAT_BC2_UNORM_SRGB:
				return TextureFormat::Unknown; // [TODO] Currently not supported.

			case DDS_DXGI_FORMAT_BC3_TYPELESS:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_BC3_UNORM:
				return TextureFormat::BC3_UNORM;
			case DDS_DXGI_FORMAT_BC3_UNORM_SRGB:
				return TextureFormat::Unknown; // [TODO] Currently not supported.

			case DDS_DXGI_FORMAT_BC4_TYPELESS:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_BC4_UNORM:
				return TextureFormat::BC4_UNORM;
			case DDS_DXGI_FORMAT_BC4_SNORM:
				return TextureFormat::BC4_SNORM;

			case DDS_DXGI_FORMAT_BC5_TYPELESS:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_BC5_UNORM:
				return TextureFormat::BC5_UNORM;
			case DDS_DXGI_FORMAT_BC5_SNORM:
				return TextureFormat::BC5_SNORM;

			case DDS_DXGI_FORMAT_B5G6R5_UNORM:
				return TextureFormat::Unknown; // Currently not supported.
			case DDS_DXGI_FORMAT_B5G5R5A1_UNORM:
				return TextureFormat::Unknown; // Currently not suppotted.
			case DDS_DXGI_FORMAT_B8G8R8A8_UNORM:
				return TextureFormat::Unknown; // Currently not suppotted.
			case DDS_DXGI_FORMAT_B8G8R8X8_UNORM:
				return TextureFormat::Unknown; // Currently not suppotted.

			case DDS_DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
				return TextureFormat::Unknown; // Currently not suppotted.

			case DDS_DXGI_FORMAT_B8G8R8A8_TYPELESS:
				return TextureFormat::Unknown; // [TODO] Currently not supported.
			case DDS_DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
				return TextureFormat::Unknown; // [TODO] Currently not supported.
			case DDS_DXGI_FORMAT_B8G8R8X8_TYPELESS:
				return TextureFormat::Unknown; // [TODO] Currently not supported.
			case DDS_DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
				return TextureFormat::Unknown; // [TODO] Currently not supported.

			case DDS_DXGI_FORMAT_BC6H_TYPELESS:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_BC6H_UF16:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_BC6H_SF16:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_BC7_TYPELESS:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_BC7_UNORM:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_BC7_UNORM_SRGB:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_AYUV:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_Y410:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_Y416:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_NV12:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_P010:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_P016:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_420_OPAQUE:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_YUY2:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_Y210:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_Y216:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_NV11:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_AI44:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_IA44:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_P8:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_A8P8:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_B4G4R4A4_UNORM:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_P208:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_V208:
				return TextureFormat::Unknown;
			case DDS_DXGI_FORMAT_V408:
				return TextureFormat::Unknown;
		}

		sgeAssert(false);
		return TextureFormat::Unknown;
	}

#define DDS_MAKEFOURCC(c0, c1, c2, c3) (((c3) << 24) | ((c2) << 16) | ((c1) << 8) | (c0))
#define ISBITMASK(r, g, b, a) \
	(ddpf.dwRBitMask == r && ddpf.dwGBitMask == g && ddpf.dwBBitMask == b && ddpf.dwABitMask == a)

	TextureFormat::Enum GetTextureFormat(const DDS_PIXELFORMAT& ddpf)
	{
		if (ddpf.dwFlags & DDPF_RGB) {
			// Note that sRGB formats are written using the "DX10" extended header

			switch (ddpf.dwRGBBitCount) {
				case 32:
					if (ISBITMASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000)) {
						return TextureFormat::R8G8B8A8_UNORM; // DXGI_FORMAT_R8G8B8A8_UNORM;
					}

					if (ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000)) {
						return TextureFormat::R8G8B8A8_UNORM; // DXGI_FORMAT_B8G8R8A8_UNORM;
					}

					if (ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000)) {
						sgeAssert(false);
						return TextureFormat::Unknown; // [TODO] Currently not supported.
						                               // return TextureFormat::B8G8R8X8_UNORM;
					}

					// No DXGI format maps to ISBITMASK(0x000000ff,0x0000ff00,0x00ff0000,0x00000000) aka D3DFMT_X8B8G8R8

					// Note that many common DDS reader/writers (including D3DX) swap the
					// the RED/BLUE masks for 10:10:10:2 formats. We assume
					// below that the 'backwards' header mask is being used since it is most
					// likely written by D3DX. The more robust solution is to use the 'DX10'
					// header extension and specify the DXGI_FORMAT_R10G10B10A2_UNORM format directly

					// For 'correct' writers, this should be 0x000003ff,0x000ffc00,0x3ff00000 for RGB data
					if (ISBITMASK(0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000)) {
						return TextureFormat::R10G10B10A2_UNORM;
					}

					// No DXGI format maps to ISBITMASK(0x000003ff,0x000ffc00,0x3ff00000,0xc0000000) aka
					// D3DFMT_A2R10G10B10

					if (ISBITMASK(0x0000ffff, 0xffff0000, 0x00000000, 0x00000000)) {
						return TextureFormat::R16G16_UNORM;
					}

					if (ISBITMASK(0xffffffff, 0x00000000, 0x00000000, 0x00000000)) {
						// Only 32-bit color channel format in D3D9 was R32F
						return TextureFormat::R32_FLOAT; // D3DX writes this out as a FourCC of 114
					}
					break;

				case 24:
					// No 24bpp DXGI formats aka D3DFMT_R8G8B8
					break;

				case 16:
					if (ISBITMASK(0x7c00, 0x03e0, 0x001f, 0x8000)) {
						sgeAssert(false); // [TODO] Currently not supported.
						return TextureFormat::Unknown;
						// return TextureFormat::B5G5R5A1_UNORM;
					}
					if (ISBITMASK(0xf800, 0x07e0, 0x001f, 0x0000)) {
						sgeAssert(false); // [TODO] Currently not supported.
						return TextureFormat::Unknown;
						// return TextureFormat::B5G6R5_UNORM;
					}

					// No DXGI format maps to ISBITMASK(0x7c00,0x03e0,0x001f,0x0000) aka D3DFMT_X1R5G5B5

					if (ISBITMASK(0x0f00, 0x00f0, 0x000f, 0xf000)) {
						sgeAssert(false); // [TODO] Currently not supported.
						return TextureFormat::Unknown;
						// return TextureFormat::B4G4R4A4_UNORM;
					}

					// No DXGI format maps to ISBITMASK(0x0f00,0x00f0,0x000f,0x0000) aka D3DFMT_X4R4G4B4

					// No 3:3:2, 3:3:2:8, or paletted DXGI formats aka D3DFMT_A8R3G3B2, D3DFMT_R3G3B2, D3DFMT_P8,
					// D3DFMT_A8P8, etc.
					break;
			}
		}
		else if (ddpf.dwFlags & DDPF_LUMINANCE) {
			if (8 == ddpf.dwRGBBitCount) {
				if (ISBITMASK(0x000000ff, 0x00000000, 0x00000000, 0x00000000)) {
					return TextureFormat::R8_UNORM; // D3DX10/11 writes this out as DX10 extension
				}

				// No DXGI format maps to ISBITMASK(0x0f,0x00,0x00,0xf0) aka D3DFMT_A4L4
			}

			if (16 == ddpf.dwRGBBitCount) {
				if (ISBITMASK(0x0000ffff, 0x00000000, 0x00000000, 0x00000000)) {
					return TextureFormat::R16_UNORM; // D3DX10/11 writes this out as DX10 extension
				}
				if (ISBITMASK(0x000000ff, 0x00000000, 0x00000000, 0x0000ff00)) {
					return TextureFormat::R8G8_UNORM; // D3DX10/11 writes this out as DX10 extension
				}
			}
		}
		else if (ddpf.dwFlags & DDPF_ALPHA) {
			if (8 == ddpf.dwRGBBitCount) {
				return TextureFormat::A8_UNORM;
			}
		}
		else if (ddpf.dwFlags & DDPF_BUMPDUDV) {
			if (16 == ddpf.dwRGBBitCount) {
				if (ISBITMASK(0x00ff, 0xff00, 0x0000, 0x0000)) {
					return TextureFormat::R8G8_SNORM; // D3DX10/11 writes this out as DX10 extension
				}
			}

			if (32 == ddpf.dwRGBBitCount) {
				if (ISBITMASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000)) {
					return TextureFormat::R8G8B8A8_SNORM; // D3DX10/11 writes this out as DX10 extension
				}
				if (ISBITMASK(0x0000ffff, 0xffff0000, 0x00000000, 0x00000000)) {
					return TextureFormat::R16G16_SNORM; // D3DX10/11 writes this out as DX10 extension
				}

				// No DXGI format maps to ISBITMASK(0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000) aka
				// D3DFMT_A2W10V10U10
			}
		}
		else if (ddpf.dwFlags & DDPF_FOURCC) {
			if (DDS_MAKEFOURCC('D', 'X', 'T', '1') == ddpf.dwFourCC) {
				return TextureFormat::BC1_UNORM;
			}
			if (DDS_MAKEFOURCC('D', 'X', 'T', '3') == ddpf.dwFourCC) {
				return TextureFormat::BC2_UNORM;
			}
			if (DDS_MAKEFOURCC('D', 'X', 'T', '5') == ddpf.dwFourCC) {
				return TextureFormat::BC3_UNORM;
			}

			// While pre-multiplied alpha isn't directly supported by the DXGI formats,
			// they are basically the same as these BC formats so they can be mapped
			if (DDS_MAKEFOURCC('D', 'X', 'T', '2') == ddpf.dwFourCC) {
				return TextureFormat::BC2_UNORM;
			}
			if (DDS_MAKEFOURCC('D', 'X', 'T', '4') == ddpf.dwFourCC) {
				return TextureFormat::BC3_UNORM;
			}

			if (DDS_MAKEFOURCC('A', 'T', 'I', '1') == ddpf.dwFourCC) {
				return TextureFormat::BC4_UNORM;
			}
			if (DDS_MAKEFOURCC('B', 'C', '4', 'U') == ddpf.dwFourCC) {
				return TextureFormat::BC4_UNORM;
			}
			if (DDS_MAKEFOURCC('B', 'C', '4', 'S') == ddpf.dwFourCC) {
				return TextureFormat::BC4_SNORM;
			}

			if (DDS_MAKEFOURCC('A', 'T', 'I', '2') == ddpf.dwFourCC) {
				return TextureFormat::BC5_UNORM;
			}
			if (DDS_MAKEFOURCC('B', 'C', '5', 'U') == ddpf.dwFourCC) {
				return TextureFormat::BC5_UNORM;
			}
			if (DDS_MAKEFOURCC('B', 'C', '5', 'S') == ddpf.dwFourCC) {
				return TextureFormat::BC5_SNORM;
			}

			// BC6H and BC7 are written using the "DX10" extended header

			if (DDS_MAKEFOURCC('R', 'G', 'B', 'G') == ddpf.dwFourCC) {
				sgeAssert(false); // [TODO] Currently not supported.
				return TextureFormat::Unknown;
				// return TextureFormat::R8G8_B8G8_UNORM;
			}
			if (DDS_MAKEFOURCC('G', 'R', 'G', 'B') == ddpf.dwFourCC) {
				sgeAssert(false); // [TODO] Currently not supported.
				return TextureFormat::Unknown;
				// return TextureFormat::G8R8_G8B8_UNORM;
			}

			if (DDS_MAKEFOURCC('Y', 'U', 'Y', '2') == ddpf.dwFourCC) {
				sgeAssert(false); // [TODO] Currently not supported.
				return TextureFormat::Unknown;
				// return TextureFormat::YUY2;
			}

			// Check for D3DFORMAT enums being set here
			switch (ddpf.dwFourCC) {
				case 36: // D3DFMT_A16B16G16R16
					return TextureFormat::R16G16B16A16_UNORM;

				case 110: // D3DFMT_Q16W16V16U16
					return TextureFormat::R16G16B16A16_SNORM;

				case 111: // D3DFMT_R16F
					return TextureFormat::R16_FLOAT;

				case 112: // D3DFMT_G16R16F
					return TextureFormat::R16G16_FLOAT;

				case 113: // D3DFMT_A16B16G16R16F
					return TextureFormat::R16G16B16A16_FLOAT;

				case 114: // D3DFMT_R32F
					return TextureFormat::R32_FLOAT;

				case 115: // D3DFMT_G32R32F
					return TextureFormat::R32G32_FLOAT;

				case 116: // D3DFMT_A32B32G32R32F
					return TextureFormat::R32G32B32A32_FLOAT;
			}
		}

		return TextureFormat::Unknown;
	}

} // namespace


//---------------------------------------------------------------
// DDS loader implementation.
//---------------------------------------------------------------
bool DDSLoader::load(
    const char* inputData, const size_t inputDataSizeBytes, TextureDesc& desc, std::vector<TextureData>& initalData)
{
	m_ddsData = inputData;
	m_ddsDataSizeBytes = inputDataSizeBytes;
	m_ddsDataPointer = 0;

	// Read and check if the dds magic number is the same as expected...
	const uint32 ddsMagic_read = readNextAs<uint32>();
	if (ddsMagic_read != DDS_MAGIC_NUMBER) {
		return false;
	}

	// Read the dds file header.
	const DDS_HEADER dds_header = readNextAs<DDS_HEADER>();

	// Read the DXT10 header (if any).
	// "If the DDS_PIXELFORMAT dwFlags is set to DDPF_FOURCC and dwFourCC is set to "DX10"
	// an additional DDS_HEADER_DXT10 structure will be present..."
	const bool hasDXT10Hheader =
	    (dds_header.dwFlags & DDPF_FOURCC) && (dds_header.ddspf.dwFourCC == DDS_MAKEFOURCC('D', 'X', '1', '0'));
	const DDS_HEADER_DXT10 dxt10ext = (hasDXT10Hheader) ? readNextAs<DDS_HEADER_DXT10>() : DDS_HEADER_DXT10();

	// The texture description
	TextureFormat::Enum textureFormat = TextureFormat::Unknown;
	const int mipCount = (dds_header.dwMipMapCount != 0) ? dds_header.dwMipMapCount : 1;
	int arraySize = 1; // Actually the nuber of surfaces(Example is Cube textures, they have 6 surfaces and in D3D11
	                   // they are represended with Texture2DArray).
	int width = dds_header.dwWidth;
	int height = dds_header.dwHeight;
	int depth = dds_header.dwDepth;
	bool isCubeTexture = false;
	int textureDimensionIdx = 0; // 1 for 1D, 2 for 2D, 3 for 3D

	// If the DXT10 header is valid use it instead of the default one.
	if (hasDXT10Hheader) {
		// Load the array size of the texture.
		arraySize = dxt10ext.arraySize;
		if (arraySize == 0) {
			// This is what DirectXTK does in that case...
			sgeAssert(false);
			return false;
		}

		// Load the format the the texture format and check if it is supported.
		textureFormat = DDS_DXGI_FORMAT_to_TextureFormat(dxt10ext.dxgiFormat);
		if (textureFormat == TextureFormat::Unknown) {
			// This is what DirectXTK does in that case...
			sgeAssert(false);
			return false;
		}

		// Check the resource dimension (1D, 2D, 3D, CUBE texture ect.)
		switch (dxt10ext.resourceDimension) {
			// 1D Textures
			case DDS_RESOURCE_DIMENSION_TEXTURE1D: {
				textureDimensionIdx = 1;
				sgeAssert((dds_header.dwFlags & DDSD_HEIGHT) && height != 1);
				height = depth = 1;

			} break;

			// 2D Textures or Cubemaps
			case DDS_RESOURCE_DIMENSION_TEXTURE2D: {
				textureDimensionIdx = 2;
				if (dxt10ext.miscFlag & DDS_D3D11_RESOURCE_MISC_TEXTURECUBE) {
					arraySize *= 6;
					isCubeTexture = true;
				}
			} break;

			// 3D Textures
			case DDS_RESOURCE_DIMENSION_TEXTURE3D: {
				textureDimensionIdx = 3;
				sgeAssert(dds_header.dwFlags & DDSD_DEPTH);
			} break;

			// Unknown.
			default: {
				sgeAssert(false);
				return false;
			}
		}
	}
	// No DX10 extended header.
	else {
		// [TODO] there colud be more textures contained in the file in that case not only 2d textures.
		textureDimensionIdx = 2;

		// Load the tecture format.
		textureFormat = GetTextureFormat(dds_header.ddspf);
		if (textureFormat == TextureFormat::Unknown) {
			// This is what DirectXTK does in that case...
			sgeAssert(false);
			return false;
		}

		if (dds_header.dwCaps2 & DDSCAPS2_CUBEMAP) {
			sgeAssert(dds_header.dwCaps2 & DDSCAPS2_CUBEMAP_ALL);

			depth = 1; // This is what DirectXTK does...
			arraySize = 6;
			isCubeTexture = true;
		}
	}

	// A bit of checks...
	sgeAssert(TextureFormat::GetSizeBits(textureFormat) != 0);

	// Save the pointers to the place where the actual texture data is stored.
	const char* const pTextureDataBegin = getDataPointer();
	const size_t textureDataSize = getRemainingBytesCount();

	// Compute the total amount of resourcces(a single mip level) are in the whole texture.
	const int numResources = arraySize * mipCount;
	initalData.reserve(numResources);

	// Generate the inial data for the texture.
	const char* resourceData = pTextureDataBegin;
	for (int iArr = 0; iArr < arraySize; iArr++) {
		int mip_width = width;
		int mip_height = height;
		int mip_depth = depth;

		for (int iMip = 0; iMip < mipCount; ++iMip) {
			const SurfaceInfo surfaceInfo = getSurfaceInfo(mip_width, mip_height, textureFormat);

			// Generate the texture inial data for this mip.
			TextureData texData;

			texData.data = resourceData;
			texData.rowByteSize = surfaceInfo.rowSizeBytes;
			texData.sliceByteSize = surfaceInfo.sliceSizeBytes;

			initalData.push_back(texData);

			// Offset the reosurce pointer to the next one.
			resourceData += surfaceInfo.sliceSizeBytes * mip_depth;

			// Check if we overflow the buffer somehow (should never happen).
			const size_t readDataSoFarBytes = resourceData - pTextureDataBegin;
			sgeAssert(readDataSoFarBytes <= textureDataSize);

			// Compute the texture sizes for the next mip level.
			mip_width = dds_max(mip_width / 2, 1);
			mip_height = dds_max(mip_height / 2, 1);
			mip_depth = dds_max(mip_depth / 2, 1);
		}
	}

	// Generate the texture desc.
	if (textureDimensionIdx == 1) {
		desc = TextureDesc();
		desc.format = textureFormat;
		desc.textureType = UniformType::Texture1D;
		desc.texture1D = Texture1DDesc(width, mipCount, arraySize);
	}
	else if (textureDimensionIdx == 2) {
		desc = TextureDesc();
		desc.format = textureFormat;

		if (isCubeTexture == false) {
			desc.textureType = UniformType::Texture2D;
			desc.texture2D = Texture2DDesc(width, height, mipCount, arraySize);
		}
		else {
			// Since cube textures are just 6 2D textures we assume this.
			sgeAssert(arraySize % 6 == 0);

			desc.textureType = UniformType::TextureCube;
			desc.textureCube = TextureCubeDesc(width, height, mipCount, arraySize / 6);
		}
	}
	else if (textureDimensionIdx == 3) {
		desc = TextureDesc();
		desc.format = textureFormat;
		desc.textureType = UniformType::Texture3D;
		desc.texture3D = Texture3DDesc(width, height, depth, mipCount);
	}
	else {
		// Should never happen.
		sgeAssert(false);
		return false;
	}

	return true;
}

DDSLoader::SurfaceInfo
    DDSLoader::getSurfaceInfo(const int width, const int height, const TextureFormat::Enum textureFormat)
{
	const size_t bpp = TextureFormat::GetSizeBits(textureFormat);
	const bool isBC = TextureFormat::IsBC(textureFormat);

	sgeAssert(bpp != 0);

	if (isBC) {
		const size_t numBlocksWide = (width > 0) ? dds_max<size_t>(1, (width + 3) / 4) : 0;
		const size_t numBlocksHigh = (height > 0) ? dds_max<size_t>(1, (height + 3) / 4) : 0;

		SurfaceInfo retval;

		// TODO: so bpp is BITS per pixel and we magically end up with bytes?
		retval.rowSizeBytes = numBlocksWide * bpp;
		retval.sliceSizeBytes = retval.rowSizeBytes * numBlocksHigh;

		return retval;
	}

	SurfaceInfo retval;

	retval.rowSizeBytes = (width * bpp + 7) / 8;
	retval.sliceSizeBytes = retval.rowSizeBytes * height;

	return retval;
}

} // namespace sge
