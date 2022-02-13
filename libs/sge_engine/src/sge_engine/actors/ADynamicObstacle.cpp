#pragma once

#include "ADynamicObstacle.h"
#include "sge_core/ICore.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/RigidBodyFromModel.h"

namespace sge {
//-----------------------------------------------------
// ADynamicObstacle
//-----------------------------------------------------
// clang-format off
ReflAddTypeId(ADynamicObstacle, 20'03'02'0007);
ReflBlock() {
	ReflAddActor(ADynamicObstacle) 
		ReflMember(ADynamicObstacle, m_traitModel)
	    ReflMember(ADynamicObstacle, m_rbConfig)
	;
}
// clang-format on

void ADynamicObstacle::create()
{
	m_traitModel.addModel("assets/editor/models/box.mdl");

	registerTrait(m_traitRB);
	registerTrait(m_traitModel);
}

void ADynamicObstacle::onPlayStateChanged(bool const isStartingToPlay)
{
	Actor::onPlayStateChanged(isStartingToPlay);

	if (isStartingToPlay) {
		if (m_traitModel.postUpdate()) {
			m_rbConfig.apply(*this, true);
		}
	}
}

void ADynamicObstacle::onDuplocationComplete()
{
	m_traitModel.invalidateCachedAssets();
}

void ADynamicObstacle::update(const GameUpdateSets& UNUSED(u))
{
	// Update the rigid body config if the attached model changes or if the model
	// for rigid body has changed as these could affect the rigid body collition shape.
	if (m_traitModel.postUpdate() || m_rbConfig.m_sourceModel.update()) {
		m_rbConfig.apply(*this, true);
	}
}

Box3f ADynamicObstacle::getBBoxOS() const
{
	return m_traitModel.getBBoxOS();
}

} // namespace sge
