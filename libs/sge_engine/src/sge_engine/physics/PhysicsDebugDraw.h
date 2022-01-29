#pragma once

#include "BulletHelper.h"
#include "sge_engine/sge_engine_api.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/math/mat4.h"

SGE_NO_WARN_BEGIN
#include <LinearMath/btIDebugDraw.h>
SGE_NO_WARN_END

namespace sge {

struct QuickDraw;
struct RenderDestination;

struct SGE_ENGINE_API BulletPhysicsDebugDraw : public btIDebugDraw {
	BulletPhysicsDebugDraw() = default;

	int m_debugMode = DBG_DrawWireframe | DBG_DrawContactPoints | DBG_DrawNormals | DBG_EnableCCD | DBG_DrawConstraints | DBG_DrawFrames |
	                  DBG_DrawContactPoints;
	mat4f m_projView = mat4f::getIdentity();
	QuickDraw* m_quickDraw = nullptr;

	RenderDestination cachedRdest;

  public:
	/// From Bullet Physics docs:
	/// bt*World::debugDraw* should be called between these two.
	void preDebugDraw(const mat4f& projView, QuickDraw* const debugDraw, const RenderDestination& rdest);
	void postDebugDraw();

	void setDebugMode(int debugMode) final
	{
		m_debugMode = debugMode;
	}

	int getDebugMode() const final
	{
		return m_debugMode;
	}

	void reportErrorWarning(const char* warningString) final;

	void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) final;

	void drawContactPoint(const btVector3& UNUSED(PointOnB),
	                      const btVector3& UNUSED(normalOnB),
	                      btScalar UNUSED(distance),
	                      int UNUSED(lifeTime),
	                      const btVector3& UNUSED(color)) final
	{
	}

	void draw3dText(const btVector3& UNUSED(location), const char* UNUSED(textString)) final
	{
	}
};

} // namespace sge
