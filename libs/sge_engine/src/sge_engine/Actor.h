#pragma once

#include "sge_utils/math/Box3f.h"
#include "sge_utils/math/transform.h"

#include "sge_engine/GameObject.h"

namespace sge {
struct Trait;
struct GameInspector;
struct InspectorCmd;
struct SelectedItem;

/// Actor is a GameObject that has a transform in the world.
/// All game object types that have that property should inherit this.
struct SGE_ENGINE_API Actor : public GameObject {
	Actor() = default;
	virtual ~Actor() = default;

	/// Returns the transform of the actor in world space.
	const transf3d& getTransform() const { return m_logicTransform; }

	/// Returns the transform of the actor in world space as a matrix.
	/// The computation is cached in roder to compute it every time we need it.
	const mat4f& getTransformMtx() const;

	/// Shorthand for getting the position of the actor in world space.
	const vec3f& getPosition() const { return m_logicTransform.p; }
	const quatf& getOrientation() const { return m_logicTransform.r; }

	/// Shorthand for retrieving the cardinal directions of an axis in world space according to this actor's transform.
	const vec3f getDirX() const { return getTransformMtx().c0.xyz(); }
	const vec3f getDirY() const { return getTransformMtx().c1.xyz(); }
	const vec3f getDirZ() const { return getTransformMtx().c2.xyz(); }

	/// Changes the transform of the actor and its assigned main rigid body (if any).
	/// @param [in] killVelocity set it to true if you want the rigid body of the actor (if any) to not have any velocty.
	void setTransform(const transf3d& transform, bool killVelocity = true);

	/// Sets the local transform of the actor according to its parent objects.
	void setLocalTransform(const transf3d& localTransform, bool killVelocity = true);

	/// A shorthand for changing the position of the actor.
	void setPosition(const vec3f& p, bool killVelocity = true);

	/// A shorthand for changing the orientation of the actor.
	void setOrientation(const quatf& r, bool killVelocity = true);

	/// Called after the physics simulation has ended and before update().
	/// It is used so the physics engine can change the transformation of the actor without looping back to the physics engine.
	void setTransformEx(const transf3d& transform, bool killVelocity, bool recomputeBinding, bool shouldChangeRigidBodyTransform);

	/// Returns the bounding box in object space. The box may be empty if not applicable.
	/// This is not intended for physics or any game logic.
	/// This should be used for the editor and the rendering.
	virtual Box3f getBBoxOS() const = 0;

	/// These functions tells the editor that the actor provides sub-objects that can be edited in the viewport.
	/// For example the Splines have control points that we can move via the transform tools.
	virtual int getNumItemsInMode(EditMode const mode) const;
	virtual bool getItemTransform(transf3d& result, EditMode const mode, int UNUSED(itemIndex));
	virtual void setItemTransform(EditMode const mode, int UNUSED(itemIndex), const transf3d& tr);
	virtual InspectorCmd*
	    generateDeleteItemCmd(GameInspector* inspector, const SelectedItem* items, int numItems, bool ifActorModeShouldDeleteActorsUnder);
	virtual InspectorCmd* generateItemSetTransformCmd(
	    GameInspector* inspector, EditMode const mode, int itemIndex, const transf3d& initalTrasform, const transf3d& newTransform);

  public:
	/// The transform of the actor in world space. this is the transform that
	/// specifies the actual position of the actor.
	transf3d m_logicTransform = transf3d::getIdentity();
	/// The transform that specifies where, according to its parent actor,
	/// is this actor positioned. This transformation should be used when
	/// the parent actor moves and we want to compute how this actor (as a child)
	/// should be moved as well.
	transf3d m_bindingToParentTransform = transf3d::getIdentity();

	/// A lot of rendering logic needs the transform of the actor as a matrix.
	/// Covertinf form transf3d to matrix could be slow so we use this pair of values to get it cached.
	mutable mat4f m_trasformAsMtx;
	mutable bool m_isTrasformAsMtxValid = false;
};

} // namespace sge
