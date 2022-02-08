#include "sge_core/model/ModelAnimator.h"
#include "sge_engine/actors/ANavMesh.h"
#include "sge_engine_basics.h"

namespace sge {
enum TyAnimTrack : int {
	tyAnim_idle,
	tyAnim_run,
};

struct AKnight : public Actor {
	TraitModel ttModel;
	ModelAnimator modelAnimator;
	std::vector<mat4f> bones;

	int palmMeshNodeIndex = -1;

	ObjectId navMeshActorId;
	std::vector<vec3f> walkingPathTemp;

	Box3f getBBoxOS() const override
	{
		return ttModel.getBBoxOS();
	}

	void create() override
	{
		registerTrait(ttModel);

		ttModel.addModel("assets/knight/knight.mdl", true);
		ttModel.addModel("assets/knight/sword.mdl", false);

		palmMeshNodeIndex = ttModel.m_models[0].customEvalModel->m_model->findFistNodeIndexWithName("palm.L");

		modelAnimator.create(*ttModel.m_models[0].customEvalModel->m_model);

		modelAnimator.trackSetFadeTime(tyAnim_idle, 0.2f);
		modelAnimator.trackAddAmimPath(tyAnim_idle, "assets/knight/knight.mdl", "idle");
		modelAnimator.trackSetFadeTime(tyAnim_run, 0.2f);
		modelAnimator.trackAddAmimPath(tyAnim_run, "assets/knight/knight.mdl", "walk");


		modelAnimator.playTrack(tyAnim_idle);
	};


	void update(const GameUpdateSets& u)
	{
		if (u.isSimulationPaused()) {
			return;
		}

		if (u.is.IsKeyReleased(Key_1)) {
			modelAnimator.playTrack(tyAnim_idle);
		}

		if (u.is.IsKeyReleased(Key_2)) {
			modelAnimator.playTrack(tyAnim_run);
		}

		Optional<vec3f> targetPointWs;
		if (u.is.IsKeyDown(Key_MouseLeft)) {
			Ray rayWs = getWorld()->getRenderCamera()->perspectivePickWs(u.is.getCursorPosUV());
			Plane feetPlane = Plane::FromPosAndDir(getPosition(), vec3f(0.f, 1.f, 0.f));

			float intersectionDistance = feetPlane.intersectRay(rayWs);
			if (intersectionDistance != FLT_MAX) {
				targetPointWs = rayWs.Sample(intersectionDistance);
			}
		}

		if (targetPointWs) {
			vec3f walkDir = (targetPointWs.get() - getPosition()).normalized0();
			
			vec3f newPos = getPosition();

			if (ANavMesh* navMesh = getWorld()->getActor<ANavMesh>(navMeshActorId)) {
				newPos = navMesh->moveAlongNavMesh(getPosition(), getPosition() + walkDir * 13.f * u.dt);
			}

			walkDir = newPos - getPosition();

			if (newPos != getPosition()) {
				setPosition(newPos);

				ttModel.m_models[0].m_additionalTransform = mat4f::getRotationQuat(
				    quatf::fromNormalizedVectors(vec3f(0.f, 0.f, -1.f), walkDir.x0z().normalized0(), vec3f::axis_y()));

				modelAnimator.playTrack(tyAnim_run);
			}
			else {
				modelAnimator.playTrack(tyAnim_idle);
			}
		}
		else {
			modelAnimator.playTrack(tyAnim_idle);
		}

		// Compute the skinning animation.
		modelAnimator.advanceAnimation(u.dt);
		modelAnimator.computeModleNodesTrasnforms(bones);

		// Move the attached sword to the parm of the actor.
		if (palmMeshNodeIndex >= 0) {
			ttModel.m_models[1].m_additionalTransform = ttModel.m_models[0].m_additionalTransform * bones[palmMeshNodeIndex];
		}

		// Set the skinning for the character model.
		ttModel.m_models[0].customEvalModel->evaluate(bones.data(), int(bones.size()));
	}
};

ReflAddTypeId(AKnight, 21'12'19'0001);

ReflBlock()
{
	ReflAddActor(AKnight) ReflMember(AKnight, navMeshActorId);
}


} // namespace sge
