#include "TraitRigidBody.h"
#include "sge_core/AssetLibrary/AssetLibrary.h"
#include "sge_core/ICore.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/RigidBodyFromModel.h"

namespace sge {

//-----------------------------------------------------------
// TraitRigidBody
//-----------------------------------------------------------
ReflAddTypeId(TraitRigidBody, 20'03'06'0001);
TraitRigidBody::~TraitRigidBody()
{
	if (m_rigidBody.isValid()) {
		this->destroyRigidBody();
	}
}

void TraitRigidBody::destroyRigidBody()
{
	if (!m_rigidBody.isValid()) {
		return;
	}

	// TODO; This needs to happen inside the rigid body, not here.
	getWorld()->removeRigidBodyManifold(&m_rigidBody);
	getWorld()->physicsWorld.removePhysicsObject(m_rigidBody);

	this->m_rigidBody.destroy();
}

bool TraitRigidBody::createBasedOnModel(const EvaluatedModel& eval, float mass, bool noResponse, bool addToWorldNow)
{
	destroyRigidBody();

	std::vector<CollsionShapeDesc> shapeDesc;
	if (addCollisionShapeBasedOnModel(shapeDesc, eval)) {
		getRigidBody()->create(getActor(), shapeDesc.data(), int(shapeDesc.size()), mass, noResponse);
		getRigidBody()->setTransformAndScaling(getActor()->getTransform(), true);

		if (addToWorldNow) {
			GameWorld* const world = getWorld();
			world->physicsWorld.addPhysicsObject(*getRigidBody());
		}

		return true;
	}

	destroyRigidBody();
	return false;
}

bool TraitRigidBody::createBasedOnModel(const char* modelPath, float mass, bool noResponse, bool addToWorldNow)
{
	AssetPtr modelAsset = getCore()->getAssetLib()->getAssetFromFile(modelPath);
	if (!isAssetLoaded(modelAsset, assetIface_model3d)) {
		sgeAssert(false && "Failed to load an asset.");
		return false;
	}

	return createBasedOnModel(
	    getLoadedAssetIface<AssetIface_Model3D>(modelAsset)->getStaticEval(), mass, noResponse, addToWorldNow);
}

bool TraitRigidBody::isInWorld() const
{
	if (!m_rigidBody.isValid()) {
		return false;
	}

	return m_rigidBody.getBulletRigidBody()->isInWorld();
}

void TraitRigidBody::onPlayStateChanged(bool const isStartingToPlay)
{
	if (!m_rigidBody.isValid())
		return;

	if (isStartingToPlay) {
		addToWorld();
	}
	else {
		removeFromWorld();
	}
}

void TraitRigidBody::setTrasnform(const transf3d& transf, bool killVelocity)
{
	if (m_rigidBody.isValid()) {
		m_rigidBody.setTransformAndScaling(transf, killVelocity);

		if (m_rigidBody.getBulletRigidBody()) {
			if (m_rigidBody.getBulletRigidBody()->isInWorld() &&
			    m_rigidBody.getBulletRigidBody()->getActivationState() == ISLAND_SLEEPING) {
				getWorld()->physicsWorld.dynamicsWorld->updateSingleAabb(m_rigidBody.getBulletRigidBody());
			}
		}
		else if (m_rigidBody.getBulletGhostObject()) {
			// getWorld()->physicsWorld.dynamicsWorld->updateSingleAabb(m_rigidBody.getBulletGhostObject());
		}
	}
}

void TraitRigidBody::addToWorld()
{
	if (isInWorld() == false && m_rigidBody.isValid()) {
		getWorld()->physicsWorld.addPhysicsObject(this->m_rigidBody);
	}
}

void TraitRigidBody::removeFromWorld()
{
	if (isInWorld()) {
		getWorld()->physicsWorld.removePhysicsObject(this->m_rigidBody);
	}
}

} // namespace sge
