#include "Font.h"
#include "sge_utils/io/FileStream.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace sge {

void QuickFont::destroy()
{
	texture.Release();
}

bool QuickFont::create(SGEDevice* sgedev, const char* const ttfFilename, float heightPixels)
{
	destroy();

	// Read the ttf file data.
	FileReadStream frs;

	std::vector<char> ttfFileData;
	if (FileReadStream::readFile(ttfFilename, ttfFileData)) {
		return createFromFileData(sgedev, (unsigned char*)ttfFileData.data(), ttfFileData.size(), heightPixels);
	}

	return false;
}

bool QuickFont::createFromFileData(
    SGEDevice* sgedev, const unsigned char* const ttfFileData, size_t UNUSED(ttfFileDataSizeBytes), float heightPixels)
{
	// Cache the font height.
	height = heightPixels;

	// Bake the font image.
	const int kTextureDim = 2048; // the size(in every axis) of the baked font texture.
	std::vector<unsigned char> textureData;
	textureData.resize(kTextureDim * kTextureDim);

	// [TODO] Find a way to pass a correct value for the "offset" argument
	// currently this is wrong and may crash!
	if (stbtt_BakeFontBitmap(
	        (unsigned char*)ttfFileData,
	        0,
	        (float)heightPixels,
	        textureData.data(),
	        kTextureDim,
	        kTextureDim,
	        0,
	        255,
	        cdata) < 0) {
		destroy();
		return false;
	}

	for (int iCh = 0; iCh < SGE_ARRSZ(cdata); ++iCh) {
		const stbtt_bakedchar ch = cdata[iCh];

		if (fabsf(ch.yoff) > maxTopToBaseline) {
			maxTopToBaseline = fabsf(ch.yoff);
		}

		float baselineToBottom = float(height) - fabsf(ch.yoff);
		if (baselineToBottom > maxBaselineToBottom) {
			maxBaselineToBottom = baselineToBottom;
		}
	}

	// Create a the texture for the font.
	TextureDesc td;
	td.textureType = UniformType::Texture2D;
	td.format = TextureFormat::R8_UNORM;
	td.usage = TextureUsage::ImmutableResource;
	td.texture2D = Texture2DDesc(kTextureDim, kTextureDim);

	TextureData texData;
	texData.data = textureData.data();
	texData.rowByteSize = kTextureDim * 1;

	texture = sgedev->requestResource<Texture>();
	if (!texture->create(td, &texData)) {
		destroy();
		return false;
	}

	return true;
}



} // namespace sge
