#include "Actor.h"
#include "GameWorld.h"
#include "InspectorCmds.h"

#include "sge_engine/traits/TraitRigidBody.h"

namespace sge {
//--------------------------------------------------------------------
// struct Actor
//--------------------------------------------------------------------
// clang-format off
ReflAddTypeId(Actor, 20'03'01'0022);

ReflBlock()
{
	ReflAddType(Actor)
		ReflInherits(Actor, GameObject)
		ReflMember(Actor, m_logicTransform)
		ReflMember(Actor, m_bindingToParentTransform).addMemberFlag(MFF_NonEditable)
	;
}
// clang-format on

const mat4f& Actor::getTransformMtx() const
{
	if (!m_isTrasformAsMtxValid) {
		m_isTrasformAsMtxValid = true;
		m_trasformAsMtx = m_logicTransform.toMatrix();
	}
	return m_trasformAsMtx;
}

//// Physics engine can't modify the scaling of the object. So this is a faster alternative for that case.
// void Actor::setTransformFromPhysicsInternal(const vec3f& p, const quatf& r) {
//	transf3d newTransf;
//	newTransf.p = p;
//	newTransf.r = r;
//	newTransf.s = getTransform().s;
//
//	setTransformEx(newTransf, false, false, false);
//}

void Actor::setTransform(const transf3d& transform, bool killVelocity)
{
	// Recompute binding is making alot of trouble, if the child object has
	// self moving logic e do want to recompute binding when we call
	// setTransform.
	// When then object is static we usually do not call this at all, except in the editor.
	// I do not know... some debugging is needed do pick the best behaviour or we need to make this thing explicit.
	bool recomputeBinding = false;
	setTransformEx(transform, killVelocity, recomputeBinding, true);
}

void Actor::setLocalTransform(const transf3d& localTransform, bool killVelocity)
{
	const Actor* parentActor = getWorld()->getActorById(getWorld()->getParentId(getId()));
	if (parentActor) {
		// TODO: should it be done with binding magic?
		transf3d newWorldTransform = parentActor->getTransform() * localTransform;
		setTransformEx(newWorldTransform, killVelocity, true, true);
	}
	else {
		setTransformEx(localTransform, killVelocity, true, true);
	}
}

void Actor::setPosition(const vec3f& p, bool killVelocity)
{
	transf3d newTr = getTransform();
	newTr.p = p;
	setTransform(newTr, killVelocity);
}

void Actor::setOrientation(const quatf& r, bool killVelocity)
{
	transf3d newTr = getTransform();
	newTr.r = r;
	setTransform(newTr, killVelocity);
}

void Actor::setTransformEx(
    const transf3d& newTransform, bool killVelocity, bool recomputeBinding, bool shouldChangeRigidBodyTransform)
{
	transf3d oldTransform = m_logicTransform;
	m_isTrasformAsMtxValid = false;
	m_logicTransform = newTransform;

	if (shouldChangeRigidBodyTransform) {
		TraitRigidBody* const traitRB = getTrait<TraitRigidBody>(this);
		if (traitRB) {
			traitRB->setTrasnform(newTransform, killVelocity);
		}
	}

	if (recomputeBinding) {
		const Actor* parentActor = getWorld()->getActorById(getWorld()->getParentId(getId()));
		if (parentActor != nullptr) {
			const transf3d parentTransform = parentActor->getTransform();

			// If the parent has any zero scaling, do not recompute the binding transform as it will not be recoverable
			// as the multiplication by zero will destroy any scaling and translation.
			const bool hasAnyZeroScaling =
			    isEpsZero(parentTransform.s.x) || isEpsZero(parentTransform.s.y) || isEpsZero(parentTransform.s.z);
			if (hasAnyZeroScaling == false) {
				m_bindingToParentTransform = newTransform.computeBindingTransform(parentTransform);
			}
		}
	}

	const vector_set<ObjectId>* pAllChildren = getWorld()->getChildensOf(getId());
	if (pAllChildren != nullptr) {
		sgeAssert(pAllChildren->size() != 0); // The pointer should be null in that case!
		for (int t = 0; t < pAllChildren->size(); ++t) {
			Actor* const child = getWorld()->getActorById(pAllChildren->data()[t]);
			if (child) {
				transf3d childNewTransformWS =
				    transf3d::applyBindingTransform(child->m_bindingToParentTransform, m_logicTransform);
				child->setTransformEx(childNewTransformWS, killVelocity, recomputeBinding, true);
			}
		}
	}
}


/// This function tells the editor that the actor provides sub-objects that can be edited in the viewport.
/// For example the Splines have control points that we can move via the transform tools.

int Actor::getNumItemsInMode(EditMode const mode) const
{
	if (mode == editMode_actors)
		return 1;
	return 0;
}

bool Actor::getItemTransform(transf3d& result, EditMode const mode, int UNUSED(itemIndex))
{
	if (mode == editMode_actors) {
		result = getTransform();
		return true;
	}

	// You probably want to implement this?
	sgeAssert(false);
	result = getTransform();

	return false;
}


// TODO: Should perform some checks when we are in actor mode>

void Actor::setItemTransform(EditMode const mode, int UNUSED(itemIndex), const transf3d& tr)
{
	if (mode == editMode_actors) {
		setTransformEx(tr, true, true, true);
		return;
	}

	// You probably want to implement this?
	sgeAssert(false);
}

//--------------------------------------------------------------------
// Actor edit mode stuff.
//--------------------------------------------------------------------
InspectorCmd* Actor::generateDeleteItemCmd(
    GameInspector* UNUSED(inspector),
    const SelectedItem* items,
    int numItems,
    bool UNUSED(ifActorModeShouldDeleteActorsUnder))
{
	if (numItems == 1 && items[0].editMode == editMode_actors) {
		CmdObjectDeletion* const cmdDel = new CmdObjectDeletion;
		vector_set<ObjectId> objToDel;
		objToDel.insert(getId());
		cmdDel->setupDeletion(*getWorld(), objToDel);

		return cmdDel;
	}

	return nullptr;
}

InspectorCmd* Actor::generateItemSetTransformCmd(
    GameInspector* UNUSED(inspector),
    EditMode const mode,
    int UNUSED(itemIndex),
    const transf3d& initalTrasform,
    const transf3d& newTransform)
{
	if (mode == editMode_actors) {
		CmdMemberChange* cmd = new CmdMemberChange;
		cmd->setupLogicTransformChange(*this, initalTrasform, newTransform);
		return cmd;
	}

	return nullptr;
}

} // namespace sge
