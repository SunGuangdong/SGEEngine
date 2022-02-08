#pragma once

#include "sge_core/sgecore_api.h"
#include "sge_utils/math/MultiCurve2D.h"
#include "sge_utils/math/vec2f.h"

namespace sge {
SGE_CORE_API void MultiCurve2DEditor(const char* const widgetName,
                                     MultiCurve2D& m_fn,
                                     vec2f widgetSize = vec2f(-1.f, -1.f),
                                     bool isThisExpandedOfAnother = false);
} // namespace sge
