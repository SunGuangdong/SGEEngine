#include "QuickDraw.h"
#include "sge_core/ICore.h"

namespace sge {

bool QuickDraw::initialize(SGEContext* sgecon)
{
	sgeAssert(sgecon);

	textureDrawer.create(sgecon);
	textRenderer.create(sgecon);
	wireframeDrawer.create(sgecon);
	solidDrawer.create(sgecon);

	return true;
}

} // namespace sge
