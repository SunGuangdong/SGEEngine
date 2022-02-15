#include "RigidBodyEditorConfig.h"
#include "Actor.h"
#include "IconsForkAwesome/IconsForkAwesome.h"
#include "sge_core/SGEImGui.h"
#include "sge_core/typelib/MemberChain.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/RigidBodyFromModel.h"
#include "sge_engine_ui/windows/PropertyEditorWindow.h"
#include "traits/TraitModel.h"
#include "traits/TraitRigidBody.h"
#include "typelibHelper.h"

namespace sge {
// clang-format off
ReflAddTypeId(RigidBodyPropertiesConfigurator,    21'02'28'0006);
ReflAddTypeId(RigidBodyConfigurator::ShapeSource, 21'02'28'0007);
ReflAddTypeId(RigidBodyConfigurator,              21'02'28'0008);

ReflBlock()
{
	ReflAddType(RigidBodyPropertiesConfigurator)
		ReflMember(RigidBodyConfigurator, mass).uiRange(0.f, 100000.f, 0.1f)
		ReflMember(RigidBodyPropertiesConfigurator, friction).uiRange(0.f, 1.f, 0.01f)
		ReflMember(RigidBodyPropertiesConfigurator, rollingFriction).uiRange(0.f, 1.f, 0.01f)
		ReflMember(RigidBodyPropertiesConfigurator, spinningFriction).uiRange(0.f, 1.f, 0.01f)
		ReflMember(RigidBodyPropertiesConfigurator, bounciness).uiRange(0.f, 1.f, 0.01f)
		ReflMember(RigidBodyPropertiesConfigurator, noMoveX).setPrettyName("No X Movement")
		ReflMember(RigidBodyPropertiesConfigurator, noMoveY).setPrettyName("No Y Movement")
		ReflMember(RigidBodyPropertiesConfigurator, noMoveZ).setPrettyName("No Z Movement")
		ReflMember(RigidBodyPropertiesConfigurator, noRotationX).setPrettyName("No X Rotation")
		ReflMember(RigidBodyPropertiesConfigurator, noRotationY).setPrettyName("No Y Rotation")
		ReflMember(RigidBodyPropertiesConfigurator, noRotationZ).setPrettyName("No Z Rotation")
		ReflMember(RigidBodyPropertiesConfigurator, movementDamping)
		ReflMember(RigidBodyPropertiesConfigurator, rotationDamping)
		ReflMember(RigidBodyPropertiesConfigurator, specifyGravity)
		ReflMember(RigidBodyPropertiesConfigurator, gravity)
	;

	ReflAddType(RigidBodyConfigurator::ShapeSource)
		ReflEnumVal(RigidBodyConfigurator::shapeSource_fromTraitModel, "From TraitModel")
		ReflEnumVal(RigidBodyConfigurator::shapeSource_manuallySpecify, "Manually Specify Shapes")
		ReflEnumVal(RigidBodyConfigurator::shapeSource_fromModel, "From Model 3D Collision Shapes")
		ReflEnumVal(RigidBodyConfigurator::shapeSource_fromModelRenderGeometry, "From Model 3D Renderable Geometry (Static Only)")
	;

	ReflAddType(RigidBodyConfigurator)
		ReflInherits(RigidBodyConfigurator, RigidBodyPropertiesConfigurator)
		ReflMember(RigidBodyConfigurator, shapeSource)
		ReflMember(RigidBodyConfigurator, assetPropery)
		ReflMember(RigidBodyConfigurator, collisionShapes)
		ReflMember(RigidBodyConfigurator, m_sourceModel)
	;	
}
// clang-format on

//---------------------------------------------------------------------
// RigidBodyConfigurator
//---------------------------------------------------------------------
void RigidBodyPropertiesConfigurator::applyProperties(Actor& actor) const
{
	TraitRigidBody* const traitRb = getTrait<TraitRigidBody>(&actor);

	if (traitRb && traitRb->getRigidBody() && traitRb->getRigidBody()->isValid()) {
		applyProperties(*traitRb->getRigidBody());
	}
}

void RigidBodyPropertiesConfigurator::applyProperties(RigidBody& rb) const
{
	rb.setFriction(friction);
	rb.setRollingFriction(rollingFriction);
	rb.setSpinningFriction(spinningFriction);
	rb.setBounciness(bounciness);
	rb.setCanMove(!noMoveX, !noMoveY, !noMoveZ);
	rb.setCanRotate(!noRotationX, !noRotationY, !noRotationZ);
	rb.setDamping(movementDamping, rotationDamping);

	rb.setMaskIdentifiesAs(identifiesAs);
	rb.setMaskCollidesWith(collidesWith);

	if (specifyGravity) {
		rb.setGravity(gravity);
	}
}

void RigidBodyPropertiesConfigurator::extractPropsFromRigidBody(const RigidBody& rb)
{
	mass = rb.getMass();
	friction = rb.getFriction();
	rollingFriction = rb.getRollingFriction();
	spinningFriction = rb.getSpinningFriction();
	bounciness = rb.getBounciness();

	rb.getCanMove(noMoveX, noMoveY, noMoveZ);
	noMoveX = !noMoveX;
	noMoveY = !noMoveY;
	noMoveZ = !noMoveZ;

	rb.getCanRotate(noRotationX, noRotationY, noRotationZ);
	noRotationX = !noRotationX;
	noRotationY = !noRotationY;
	noRotationZ = !noRotationZ;

	rb.getDamping(movementDamping, rotationDamping);

	gravity = rb.getGravity();
}

//---------------------------------------------------------------------
// RigidBodyConfigurator
//---------------------------------------------------------------------
bool RigidBodyConfigurator::apply(Actor& actor, bool addToWorldNow) const
{
	TraitRigidBody* const traitRb = getTrait<TraitRigidBody>(&actor);

	if (traitRb == nullptr) {
		return false;
	}

	const transf3d transformOfActor = actor.getTransform();

	switch (shapeSource) {
		case shapeSource_fromTraitModel: {
			TraitModel* const traitModel = getTrait<TraitModel>(&actor);
			if (traitModel) {
				traitRb->destroyRigidBody();

				std::vector<CollsionShapeDesc> collisionShapes;
				addCollisionShapeBasedOnTraitModel(collisionShapes, *traitModel);

				traitRb->getRigidBody()->create(
				    &actor, collisionShapes.data(), int(collisionShapes.size()), mass, false);
				traitRb->getRigidBody()->setTransformAndScaling(actor.getTransform(), true);

				if (addToWorldNow) {
					GameWorld* const world = actor.getWorld();
					world->physicsWorld.addPhysicsObject(*traitRb->getRigidBody());
				}
			}
		} break;
		case shapeSource_manuallySpecify: {
			traitRb->destroyRigidBody();
			traitRb->getRigidBody()->create(&actor, collisionShapes.data(), int(collisionShapes.size()), mass, false);
		} break;
		case shapeSource_fromModel: {
			traitRb->destroyRigidBody();
			const AssetIface_Model3D* modelIface = m_sourceModel.getAssetInterface<AssetIface_Model3D>();
			if (modelIface) {
				std::vector<CollsionShapeDesc> collisionShapes;
				addCollisionShapeBasedOnModel(collisionShapes, modelIface->getStaticEval());
				traitRb->getRigidBody()->create(
				    &actor, collisionShapes.data(), int(collisionShapes.size()), mass, false);
			}
		} break;
		case shapeSource_fromModelRenderGeometry: {
			traitRb->destroyRigidBody();
			const AssetIface_Model3D* modelIface = m_sourceModel.getAssetInterface<AssetIface_Model3D>();
			if (modelIface) {
				std::vector<CollsionShapeDesc> collisionShapes;
				addCollisionShapeBasedOnModelRenderGeom(collisionShapes, modelIface->getStaticEval());
				traitRb->getRigidBody()->create(
				    &actor, collisionShapes.data(), int(collisionShapes.size()), mass, false);
			}
		} break;
		default: {
			traitRb->destroyRigidBody();
			sgeAssert(false && "Not Implemented ShapeSource");
		} break;
	}

	if (traitRb->getRigidBody()) {
		traitRb->getRigidBody()->setTransformAndScaling(transformOfActor, true);
		// Apply the properties.
		applyProperties(*traitRb->getRigidBody());
	}

	if (addToWorldNow) {
		traitRb->addToWorld();
	}

	return true;
}

//---------------------------------------------------------------------
// User Interface
//---------------------------------------------------------------------
void edit_CollisionShapeDesc(GameInspector& inspector, GameObject* gameObject, MemberChain chain)
{
	CollsionShapeDesc& sdesc = *reinterpret_cast<CollsionShapeDesc*>(chain.follow(gameObject));

	const ImGuiEx::IDGuard idGuard(&sdesc);

	auto doMemberUIFn = [&](const MemberDesc* const md) -> void {
		if (md) {
			chain.add(md);
			ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain);
			chain.pop();
		}
	};

	doMemberUIFn(sgeFindMember(CollsionShapeDesc, offset));
	doMemberUIFn(sgeFindMember(CollsionShapeDesc, type));

	switch (sdesc.type) {
		case CollsionShapeDesc::type_box: {
			doMemberUIFn((sgeFindMember(CollsionShapeDesc, boxHalfDiagonal)));
		} break;
		case CollsionShapeDesc::type_sphere: {
			doMemberUIFn((sgeFindMember(CollsionShapeDesc, sphereRadius)));
		} break;
		case CollsionShapeDesc::type_capsule: {
			doMemberUIFn((sgeFindMember(CollsionShapeDesc, capsuleHeight)));
			doMemberUIFn((sgeFindMember(CollsionShapeDesc, capsuleRadius)));
		} break;
		case CollsionShapeDesc::type_cylinder: {
			doMemberUIFn((sgeFindMember(CollsionShapeDesc, cylinderHalfDiagonal)));
		} break;
		case CollsionShapeDesc::type_cone: {
			doMemberUIFn((sgeFindMember(CollsionShapeDesc, coneHeight)));
			doMemberUIFn((sgeFindMember(CollsionShapeDesc, coneRadius)));
		} break;
		default: {
			ImGui::TextEx("Not Implemented");
		}
	}
}

void edit_RigidBodyPropertiesConfigurator(GameInspector& inspector, GameObject* gameObject, MemberChain chain)
{
	RigidBodyPropertiesConfigurator& rbpc =
	    *reinterpret_cast<RigidBodyPropertiesConfigurator*>(chain.follow(gameObject));

	auto doMemberUIFn = [&](const MemberDesc* const md) -> void {
		if (md) {
			chain.add(md);
			ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain);
			chain.pop();
		}
	};

	const ImGuiEx::IDGuard idGuard(&rbpc);
	ImGuiEx::BeginGroupPanel("Rigid Body Properties Config");

	if (rbpc.dontShowDynamicProperties == false) {
		doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, mass));
	}

