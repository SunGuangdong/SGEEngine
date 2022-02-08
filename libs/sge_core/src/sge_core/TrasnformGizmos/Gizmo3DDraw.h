#pragma once

#include "sge_core/sgecore_api.h"
#include "sge_utils/math/mat4f.h"

namespace sge {

struct RenderDestination;
struct Gizmo3D;
struct Gizmo3DTranslation;
struct Gizmo3DScale;
struct Gizmo3DRotation;
struct Gizmo3DScaleVolume;

void SGE_CORE_API drawGizmo(const RenderDestination& rdest, const Gizmo3D& gizmo, const mat4f& projView);
void SGE_CORE_API drawTranslationGizmo(const RenderDestination& rdest, const Gizmo3DTranslation& gizmo, const mat4f& projView);
void SGE_CORE_API drawRotationGizmo(const RenderDestination& rdest, const Gizmo3DRotation& gizmo, const mat4f& projView);
void SGE_CORE_API drawScaleGizmo(const RenderDestination& rdest, const Gizmo3DScale& gizmo, const mat4f& projView);
void SGE_CORE_API drawScaleVolumeGizmo(const RenderDestination& rdest, const Gizmo3DScaleVolume& gizmo, const mat4f& projView);

} // namespace sge
