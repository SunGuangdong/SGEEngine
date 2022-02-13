#include "sge_engine/Actor.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/traits/TraitModel.h"
#include "sge_engine/traits/TraitRigidBody.h"
#include "sge_engine/typelibHelper.h"

namespace sge {

struct DummyActor : public Actor {
	TraitRigidBody ttRigidbody;
	TraitModel ttModel;

	virtual AABox3f getBBoxOS() const { return AABox3f(); }

	void create()
	{
		registerTrait(ttRigidbody);
		registerTrait(ttModel);

		ttModel.setModel("assets/editor/models/box.mdl", true);
		ttRigidbody.createBasedOnModel("assets/editor/models/box.mdl", 0.f, false, false);
	}

	void update(const GameUpdateSets& u)
	{
		if (u.isSimulationPaused()) {
			return;
		}

		// Add your logic here.
	}

	void postUpdate(const GameUpdateSets& u)
	{
		if (u.isSimulationPaused()) {
			return;
		}

		// Add your logic here.
	}
};

ReflAddTypeId(DummyActor, 30'02'22'0001);
ReflBlock()
{
	ReflAddActor(DummyActor);
}

} // namespace sge
