#pragma once

#include "sge_core/sgecore_api.h"
#include "sge_renderer/renderer/renderer.h"

#include "sge_core/QuickDraw/SolidDrawer.h"
#include "sge_core/QuickDraw/TextRender.h"
#include "sge_core/QuickDraw/TextureDrawer.h"
#include "sge_core/QuickDraw/WireframeDrawer.h"

namespace sge {

/// QuickDraw is a wrapper around the rendering API that provides functions
/// for quickly drawing simple shapes, wireframes, texture, text and so on.
struct SGE_CORE_API QuickDraw : public NoCopy {
	QuickDraw() = default;

	bool initialize(SGEContext* sgecon);

	TextureDrawer& getTextureDrawer() { return textureDrawer; }
	TextRenderer& getTextRenderer() { return textRenderer; }
	WireframeDrawer& getWire() { return wireframeDrawer; }
	SolidDrawer& getSolid() { return solidDrawer; }

  private:
	TextureDrawer textureDrawer;
	WireframeDrawer wireframeDrawer;
	SolidDrawer solidDrawer;
	TextRenderer textRenderer;
};

} // namespace sge
