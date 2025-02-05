#define _CRT_SECURE_NO_WARNINGS

#include "PropertyEditorWindow.h"
#include "IconsForkAwesome/IconsForkAwesome.h"
#include "sge_core/ICore.h"
#include "sge_core/SGEImGui.h"
#include "sge_core/typelib/MemberChain.h"
#include "sge_core/ui/MultiCurve2DEditor.h"
#include "sge_engine/AssetProperty.h"
#include "sge_engine/DynamicProperties.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/GameInspector.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/InspectorCmds.h"
#include "sge_engine/RigidBodyEditorConfig.h"
#include "sge_engine/physics/CollisionShape.h"
#include "sge_engine/traits/TraitCustomAE.h"
#include "sge_engine/traits/TraitScriptSlot.h"
#include "sge_engine_ui/ui/UIAssetPicker.h"
#include "sge_utils/containers/Variant.h"
#include "sge_utils/math/EulerAngles.h"
#include "sge_utils/math/MultiCurve2D.h"
#include "sge_utils/math/Rangef.h"
#include "sge_utils/math/SphericalCoordinates.h"
#include "sge_utils/text/format.h"

namespace sge {

//----------------------------------------------------------
// ProperyEditorUIGen
//----------------------------------------------------------
namespace ProperyEditorUIGen {
	// A set of variables used for the user interface.
	struct UIState {
		int createObjComboIdx = 0;
		int inspectObjComboIdx = 0;

		// In order to create CmdMemberChange commands, we must keep the inial value of the variable that is being
		// changed and when the user is done interacting with the object we use this value for the "original" state of
		// the variable.
		enum { VariantSizeInBytes = sizeof(transf3d) };
		Variant<VariantSizeInBytes> widgetSavedData;
	};