	ImGuiEx::BeginGroupPanel("CollisionFilter");
	{
		{
			ImGuiEx::IDGuard guard("indetify as");
			ImGuiEx::Label("Indentified as");

			const char* icons[4] = {
			    ICON_FK_CUBE,
			    ICON_FK_CIRCLE,
			    ICON_FK_STAR,
			    ICON_FK_BOMB,
			};

			for (int bit = 0; bit < 4; ++bit) {
				bool changed = false;
				if (rbpc.identifiesAs & (1 << bit)) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 1.f, 0.f, 1.f));
					changed = ImGui::Button(icons[bit]);
					ImGui::PopStyleColor();
				}
				else {
					changed = ImGui::Button(icons[bit]);
				}

				if (bit != 3) {
					ImGui::SameLine();
				}

				if (changed) {
					ubyte value = rbpc.identifiesAs & (1 << bit);

					if (value) {
						rbpc.identifiesAs = rbpc.identifiesAs & ~(1 << bit);
					}
					else {
						rbpc.identifiesAs = rbpc.identifiesAs | (1 << bit);
					}
				}
			}
		}

		{
			ImGuiEx::IDGuard guard("Collides with");
			ImGuiEx::Label("Collides with");

			const char* icons[4] = {
			    ICON_FK_CUBE,
			    ICON_FK_CIRCLE,
			    ICON_FK_STAR,
			    ICON_FK_BOMB,
			};

			for (int bit = 0; bit < 4; ++bit) {
				bool changed = false;
				if (rbpc.collidesWith & (1 << bit)) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 1.f, 0.f, 1.f));
					changed = ImGui::Button(icons[bit]);
					ImGui::PopStyleColor();
				}
				else {
					changed = ImGui::Button(icons[bit]);
				}

				if (bit != 3) {
					ImGui::SameLine();
				}

				if (changed) {
					ubyte value = rbpc.collidesWith & (1 << bit);

					if (value) {
						rbpc.collidesWith = rbpc.collidesWith & ~(1 << bit);
					}
					else {
						rbpc.collidesWith = rbpc.collidesWith | (1 << bit);
					}
				}
			}
		}
	}
	ImGuiEx::EndGroupPanel();


	doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, friction));
	doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, rollingFriction));
	doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, spinningFriction));
	doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, bounciness));

	if (rbpc.dontShowDynamicProperties == false) {
		doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, noMoveX));
		doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, noMoveY));
		doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, noMoveZ));
		doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, noRotationX));
		doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, noRotationY));
		doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, noRotationZ));
		doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, movementDamping));
		doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, rotationDamping));

		doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, specifyGravity));
		if (rbpc.specifyGravity) {
			doMemberUIFn(sgeFindMember(RigidBodyPropertiesConfigurator, gravity));
		}
	}

	if (ImGui::Button(ICON_FK_CHECK " Apply")) {
		Actor* const actor = gameObject->getActor();
		if (actor) {
			rbpc.applyProperties(*actor);
		}
	}

	ImGuiEx::EndGroupPanel();
}

