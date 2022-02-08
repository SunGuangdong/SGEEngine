#include "PlantingTool.h"
#include "GameInspector.h"
#include "sge_core/AssetLibrary/AssetLibrary.h"
#include "sge_core/ICore.h"
#include "sge_core/SGEImGui.h"
#include "sge_core/application/input.h"
#include "sge_engine/GameDrawer/GameDrawer.h"
#include "sge_engine/InspectorCmds.h"
#include "sge_engine/physics/PhysicsWorldQuery.h"
#include "sge_engine/traits/TraitCamera.h"

namespace sge {

struct AInvisibleRigidObstacle;

void PlantingTool::setup(Actor* actorToPlant)
{
	vector_set<ObjectId> actorList;
	if (actorToPlant) {
		actorList.insert(actorToPlant->getId());
	}

	setup(actorList, *actorToPlant->getWorld());
}

void PlantingTool::setup(const vector_set<ObjectId>& actorsToPlant, GameWorld& world)
{
	actorsToPlantOriginalTrasnforms.clear();
	actorsToIgnorePhysicsHits.clear();

	for (ObjectId actorId : actorsToPlant) {
		Actor* actor = world.getActorById(actorId);
		if (actor) {
			actorsToPlantOriginalTrasnforms[actor->getId()] = actor->getTransform();

			// Add the object and all of its child to be ignored by raycasts.
			world.getAllChildren(actorsToIgnorePhysicsHits, actor->getId());
			actorsToIgnorePhysicsHits.insert(actor->getId());
		}
	}
}

void PlantingTool::onSetActive(GameInspector* const UNUSED(inspector))
{
}

void PlantingTool::onUI(GameInspector* UNUSED(inspector))
{
	ImGui::Checkbox("Rotate Objects", &shouldRotateObjects);
}

InspectorToolResult
    PlantingTool::updateTool(GameInspector* const inspector, bool isAllowedToTakeInput, const InputState& is, const GameDrawSets& drawSets)
{
	InspectorToolResult result;

	// Compute the picking ray.
	vec2f const viewportSize(drawSets.rdest.viewport.width, drawSets.rdest.viewport.height);
	vec2f const cursorUV = is.GetCursorPos() / viewportSize;
	const Ray pickRay = drawSets.drawCamera->perspectivePickWs(cursorUV);

	if (isAllowedToTakeInput && is.IsKeyPressed(Key::Key_Escape)) {
		inspector->setTool(nullptr);
		result.propagateInput = false;
		return result;
	}

	GameWorld* const world = inspector->getWorld();

	const btVector3 rayPos = toBullet(pickRay.pos);
	const btVector3 rayTar = toBullet(pickRay.pos + pickRay.dir * 1e6f);

	if (actorsToPlantOriginalTrasnforms.empty()) {
		result.propagateInput = false;
		result.isDone = true;
		return result;
	}

	Actor* const primaryActor = inspector->getWorld()->getActorById(actorsToPlantOriginalTrasnforms.begin().key());
	if (primaryActor == nullptr) {
		result.propagateInput = false;
		result.isDone = true;
		return result;
	}

	std::function<bool(const Actor*)> actorFilterFn = [&](const Actor* a) -> bool {
		bool isInIngnoreList = actorsToIgnorePhysicsHits.count(a->getId()) != 0;
		return isInIngnoreList || a->getType() == sgeTypeId(AInvisibleRigidObstacle);
	};

	// Try to cast a ray from the camera in direction of the cursor position.
	// Based on that hit compute the transformation of the object.
	// If not physics world hit was found, come up with something based on the camera settings.
	RayResultActor rayResult;
	rayResult.setup(nullptr, rayPos, rayTar, actorFilterFn);
	world->physicsWorld.dynamicsWorld->rayTest(rayPos, rayTar, rayResult);

	transf3d primaryActorNewTransf = actorsToPlantOriginalTrasnforms.begin().value();
	if (rayResult.hasHit()) {
		const vec3f hitNormal = normalized0(fromBullet(rayResult.m_hitNormalWorld));

		MemberChain mfc;
		mfc.add(typeLib().findMember(&Actor::m_logicTransform));

		const vec3f rotateAxis = cross(vec3f::getAxis(1), hitNormal);
		const float rotationAngle = asinf(rotateAxis.length());

		primaryActorNewTransf.p = fromBullet(rayResult.m_hitPointWorld);
		primaryActorNewTransf.r = quatf::getAxisAngle(rotateAxis.normalized0(), rotationAngle);

		const Box3f bbox = primaryActor->getBBoxOS();
		if (bbox.IsEmpty() == false) {
			if (bbox.min.y < 0.f) {
				primaryActorNewTransf.p -= hitNormal * bbox.min.y;
			}
		}
	}
	else {
		// intersect with the "grid" plane witch is looking towards Y+ from (0,0,0).
		const Plane p = Plane::FromPosAndDir(vec3f(0.f), vec3f::axis_y());
		const float hitDistance = intersectRayPlane(pickRay.pos, pickRay.dir, p.norm(), p.d());

		if (hitDistance != FLT_MAX) {
			if (hitDistance > 0.f) {
				primaryActorNewTransf.p = pickRay.Sample(hitDistance);
			}
			else {
				// A bit a of strage solution but here me out:
				// if the hit distance is negative, this means that the camera is not looking towards
				// the plane, meaning that the hit is behind the camera.
				// Placing the object behind the camera is pretty stupid and non-intuitive.
				// In order to place the object on same logical position We are going to do this:
				//
				// ----(plane)---*(negative hit)----------------------
				//                \                    
				//                 \             
				//                  \           
				//                   * (eye - ray start from here)
				//                    \
				//                     > (look dir - the ray is looking towards this direction)
				//                      \                            
				//                       \                                                        
				//                        * fabsf(negative hit)
				//
				// We will use (negative hit) here, so the object would be placed infornt of the camera.

				const float absHitDistance = fabsf(hitDistance);
				primaryActorNewTransf.p = pickRay.Sample(absHitDistance);
			}
		}
	}

	// Apply the snapping form the transform tool.
	if (inspector->m_transformTool.m_useSnapSettings) {
		primaryActorNewTransf.p = inspector->m_transformTool.m_snapSettings.applySnappingTranslation(primaryActorNewTransf.p);
	}

	const transf3d diffTrasform = primaryActorNewTransf * actorsToPlantOriginalTrasnforms.begin().value().inverseSimple();
	transf3d diffTrasformNoRotation = diffTrasform;
	diffTrasformNoRotation.r = quatf::getIdentity();

	if (isAllowedToTakeInput && is.IsKeyReleased(Key::Key_MouseLeft)) {
		CmdCompound* cmdAll = new CmdCompound();

		for (auto pair : actorsToPlantOriginalTrasnforms) {
			Actor* actor = world->getActorById(pair.key());
			if (actor) {
				CmdMemberChange* const cmd = new CmdMemberChange();
				transf3d newTransform;

				if (shouldRotateObjects) {
					newTransform = diffTrasform * pair.value();
				}
				else {
					newTransform = diffTrasformNoRotation * pair.value();
				}

				cmd->setupLogicTransformChange(*actor, pair.value(), newTransform);
				cmdAll->addCommand(cmd);
			}
		}

		inspector->appendCommand(cmdAll, true);

		result.propagateInput = false;
		result.isDone = true;
		return result;
	}
	else {
		for (const auto& pair : actorsToPlantOriginalTrasnforms) {
			Actor* actor = world->getActorById(pair.key());
			if (actor) {
				actor->setTransform(diffTrasform * pair.value(), true);
			}
		}
	}

	result.propagateInput = false;
	return result;
}

void PlantingTool::onCancel(GameInspector* inspector)
{
	actorsToPlantOriginalTrasnforms.clear();
	actorsToIgnorePhysicsHits.clear();

	for (const auto& pair : actorsToPlantOriginalTrasnforms) {
		Actor* actor = inspector->getWorld()->getActorById(pair.key());
		if (actor) {
			actor->setTransform(pair.value(), true);
		}
	}
}

void PlantingTool::drawOverlay(const GameDrawSets& UNUSED(drawSets))
{
}

} // namespace sge
