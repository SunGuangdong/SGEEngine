#include "sge_core/typelib/typeLib.h"
#include "sge_engine/GameWorld.h"
#include "sge_utils/text/format.h"

#include "Actor.h"

namespace sge {



// clang-format off
ReflAddTypeId(ObjectId, 20'03'06'0005);
ReflAddTypeId(GameObject, 20'03'01'0011);
ReflAddTypeId(std::vector<ObjectId>, 20'03'07'0012);

ReflBlock()
{
	ReflAddType(ObjectId)
		ReflMember(ObjectId, id);

	ReflAddType(std::vector<ObjectId>);

	ReflAddType(GameObject)
		ReflMember(GameObject, m_id)
			.addMemberFlag(MFF_PrefabDontCopy)
			.addMemberFlag(MFF_NonEditable) // CAUTION: the id is actually saveable, but it is handeled diferently!
		ReflMember(GameObject, m_displayName)
			.addMemberFlag(MFF_PrefabDontCopy)
	;
}
// clang-format on

//--------------------------------------------------------------------
// GameObject
//--------------------------------------------------------------------
void GameObject::private_GameWorld_performInitialization(
    GameWorld* const world, const ObjectId id, const TypeId typeId, std::string displayName)
{
	m_id = id;
	m_type = typeId;
	m_world = world;
	m_displayName = std::move(displayName);
}

bool GameObject::isActor() const
{
	return getActor() != nullptr;
}

Actor* GameObject::getActor()
{
	return dynamic_cast<Actor*>(this);
}

const Actor* GameObject::getActor() const
{
	return dynamic_cast<const Actor*>(this);
}

void GameObject::composeDebugDisplayName(std::string& result) const
{
	string_format(result, "%s[id=%d]", m_displayName.c_str(), m_id.id);
}

void GameObject::onPlayStateChanged(bool const isStartingToPlay)
{
	for (const TraitRegistration& trait : m_traits) {
		sgeAssert(trait.pointerToTrait != nullptr);

		if (trait.pointerToTrait) {
			trait.pointerToTrait->onPlayStateChanged(isStartingToPlay);
		}
	}
}

void GameObject::registerTrait(Trait& trait)
{
	Trait* const pTrait = &trait;
	TypeId const family = pTrait->getFamily();

	// There shouldn't be a trait with the same family registered.
	// Check if this is true.
	for (int t = 0; t < int(m_traits.size()); ++t) {
		if (m_traits[t].traitFamilyType == family) {
			sgeAssertFalse(
			    "It seems that the trait of that family is already registered. Having multiple traits of the same "
			    "family is not suported. "
			    "The current registration will get discarded.");
			return;
		}
	}

	TraitRegistration traitReg = {family, pTrait};
	m_traits.push_back(traitReg);
	pTrait->private_GameObject_register(this);
}

//--------------------------------------------------------------------
// Trait
//--------------------------------------------------------------------
Actor* Trait::getActor()
{
	Actor* actor = dynamic_cast<Actor*>(getObject());
	return actor;
}

const Actor* Trait::getActor() const
{
	const Actor* actor = dynamic_cast<const Actor*>(getObject());
	return actor;
}

SelectedItemDirect SelectedItemDirect::fromSelectedItem(const SelectedItem& item, GameWorld& world)
{
	SelectedItemDirect result;
	result.editMode = item.editMode;
	result.index = item.index;
	result.gameObject = world.getObjectById(item.objectId);

	return result;
}

} // namespace sge
