#pragma once

#include "sge_core/sgecore_api.h"
#include "sge_renderer/renderer/renderer.h"

#include <stb_truetype.h>

namespace sge {

/// QuickFont represents a font rasterized to a texture.
/// To render this font you can use TextRenderer2D.
struct SGE_CORE_API QuickFont {
  public:
	QuickFont() = default;
	~QuickFont() { destroy(); }

	void destroy();

	bool create(SGEDevice* sgedev, const char* const ttfFilename, float heightPixels);

	bool createFromFileData(
	    SGEDevice* sgedev, const unsigned char* const ttfFileData, size_t ttfFileDataSizeBytes, float heightPixels);

	float getScalingForHeight(float targetHeightPixels) const { return height / targetHeightPixels; }

	/// Every value here is in pixels.
	/// Explanation: https://www.microsoft.com/typography/otspec/TTCH01.htm
	///
	/// But in short, here is the letter 'q':
	///--------------------------------------
	///                                    |
	///----QQQ----------------------       |
	///   Q   Q    |           |           |
	///   Q   Q  Ascent      Body          |
	///   Q   Q    |           |           EM
	///----QQQQ--------        |           |
	///       Q  Descent       |           |
	///       Q---------------------       |
	///                                    |
	///--------------------------------------
	///
	///  "EM" square typically are those of the full body height of a font
	/// plus some extra spacing to prevent lines of text
	/// from colliding when typeset without extra leading.
	float height = 0.f; // The height of the baked font (EM in the picture above).
	float maxTopToBaseline = 0.f;
	float maxBaselineToBottom = 0.f;
	stbtt_bakedchar cdata[256];
	GpuHandle<Texture> texture;
};


} // namespace sge