SGE_ENGINE_API void edit_RigidBodyConfigurator(GameInspector& inspector, GameObject* gameObject, MemberChain chain)
{
	RigidBodyConfigurator& rbec = *reinterpret_cast<RigidBodyConfigurator*>(chain.follow(gameObject));

	const ImGuiEx::IDGuard idGuard(&rbec);

	ImGuiEx::BeginGroupPanel("Rigid Body Config");

	auto doMemberUIFn = [&](const MemberDesc* const md) -> void {
		if (md) {
			chain.add(md);
			ProperyEditorUIGen::doMemberUI(inspector, gameObject, chain);
			chain.pop();
		}
	};

	ImGuiEx::BeginGroupPanel("CollisionFilter");
	{
		{
			ImGuiEx::IDGuard guard("indetify as");
			ImGuiEx::Label("Indentified as");

			const char* icons[4] = {
			    ICON_FK_CUBE,
			    ICON_FK_CIRCLE,
			    ICON_FK_STAR,
			    ICON_FK_BOMB,
			};

			for (int bit = 0; bit < 4; ++bit) {
				bool changed = false;
				if (rbec.identifiesAs & (1 << bit)) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 1.f, 0.f, 1.f));
					changed = ImGui::Button(icons[bit]);
					ImGui::PopStyleColor();
				}
				else {
					changed = ImGui::Button(icons[bit]);
				}

				if (bit != 3) {
					ImGui::SameLine();
				}

				if (changed) {
					ubyte value = rbec.identifiesAs & (1 << bit);

					if (value) {
						rbec.identifiesAs = rbec.identifiesAs & ~(1 << bit);
					}
					else {
						rbec.identifiesAs = rbec.identifiesAs | (1 << bit);
					}
				}
			}
		}

		{
			ImGuiEx::IDGuard guard("Collides with");
			ImGuiEx::Label("Collides with");

			const char* icons[4] = {
			    ICON_FK_CUBE,
			    ICON_FK_CIRCLE,
			    ICON_FK_STAR,
			    ICON_FK_BOMB,
			};

			for (int bit = 0; bit < 4; ++bit) {
				bool changed = false;
				if (rbec.collidesWith & (1 << bit)) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 1.f, 0.f, 1.f));
					changed = ImGui::Button(icons[bit]);
					ImGui::PopStyleColor();
				}
				else {
					changed = ImGui::Button(icons[bit]);
				}

				if (bit != 3) {
					ImGui::SameLine();
				}

				if (changed) {
					ubyte value = rbec.collidesWith & (1 << bit);

					if (value) {
						rbec.collidesWith = rbec.collidesWith & ~(1 << bit);
					}
					else {
						rbec.collidesWith = rbec.collidesWith | (1 << bit);
					}
				}
			}
		}
	}
	ImGuiEx::EndGroupPanel();


	if (rbec.dontShowDynamicProperties == false) {
		doMemberUIFn(sgeFindMember(RigidBodyConfigurator, mass));
	}

	doMemberUIFn(sgeFindMember(RigidBodyConfigurator, friction));
	doMemberUIFn(sgeFindMember(RigidBodyConfigurator, rollingFriction));
	doMemberUIFn(sgeFindMember(RigidBodyConfigurator, spinningFriction));
	doMemberUIFn(sgeFindMember(RigidBodyConfigurator, bounciness));

	if (rbec.dontShowDynamicProperties == false) {
		doMemberUIFn(sgeFindMember(RigidBodyConfigurator, noMoveX));
		doMemberUIFn(sgeFindMember(RigidBodyConfigurator, noMoveY));
		doMemberUIFn(sgeFindMember(RigidBodyConfigurator, noMoveZ));
		doMemberUIFn(sgeFindMember(RigidBodyConfigurator, noRotationX));
		doMemberUIFn(sgeFindMember(RigidBodyConfigurator, noRotationY));
		doMemberUIFn(sgeFindMember(RigidBodyConfigurator, noRotationZ));
		doMemberUIFn(sgeFindMember(RigidBodyConfigurator, movementDamping));
		doMemberUIFn(sgeFindMember(RigidBodyConfigurator, rotationDamping));

		doMemberUIFn(sgeFindMember(RigidBodyConfigurator, specifyGravity));
		if (rbec.specifyGravity) {
			doMemberUIFn(sgeFindMember(RigidBodyConfigurator, gravity));
		}
	}


	doMemberUIFn(sgeFindMember(RigidBodyConfigurator, shapeSource));

	if (rbec.shapeSource == RigidBodyConfigurator::shapeSource_fromTraitModel) {
		if (gameObject->findTraitByFamily(sgeTypeId(TraitModel)) == nullptr) {
			ImGui::TextEx("The current objects needs to have TraitModel for this option to work.");
		}
	}
	else if (rbec.shapeSource == RigidBodyConfigurator::shapeSource_manuallySpecify) {
		ImGui::Text("Manually specify the shapes that will form the rigid body below!");
		doMemberUIFn(sgeFindMember(RigidBodyConfigurator, collisionShapes));
	}
	else if (rbec.shapeSource == RigidBodyConfigurator::shapeSource_fromModel) {
		ImGui::Text("Model to be the source of collision geometry.");
		doMemberUIFn(sgeFindMember(RigidBodyConfigurator, m_sourceModel));
	}
	else if (rbec.shapeSource == RigidBodyConfigurator::shapeSource_fromModelRenderGeometry) {
		ImGui::Text("Model to be the source of collision geometry. Usable of Static Objects only");
		doMemberUIFn(sgeFindMember(RigidBodyConfigurator, m_sourceModel));
	}

	if (ImGui::Button(ICON_FK_CHECK " Apply")) {
		Actor* const actor = gameObject->getActor();
		if (actor) {
			rbec.apply(*actor, true);
		}
	}

	ImGuiEx::EndGroupPanel();
}



} // namespace sge