	// Caution:
	// This one instance of the  UI state hold additinal data per each widget in the Property Editor window.
	// The idea is to store somewhere the original value of the property being edited so when the change of the value is
	// complete (meaning that the user is no longer interacting with the widget) we can generate undo/redo histroy). One
	// instance of this state should be enough as the user can only interact with one widget at the time (as there is
	// only one mouse and one keyboard.
	UIState g_propWidgetState;
} // namespace ProperyEditorUIGen

void ProperyEditorUIGen::doGameObjectUI(GameInspector& inspector, GameObject* const gameObject)
{
	const TypeDesc* const pDesc = typeLib().find(gameObject->getType());
	if (pDesc != nullptr) {
		const AssetIface_Texture2D* iconImg = getLoadedAssetIface<AssetIface_Texture2D>(
		    getEngineGlobal()->getEngineAssets().getIconForObjectType(gameObject->getType()));
		if (iconImg && iconImg->getTexture()) {
			ImGui::Image(iconImg->getTexture(), ImVec2(32.f, 32.f));
			ImGui::SameLine();
		}

		ImGui::Text(pDesc->name);


		char objIdText[64] = {'\0'};
		sge_snprintf(objIdText, SGE_ARRSZ(objIdText), "%d", gameObject->getId().id);
		ImGuiEx::Label("ID");
		ImGui::InputText("##GameObject_Id", objIdText, SGE_ARRSZ(objIdText), ImGuiInputTextFlags_ReadOnly);

		if (ImGui::BeginChild("PropertyEditorObjectProperties")) {
			IActorCustomAttributeEditorTrait* const actorCustomAETrait =
			    getTrait<IActorCustomAttributeEditorTrait>(gameObject);
			if (actorCustomAETrait) {
				// If the attribute editor is customly made, make sure that we show the most common properties
				// automatically. It is thedious to always add them manually.
				ProperyEditorUIGen::doMemberUI(inspector, gameObject, sgeFindMember(GameObject, m_displayName));
				if (gameObject->isActor()) {
					ProperyEditorUIGen::doMemberUI(inspector, gameObject, sgeFindMember(Actor, m_logicTransform));
				}

				actorCustomAETrait->doAttributeEditor(&inspector);
			}
			else {
				for (const MemberDesc& member : pDesc->members) {
					if (member.isEditable()) {
						ProperyEditorUIGen::doMemberUI(inspector, gameObject, MemberChain(&member));
					}
				}
			}
		}
		ImGui::EndChild();
	}
}

void ProperyEditorUIGen::doMemberUI(
    GameInspector& inspector, GameObject* const gameObject, MemberChain chain, int flags)
{
	const TypeDesc* const memberTypeDesc = chain.getType();
	// Caution:
	// if the member is an index in an array this points to the array,
	// not the value as there is no MemberDesc  for array elements!
	const MemberDesc& member = *chain.knots.back().mfd;
	const char* const memberName = chain.knots.back().mfd->prettyName.c_str();
	char* const pMember = (char*)chain.follow(gameObject);

	Actor* actor = gameObject->getActor();

	if (pMember == nullptr) {
		ImGui::Text("TODO: Getter/Setter based member %s", memberName);
		return;
	}

	// Just push some unique ID in order to avoid clashes for UI elements with same name(as imgui uses names for ids).
	ImGuiEx::IDGuard memberIDGuard((void*)pMember);

	const PropertyEditorGeneratorForTypeFn pUserUIGenerator =
	    getEngineGlobal()->getPropertyEditorIUGeneratorForType(memberTypeDesc->typeId);

	if (pUserUIGenerator) {
		pUserUIGenerator(inspector, gameObject, chain);
	}
	else if (memberTypeDesc->typeId == sgeTypeId(CollsionShapeDesc)) {
		edit_CollisionShapeDesc(inspector, gameObject, chain);
	}
	else if (memberTypeDesc->typeId == sgeTypeId(RigidBodyPropertiesConfigurator)) {
		edit_RigidBodyPropertiesConfigurator(inspector, gameObject, chain);
	}
	else if (memberTypeDesc->typeId == sgeTypeId(RigidBodyConfigurator)) {
		edit_RigidBodyConfigurator(inspector, gameObject, chain);
	}
	else if (memberTypeDesc->typeId == sgeTypeId(TraitScriptSlot)) {
		TraitScriptSlot_doProperyEditor(inspector, gameObject, chain);
	}
	else if (memberTypeDesc->typeId == sgeTypeId(transf3d)) {
		static bool uiTransformInLocalSpace = false;

		const bool isLogicTransform = member.is(&Actor::m_logicTransform);
		const bool actorHasParent = inspector.getWorld()->getParentActor(gameObject->getId()) != nullptr;

		ImGuiEx::BeginGroupPanel(memberName, ImVec2(-1.f, -1.f));

		if (actorHasParent && isLogicTransform) {
			ImGuiEx::Label("View in Local Space");
			ImGui::Checkbox("##Local Space", &uiTransformInLocalSpace);
		}

		bool thisIsTransformInLocalSpace = false;
		if (isLogicTransform && uiTransformInLocalSpace && actor) {
			thisIsTransformInLocalSpace = actorHasParent;
		}

		transf3d& memberRef = *(transf3d*)(pMember);
		transf3d v = memberRef;

		if (thisIsTransformInLocalSpace) {
			v = actor->m_bindingToParentTransform;
		}
		else if (isLogicTransform) {
		}
		else {
		}

		// Display and handle the UI.
		bool justReleased = false;
		bool justActivated = false;

		vec3f eulerAngles = quaternionToEuler(v.r);

		for (int t = 0; t < 3; t++) {
			eulerAngles[t] = rad2deg(eulerAngles[t]);
		}

		bool change = false;
		ImGuiEx::Label("Position");
		change |= SGEImGui::DragFloats("##Position", v.p.data, 3, &justReleased, &justActivated, 0.f);
		ImGuiEx::Label("Rotation");
		change |= SGEImGui::DragFloats("##Rotation", eulerAngles.data, 3, &justReleased, &justActivated, 0.f);

		ImGuiEx::Label("Scaling");

		float scalingDragsWidth = ImGui::CalcItemWidth();
		ImGui::PopItemWidth();

		static bool lockedScaling = true;
		if (ImGui::Button(lockedScaling ? ICON_FK_LOCK : ICON_FK_UNLOCK)) {
			lockedScaling = !lockedScaling;
		}

		scalingDragsWidth =
		    std::max(scalingDragsWidth - ImGui::GetItemRectSize().x - ImGui::GetStyle().ItemSpacing.x, 10.f);

		ImGui::SameLine();
		ImGui::PushItemWidth(scalingDragsWidth);
		const bool isInitalScalingUniform = v.s.x == v.s.y && v.s.y == v.s.z;
		if (isInitalScalingUniform && lockedScaling) {
			// Locked Uniform scaling.
			float uniformScale = v.s.x;
			change |= SGEImGui::DragFloats("##Scaling", &uniformScale, 1, &justReleased, &justActivated, 1.f, 0.01f);
			v.s = vec3f(uniformScale);
		}
		else {
			if (lockedScaling) {
				// Locked non-uniform scaling.
				float uniformScalePreDiff = v.s.x;
				float uniformScale = v.s.x;
				change |=
				    SGEImGui::DragFloats("##Scaling", &uniformScale, 1, &justReleased, &justActivated, 1.f, 0.01f);
				float diff = uniformScale / uniformScalePreDiff;
				v.s *= diff;
			}
			else {
				// Non-locked scaling.
				change |= SGEImGui::DragFloats("##Scaling", v.s.data, 3, &justReleased, &justActivated, 1.f, 0.01f);
			}
		}

		for (int t = 0; t < 3; t++) {
			eulerAngles[t] = deg2rad(eulerAngles[t]);
		}

		v.r = eulerToQuaternion(eulerAngles);

		// If the user has just started interacting with this widget remember the inial value of the member.
		if (justActivated) {
			if (thisIsTransformInLocalSpace) {
				g_propWidgetState.widgetSavedData.resetVariantToValue<transf3d>(actor->m_bindingToParentTransform);
			}
			else {
				g_propWidgetState.widgetSavedData.resetVariantToValue<transf3d>(memberRef);
			}
		}

		if (change) {
			if (isLogicTransform && actor) {
				if (thisIsTransformInLocalSpace) {
					actor->setLocalTransform(v);
				}
				else {
					actor->setTransform(v);
				}
				// HACK: force gizmo reset.
				inspector.m_transformTool.workingSelectionDirtyIndex = 0;
			}
			else {
				memberRef = v;
			}
		}

		if (justReleased) {
			// Check if the new data is actually different, as the UI may fire a lot of updates at us.
			transf3d* pSavedValued = g_propWidgetState.widgetSavedData.get<transf3d>();
			sgeAssert(pSavedValued);
			if (*pSavedValued != v) {
				CmdMemberChange* cmd = new CmdMemberChange;
				if (isLogicTransform) {
					if (thisIsTransformInLocalSpace) {
						cmd->setup(
						    gameObject,
						    chain,
						    g_propWidgetState.widgetSavedData.get<transf3d>(),
						    &v,
						    &CmdMemberChange::setActorLocalTransform);
					}
					else {
						cmd->setup(
						    gameObject,
						    chain,
						    g_propWidgetState.widgetSavedData.get<transf3d>(),
						    &v,
						    &CmdMemberChange::setActorLogicTransform);
					}
				}
				else {
					cmd->setup(gameObject, chain, g_propWidgetState.widgetSavedData.get<transf3d>(), &v, nullptr);
				}

				inspector.appendCommand(cmd, true);

				g_propWidgetState.widgetSavedData.Destroy();
			}
		}
		ImGuiEx::EndGroupPanel();
	}
	else if (memberTypeDesc->typeId == sgeTypeId(TypeId)) {
		TypeId& typeId = *(TypeId*)(pMember);
		TypeId typeIdOriginal = typeId;
		TypeId typeIdEditable = typeId;
		if (gameObjectTypePicker(memberTypeDesc->name, typeIdEditable)) {
			CmdMemberChange* cmd = new CmdMemberChange;
			cmd->setup(gameObject, chain, &typeIdOriginal, &typeIdEditable, nullptr);
			inspector.appendCommand(cmd, true);
		}
	}
	else if (memberTypeDesc->typeId == sgeTypeId(AssetProperty)) {
		const AssetProperty& assetPropery = *(AssetProperty*)(pMember);

		AssetPtr assetToChange = assetPropery.m_asset;
		bool hadAChange = false;
		if (assetPropery.m_uiPossibleAssets.empty()) {
			hadAChange = assetPicker(
			    memberName,
			    assetToChange,
			    getCore()->getAssetLib(),
			    assetPropery.m_acceptedAssetIfaceTypes.data(),
			    assetPropery.m_acceptedAssetIfaceTypes.size());
		}
		else {
			ImGuiEx::Label(memberName);

			const char* const comboSelectedItemText =
			    isAssetLoaded(assetPropery.getAsset()) ? assetPropery.getAsset()->getPath().c_str() : nullptr;

			if (ImGui::BeginCombo("##Asset", comboSelectedItemText)) {
				for (const std::string& option : assetPropery.m_uiPossibleAssets) {
					bool isSelected = option == comboSelectedItemText;
					if (ImGui::Selectable(option.c_str(), &isSelected)) {
						assetToChange = getCore()->getAssetLib()->getAssetFromFile(option.c_str());
						hadAChange = true;
					}
				}

				ImGui::EndCombo();
			}
		}

		if (hadAChange) {
			AssetProperty newData = assetPropery;
			newData.setAsset(assetToChange);
			AssetProperty oldData = assetPropery;

			CmdMemberChange* const cmd = new CmdMemberChange;
			cmd->setup(gameObject, chain, &oldData, &newData, CmdMemberChange::setAssetPropertyChange);
			inspector.appendCommand(cmd, true);
		}
	}
	else if (memberTypeDesc->typeId == sgeTypeId(bool)) {
		bool v = *(bool*)(pMember);
		ImGuiEx::Label(memberName);
		if (ImGui::Checkbox("##Checkbox", &v)) {
			bool origivalValue = !v;

			CmdMemberChange* cmd = new CmdMemberChange;
			cmd->setup(gameObject, chain, &origivalValue, &v, nullptr);
			inspector.appendCommand(cmd, true);
		}
	}
	else if (memberTypeDesc->typeId == sgeTypeId(float)) {
		editFloat(inspector, memberName, gameObject, chain);
	}
	else if (memberTypeDesc->typeId == sgeTypeId(Rangef)) {
		Rangef& valRef = *(Rangef*)(pMember);
		Rangef edit = valRef;

		bool justReleased = false;
		bool justActivated = false;
		bool change = false;
		ImGui::Text(memberName);

		ImGui::SameLine();
		bool checkboxChange = ImGui::Checkbox("Locked##RangeLocked", &edit.locked);
		change |= checkboxChange;

		ImGui::SameLine();
		if (edit.locked) {
			change |= SGEImGui::DragFloats("##RangeMin", &edit.min, 1, &justReleased, &justActivated);
		}
		else {
			change |= SGEImGui::DragFloats("##RangeMinMax", &edit.min, 2, &justReleased, &justActivated);
		}

		if (justActivated) {
			g_propWidgetState.widgetSavedData.resetVariantToValue<Rangef>(valRef);
		}

		if (change) {
			valRef = edit;
		}

		if (justReleased) {
			// Check if the new data is actually different, as the UI may fire a lot of updates at us.
			if (*g_propWidgetState.widgetSavedData.get<Rangef>() != valRef) {
				CmdMemberChange* cmd = new CmdMemberChange;
				cmd->setup(gameObject, chain, g_propWidgetState.widgetSavedData.get<Rangef>(), &edit, nullptr);
				inspector.appendCommand(cmd, true);

				g_propWidgetState.widgetSavedData.Destroy();
			}
		}
	}
	else if (memberTypeDesc->typeId == sgeTypeId(vec2i)) {
		vec2i& vref = *(vec2i*)(pMember);
		vec2i vedit = vref;

		bool justReleased = false;
		bool justActivated = false;
		bool change = false;

		ImGuiEx::Label(memberName);
		change = SGEImGui::DragInts("##DragInts", vedit.data, 2, &justReleased, &justActivated);


		if (justActivated) {
			g_propWidgetState.widgetSavedData.resetVariantToValue<vec2i>(vref);
		}

		if (change) {
			vref = vedit;
		}

		if (justReleased) {
			// Check if the new data is actually different, as the UI may fire a lot of updates at us.
			if (*g_propWidgetState.widgetSavedData.get<vec2i>() != vref) {
				CmdMemberChange* cmd = new CmdMemberChange;
				cmd->setup(gameObject, chain, g_propWidgetState.widgetSavedData.get<vec2i>(), &vedit, nullptr);
				inspector.appendCommand(cmd, true);

				g_propWidgetState.widgetSavedData.Destroy();
			}
		}
	}
	else if (memberTypeDesc->typeId == sgeTypeId(vec2f)) {
		vec2f& vref = *(vec2f*)(pMember);
		vec2f vedit = vref;

		bool justReleased = false;
		bool justActivated = false;
		bool change = false;

		ImGuiEx::Label(memberName);
		change = SGEImGui::DragFloats(
		    "##Drag_vec2f",
		    vedit.data,
		    2,
		    &justReleased,
		    &justActivated,
		    0.f,
		    member.sliderSpeed_float,
		    member.min_float,
		    member.max_float);


		if (justActivated) {
			g_propWidgetState.widgetSavedData.resetVariantToValue<vec2f>(vref);
		}

		if (change) {
			vref = vedit;
		}

		if (justReleased) {
			// Check if the new data is actually different, as the UI may fire a lot of updates at us.
			if (*g_propWidgetState.widgetSavedData.get<vec2f>() != vref) {
				CmdMemberChange* cmd = new CmdMemberChange;
				cmd->setup(gameObject, chain, g_propWidgetState.widgetSavedData.get<vec2f>(), &vedit, nullptr);
				inspector.appendCommand(cmd, true);

				g_propWidgetState.widgetSavedData.Destroy();
			}
		}
	}
	else if (memberTypeDesc->typeId == sgeTypeId(quatf)) {
		quatf& quatRef = *(quatf*)(pMember);

		vec3f eulerAngles = quaternionToEuler(quatRef);

		for (int t = 0; t < 3; t++) {
			eulerAngles[t] = rad2deg(eulerAngles[t]);
		}

		ImGuiEx::Label(member.prettyName.c_str());
		bool justReleased = false, justActivated = false;
		bool change = SGEImGui::DragFloats("##RotationQuat", eulerAngles.data, 3, &justReleased, &justActivated, 0.f);

		for (int t = 0; t < 3; t++) {
			eulerAngles[t] = deg2rad(eulerAngles[t]);
		}

		if (justActivated) {
			g_propWidgetState.widgetSavedData.resetVariantToValue<quatf>(quatRef);
		}

		quatf newQ = eulerToQuaternion(eulerAngles);

		if (change) {
			quatRef = newQ;
		}

		if (justReleased) {
			// Check if the new data is actually different, as the UI may fire a lot of updates at us.
			if (g_propWidgetState.widgetSavedData.get<quatf>() &&
			    *g_propWidgetState.widgetSavedData.get<quatf>() != quatRef) {
				CmdMemberChange* cmd = new CmdMemberChange;
				cmd->setup(gameObject, chain, g_propWidgetState.widgetSavedData.get<quatf>(), &quatRef, nullptr);
				inspector.appendCommand(cmd, true);

				g_propWidgetState.widgetSavedData.Destroy();
			}
		}
	}
	else if (memberTypeDesc->typeId == sgeTypeId(vec3f)) {
		vec3f& v3ref = *(vec3f*)(pMember);
		vec3f v3edit = v3ref;

		bool justReleased = false;
		bool justActivated = false;
		bool change = false;

		ImGuiEx::Label(memberName);
		if (member.flags & MFF_Vec3fAsColor) {
			change = SGEImGui::ColorPicker3(memberName, v3edit.data, &justReleased, &justActivated, 0);
		}
		else {
			change = SGEImGui::DragFloats(memberName, v3edit.data, 3, &justReleased, &justActivated);
		}

		if (justActivated) {
			g_propWidgetState.widgetSavedData.resetVariantToValue<vec3f>(v3ref);
		}

		if (change) {
			v3ref = v3edit;
		}

		if (justReleased) {
			// Check if the new data is actually different, as the UI may fire a lot of updates at us.
			if (*g_propWidgetState.widgetSavedData.get<vec3f>() != v3ref) {
				CmdMemberChange* cmd = new CmdMemberChange;
				cmd->setup(gameObject, chain, g_propWidgetState.widgetSavedData.get<vec3f>(), &v3edit, nullptr);
				inspector.appendCommand(cmd, true);

				g_propWidgetState.widgetSavedData.Destroy();
			}
		}
	}
	else if (memberTypeDesc->typeId == sgeTypeId(vec4f)) {
		vec4f& v4ref = *(vec4f*)(pMember);
		vec4f v4edit = v4ref;

		bool justReleased = false;
		bool justActivated = false;
		bool change = false;

		ImGuiEx::Label(memberName);
		if (member.flags & MFF_Vec4fAsColor) {
			change = SGEImGui::ColorPicker4(
			    memberName, v4edit.data, &justReleased, &justActivated, ImGuiColorEditFlags_AlphaBar);
		}
		else {
			change = SGEImGui::DragFloats(memberName, v4edit.data, 4, &justReleased, &justActivated);
		}

		if (justActivated) {
			g_propWidgetState.widgetSavedData.resetVariantToValue<vec4f>(v4ref);
		}

		if (change) {
			v4ref = v4edit;
		}

		if (justReleased) {
			// Check if the new data is actually different, as the UI may fire a lot of updates at us.
			if (*g_propWidgetState.widgetSavedData.get<vec4f>() != v4ref) {
				CmdMemberChange* cmd = new CmdMemberChange;
				cmd->setup(gameObject, chain, g_propWidgetState.widgetSavedData.get<vec4f>(), &v4edit, nullptr);
				inspector.appendCommand(cmd, true);

				g_propWidgetState.widgetSavedData.Destroy();
			}
		}
	}
	else if (memberTypeDesc->typeId == sgeTypeId(SphericalRotation)) {
		SphericalRotation& v3ref = *(SphericalRotation*)(pMember);
		SphericalRotation v3edit;
		v3edit.aroundY = rad2deg(v3ref.aroundY);
		v3edit.fromY = rad2deg(v3ref.fromY);


		bool change = false;


		ImGuiEx::BeginGroupPanel(memberName);
		ImGuiEx::Label("Angle Around +Y");
		bool justReleased0 = false;
		bool justActivated0 = false;
		change |= SGEImGui::DragFloats(
		    "##SphericalRotation_RotationFromY", &v3edit.aroundY, 1, &justReleased0, &justActivated0);
		ImGuiEx::Label("Angle From +Y");
		bool justReleased1 = false;
		bool justActivated1 = false;
		change |= SGEImGui::DragFloats(
		    "##SphericalRotation_RotationAroundY", &v3edit.fromY, 1, &justReleased1, &justActivated1);
		ImGuiEx::EndGroupPanel();

		bool justReleased = justReleased0 || justReleased1;
		bool justActivated = justActivated0 || justActivated1;

		v3edit.aroundY = deg2rad(v3edit.aroundY);
		v3edit.fromY = deg2rad(v3edit.fromY);

		if (justActivated) {
			g_propWidgetState.widgetSavedData.resetVariantToValue<SphericalRotation>(v3ref);
		}

		if (change) {
			v3ref = v3edit;
		}

		if (justReleased) {
			// Check if the new data is actually different, as the UI may fire a lot of updates at us.
			if (*g_propWidgetState.widgetSavedData.get<SphericalRotation>() != v3ref) {
				CmdMemberChange* cmd = new CmdMemberChange;
				cmd->setup(
				    gameObject, chain, g_propWidgetState.widgetSavedData.get<SphericalRotation>(), &v3edit, nullptr);
				inspector.appendCommand(cmd, true);

				g_propWidgetState.widgetSavedData.Destroy();
			}
		}
	}
	else if (memberTypeDesc->typeId == sgeTypeId(MultiCurve2D)) {
		MultiCurve2D& curveRef = *(MultiCurve2D*)(pMember);
		MultiCurve2DEditor(memberName, curveRef);
	}
	else if (memberTypeDesc->typeId == sgeTypeId(int)) {
		ProperyEditorUIGen::editInt(inspector, memberName, gameObject, chain);
	}
	else if (memberTypeDesc->typeId == sgeTypeId(ObjectId)) {
		ImGuiEx::Label(memberName);
		const ObjectId& originalValue = *(ObjectId*)(pMember);
		ObjectId newValue = originalValue;
		bool change = actorPicker("##actorPicker", *gameObject->getWorld(), newValue);
		if (change) {
			CmdMemberChange* cmd = new CmdMemberChange;
			cmd->setup(gameObject, chain, &originalValue, &newValue, nullptr);
			inspector.appendCommand(cmd, true);
		}
	}
	else if (memberTypeDesc->typeId == sgeTypeId(std::string)) {
		ImGuiEx::Label(memberName);
		editString(inspector, "##editString", gameObject, chain);
	}
	else if (memberTypeDesc->enumUnderlayingType.isValid()) {
		int enumAsInt;

		if (memberTypeDesc->enumUnderlayingType == sgeTypeId(int))
			enumAsInt = *(int*)pMember;
		else if (memberTypeDesc->enumUnderlayingType == sgeTypeId(unsigned))
			enumAsInt = *(unsigned*)pMember;
		else if (memberTypeDesc->enumUnderlayingType == sgeTypeId(short))
			enumAsInt = *(int*)pMember;
		else if (memberTypeDesc->enumUnderlayingType == sgeTypeId(unsigned short))
			enumAsInt = *(unsigned short*)pMember;

		int enumElemIdx = memberTypeDesc->enumValueToNameLUT.find_element_index(enumAsInt);
		if (enumElemIdx >= 0 && memberTypeDesc->enumUnderlayingType == sgeTypeId(int)) {
			bool (*comboBoxEnumNamesGetter)(void* data, int idx, const char** out_text) =
			    [](void* data, int idx, const char** out_text) -> bool {
				const TypeDesc* const enumTypeDesc = (TypeDesc*)data;
				if (enumTypeDesc == nullptr)
					return false;
				if (idx >= enumTypeDesc->enumValueToNameLUT.size())
					return false;

				const char* const* foundElement = enumTypeDesc->enumValueToNameLUT.find_element(idx);
				if (foundElement == nullptr)
					return false;

				*out_text = *foundElement;
				return true;
			};

			ImGuiEx::Label(memberName);
			bool const changed = ImGui::Combo(
			    "##Combo",
			    &enumElemIdx,
			    comboBoxEnumNamesGetter,
			    (void*)(const_cast<TypeDesc*>(memberTypeDesc)),
			    int(memberTypeDesc->enumValueToNameLUT.size()));

			if (changed) {
				int const originalValue = *(int*)pMember;
				int const newValue = memberTypeDesc->enumValueToNameLUT.getAllKeys()[enumElemIdx];
				*(int*)(pMember) = newValue;

				CmdMemberChange* const cmd = new CmdMemberChange();
				cmd->setup(gameObject, chain, &originalValue, pMember, nullptr);
				inspector.appendCommand(cmd, false);
			}
		}
		else {
			ImGui::Text(
			    "%s is of type enum %s with unknown value",
			    memberName,
			    typeLib().find(memberTypeDesc->enumUnderlayingType)->name);
		}
	}
	else if (memberTypeDesc->members.size() != 0) {
		const bool showCollapsingHeader = (flags & DoMemberUIFlags_structInNoHeader) == 0;

		bool shouldDoUI = false;
		if (showCollapsingHeader) {
			char headerName[256];
			sge_snprintf(headerName, SGE_ARRSZ(headerName), "%s of %s", memberName, memberTypeDesc->name);
			shouldDoUI =
			    ImGui::CollapsingHeader(headerName, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_FramePadding);
		}
		else {
			shouldDoUI = true;
		}

		if (shouldDoUI) {
			ImGuiEx::IndentGuard indentCollapsHeaderCotent;
			for (auto& membersMember : memberTypeDesc->members) {
				if (membersMember.isEditable()) {
					MemberChain memberChain = chain;
					memberChain.add(&membersMember);
					doMemberUI(inspector, gameObject, memberChain);
				}
			}
			ImGui::Separator();
		}
	}
	else if (memberTypeDesc->stdVectorUnderlayingType.isValid()) {
		char headerName[256];
		sge_snprintf(headerName, SGE_ARRSZ(headerName), "%s of %s", memberName, memberTypeDesc->name);
		if (ImGui::CollapsingHeader(headerName, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_FramePadding)) {
			ImGuiEx::IndentGuard indentCollapsHeaderCotent;
			int const numElements = int(memberTypeDesc->stdVectorSize(pMember));
			int elemRemovedIdx = -1;

			for (int t = 0; t < numElements; ++t) {
				MemberChain memberChain = chain;
				memberChain.knots.back().arrayIdx = t;

				ImGuiEx::IDGuard elemIdGuard(t);
				doMemberUI(inspector, gameObject, memberChain);
				if (ImGui::Button(ICON_FK_TRASH)) {
					elemRemovedIdx = t;
				}
			}

			bool elemAdded = ImGui::Button(ICON_FK_PLUS);

			if (elemAdded || elemRemovedIdx != -1) {
				std::unique_ptr<char[]> newVector(new char[memberTypeDesc->sizeBytes]);

				memberTypeDesc->constructorFn(newVector.get());
				memberTypeDesc->copyFn(newVector.get(), pMember);

				if (elemAdded) {
					memberTypeDesc->stdVectorResize(newVector.get(), numElements + 1);
				}
				else if (elemRemovedIdx != -1) {
					memberTypeDesc->stdVectorEraseAtIndex(newVector.get(), elemRemovedIdx);
				}

				CmdMemberChange* const cmd = new CmdMemberChange;
				cmd->setup(gameObject, chain, pMember, newVector.get(), nullptr);
				inspector.appendCommand(cmd, true);
			}
		}
	}
	else {
		const char* const typeName =
		    typeLib().find(memberTypeDesc->typeId) ? typeLib().find(memberTypeDesc->typeId)->name : "<Unknown>";

		ImGui::Text("Non editable value '%s' of type '%s'", memberName, typeName);
	}
}
void ProperyEditorUIGen::editFloat(GameInspector& inspector, const char* label, GameObject* actor, MemberChain chain)
{
	const MemberDesc* const mdMayBeNull = chain.getMemberDescIfNotIndexing();

	float* const pActorsValue = (float*)chain.follow(actor);
	float v = *pActorsValue;

	bool justReleased = false;
	bool justActivated = false;

	ImGuiEx::Label(label);
	if (chain.knots.back().mfd->flags & MFF_FloatAsDegrees) {
		v = rad2deg(v);

		if (mdMayBeNull == nullptr) {
			SGEImGui::DragFloats(label, &v, 1, &justReleased, &justActivated, 0.1f);
		}
		else {
			SGEImGui::DragFloats(
			    label,
			    &v,
			    1,
			    &justReleased,
			    &justActivated,
			    0.f,
			    mdMayBeNull->sliderSpeed_float,
			    mdMayBeNull->min_float,
			    mdMayBeNull->max_float,
			    "%3.f Degrees");
		}

		v = deg2rad(v);
	}
	else if (chain.knots.back().mfd->flags & MFF_FloatAsDegrees) {
		v = v * 100.f;

		if (mdMayBeNull == nullptr) {
			SGEImGui::DragFloats(label, &v, 1, &justReleased, &justActivated, 0.1f);
		}
		else {
			SGEImGui::DragFloats(
			    label,
			    &v,
			    1,
			    &justReleased,
			    &justActivated,
			    0.f,
			    mdMayBeNull->sliderSpeed_float,
			    mdMayBeNull->min_float,
			    mdMayBeNull->max_float,
			    "%.3f %%");
		}

		v = v * 0.01f;
	}
	else {
		if (mdMayBeNull == nullptr) {
			SGEImGui::DragFloats(label, &v, 1, &justReleased, &justActivated, 0.1f);
		}
		else {
			SGEImGui::DragFloats(
			    label,
			    &v,
			    1,
			    &justReleased,
			    &justActivated,
			    0.f,
			    mdMayBeNull->sliderSpeed_float,
			    mdMayBeNull->min_float,
			    mdMayBeNull->max_float);
		}
	}

	if (justActivated) {
		g_propWidgetState.widgetSavedData.resetVariantToValue<float>(*pActorsValue);
	}

	*pActorsValue = v;

	if (justReleased) {
		if (g_propWidgetState.widgetSavedData.isVariantSetTo<float>() &&
		    g_propWidgetState.widgetSavedData.as<float>() != v) {
			CmdMemberChange* cmd = new CmdMemberChange;
			cmd->setup(actor, chain, g_propWidgetState.widgetSavedData.get<float>(), &v, nullptr);
			inspector.appendCommand(cmd, true);

			g_propWidgetState.widgetSavedData.Destroy();
		}
	}
}
void ProperyEditorUIGen::editInt(GameInspector& inspector, const char* label, GameObject* actor, MemberChain chain)
{
	bool justReleased = 0;
	bool justActivated = 0;

	const MemberDesc* mdMayBeNull = chain.getMemberDescIfNotIndexing();

	int* const pActorsValue = (int*)chain.follow(actor);
	int v = *pActorsValue;


	ImGuiEx::IDGuard actorValueIDGuard(pActorsValue);

	ImGuiEx::Label(label);

	if (mdMayBeNull == nullptr) {
		SGEImGui::DragInts("##EditInt", &v, 1, &justReleased, &justActivated);
	}
	else {
		SGEImGui::DragInts(
		    "##EditInt",
		    &v,
		    1,
		    &justReleased,
		    &justActivated,
		    mdMayBeNull->sliderSpeed_float,
		    mdMayBeNull->min_int,
		    mdMayBeNull->max_int);
	}

	if (justActivated) {
		g_propWidgetState.widgetSavedData.resetVariantToValue<int>(*pActorsValue);
	}

	*pActorsValue = v;

	if (justReleased) {
		if (g_propWidgetState.widgetSavedData.isVariantSetTo<int>() &&
		    g_propWidgetState.widgetSavedData.as<int>() != v) {
			CmdMemberChange* cmd = new CmdMemberChange;
			cmd->setup(actor, chain, g_propWidgetState.widgetSavedData.get<int>(), &v, nullptr);
			inspector.appendCommand(cmd, true);

			g_propWidgetState.widgetSavedData.Destroy();
		}
	}
}
void ProperyEditorUIGen::editString(
    GameInspector& inspector, const char* label, GameObject* gameObject, MemberChain chain)
{
	std::string& srcString = *(std::string*)chain.follow(gameObject);

	std::string stringEdit = srcString;

	bool const change = ImGuiEx::InputText(label, stringEdit, ImGuiInputTextFlags_EnterReturnsTrue);
	if (change) {
		std::string newData = stringEdit;
		CmdMemberChange* const cmd = new CmdMemberChange;
		cmd->setup(gameObject, chain, &srcString, &newData, nullptr);
		inspector.appendCommand(cmd, true);

		srcString = stringEdit;
	}
}

void ProperyEditorUIGen::editStringAsAssetPath(
    GameInspector& inspector,
    const char* label,
    GameObject* gameObject,
    MemberChain chain,
    const AssetIfaceType possibleAssetIfaceTypes[],
    const int numPossibleAssetIfaceTypes)
{
	std::string& srcString = *(std::string*)chain.follow(gameObject);
	std::string stringEdit = srcString;

	bool const change =
	    assetPicker(label, stringEdit, getCore()->getAssetLib(), possibleAssetIfaceTypes, numPossibleAssetIfaceTypes);

	if (change) {
		std::string newData = stringEdit;
		CmdMemberChange* const cmd = new CmdMemberChange;
		cmd->setup(gameObject, chain, &srcString, &newData, nullptr);
		inspector.appendCommand(cmd, true);

		srcString = stringEdit;
	}
}

void ProperyEditorUIGen::editDynamicProperties(
    GameInspector& inspector, GameObject* gameObject, MemberChain chainToDynamicProps)
{
	DynamicProperties* dynPops = (DynamicProperties*)chainToDynamicProps.follow(gameObject);

	for (auto& itrProp : dynPops->m_properties) {
		ImGuiEx::Label(itrProp.first.c_str());
		DynamicProperties::Property& prop = itrProp.second;

		if (prop.type == sgeTypeId(int)) {
			int& data = prop.getRef<int>();
			int dataForEdit = data;

			bool justReleased = false;
			bool justActivated = false;
			bool change = SGEImGui::DragInts("##DragInts", &dataForEdit, 1, &justReleased, &justActivated);

			if (justActivated) {
				g_propWidgetState.widgetSavedData.resetVariantToValue<int>(data);
			}

			if (change) {
				data = dataForEdit;
			}

			if (justReleased) {
				// Check if the new data is actually different, as the UI may fire a lot of updates at us.
				if (*g_propWidgetState.widgetSavedData.get<int>() != data) {
					CmdDynamicProperyChanged* cmd = new CmdDynamicProperyChanged;
					cmd->setup(
					    gameObject,
					    chainToDynamicProps,
					    itrProp.first,
					    g_propWidgetState.widgetSavedData.get<int>(),
					    &dataForEdit);
					inspector.appendCommand(cmd, true);

					g_propWidgetState.widgetSavedData.Destroy();
				}
			}
		}
	}
}

//----------------------------------------------------------
// PropertyEditorWindow
//----------------------------------------------------------
void PropertyEditorWindow::update(
    SGEContext* const UNUSED(sgecon), struct GameInspector* inspector, const InputState& UNUSED(is))
{
	if (isClosed()) {
		return;
	}

	if (ImGui::Begin(m_windowName.c_str(), &m_isOpened, ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
		GameObject* gameObject = !inspector->m_selection.empty()
		                           ? inspector->getWorld()->getObjectById(inspector->m_selection[0].objectId)
		                           : nullptr;
		if (gameObject == nullptr) {
			ImGui::TextUnformatted("Meke a selection!");
		}
		else {
			if (ImGui::BeginCombo("Primary Selection", gameObject->getDisplayNameCStr())) {
				for (int t = 0; t < inspector->m_selection.size(); ++t) {
					GameObject* actorFromSel = inspector->getWorld()->getObjectById(inspector->m_selection[t].objectId);
					if (actorFromSel) {
						ImGuiEx::IDGuard idGuard(t);
						if (ImGui::Selectable(actorFromSel->getDisplayNameCStr(), gameObject == actorFromSel)) {
							std::swap(inspector->m_selection[t], inspector->m_selection[0]);
							gameObject = actorFromSel;
						}
					}
				}

				ImGui::EndCombo();
			}

			ImGui::Separator();

			// Show all the members of the actors structure.
			ProperyEditorUIGen::doGameObjectUI(*inspector, gameObject);
		}
	}
	ImGui::End();
}
} // namespace sge
