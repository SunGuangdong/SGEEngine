#pragma once

#include "sge_core/Animator.h"
#include "sge_core/shaders/modeldraw.h"
#include "sge_engine/Actor.h"
#include "sge_engine/AssetProperty.h"
#include "sge_engine/GameDrawer/IRenderItem.h"


namespace sge {

struct ICamera;
struct TraitModel;

struct TraitModelRenderItem : public IRenderItem {
	TraitModel* traitModel = nullptr;
	const EvaluatedModel* evalModel = nullptr;
	int iEvalNode = -1; // The mesh to be rendered from the model.
	int iEvalNodeMechAttachmentIndex = -1;
};

/// @brief TraitModel is a trait designed to be attached in an Actor.
/// It provides a simple way to assign a renderable 3D Model to the game object (both animated and static).
/// The trait is not automatically updateable, the user needs to manually call @postUpdate() method in their objects.
/// This is because not all game object need this complication of auto updating.
///
/// For Example:
///
///    Lets say that you have a actor that is some collectable, a coin from Super Mario.
///    That coin 3D model is never going to change, you know that the game object is only going to use
///    that one specfic 3D model that you can set in @Actor::create method with @TraitModel::setModel() and forget about it.
///    You do not need any upadates on it, nor you want to be able to change the 3D model from the Property Editor Window.
///    In this situation you just add the trait, set the model and you are done.
///
///    The situation where the 3D model might change is for example with some Decore.
///    Lets say that your 3D artist has prepared a grass and bush models that you want to scatter around the level.
///    It would be thedious to have a separate game object for each 3D model.
///    Instead you might do what the @AStaticObstacle does, have the model be changed via Propery Editor Window.
///    In that way you have I generic actor type that could be configured to your desiers.
///    In order for the game object to take into account the change you need in your Actor::postUpdate to
///    to update the trait, see if the model has been changed and maybe update the rigid body for that actor.
DefineTypeIdExists(TraitModel);
struct SGE_ENGINE_API TraitModel : public Trait {
	SGE_TraitDecl_Full(TraitModel);

	TraitModel()
	    : m_assetProperty(AssetType::Model) {}

	void setModel(const char* assetPath, bool updateNow) {
		m_assetProperty.setTargetAsset(assetPath);
		if (updateNow) {
			postUpdate();
		}
	}

	void setModel(std::shared_ptr<Asset>& asset, bool updateNow) {
		m_assetProperty.setAsset(asset);
		m_evalModel = NullOptional();
		if (updateNow) {
			updateAssetProperty();
		}
	}

	/// Not called automatically see the class comment above.
	/// Updates the working model.
	/// Returns true if the model has been changed (no matter if it is valid or not).
	bool postUpdate() { return updateAssetProperty(); }

	/// Invalidates the asset property focing an update.
	void clear() { m_assetProperty.clear(); }

	AssetProperty& getAssetProperty() { return m_assetProperty; }
	const AssetProperty& getAssetProperty() const { return m_assetProperty; }

	mat4f getAdditionalTransform() const { return m_additionalTransform; }
	void setAdditionalTransform(const mat4f& tr) { m_additionalTransform = tr; }

	AABox3f getBBoxOS() const;

	void setRenderable(bool v) { isRenderable = v; }
	bool getRenderable() const { return isRenderable; }
	void setNoLighting(bool v) { instanceDrawMods.forceNoLighting = v; }
	bool getNoLighting() const { return instanceDrawMods.forceNoLighting; }

	void computeNodeToBoneIds();
	void computeSkeleton(std::vector<mat4f>& boneOverrides);

	void getRenderItems(std::vector<IRenderItem*>& renderItems);

  private:
	bool updateAssetProperty() {
		if (m_assetProperty.update()) {
			m_evalModel = NullOptional();
			return true;
		}
		return false;
	}

	void onModelChanged() {
		useSkeleton = false;
		rootSkeletonId = ObjectId();
		nodeToBoneId.clear();
	}

  public:
	struct MaterialOverride {
		std::string materialName;
		ObjectId materialObjId;
	};

  public:
	bool isRenderable = true;
	AssetProperty m_assetProperty;
	mat4f m_additionalTransform = mat4f::getIdentity();

	// Used when the trait is going to render an animated model.
	// This hold the evaluated 3D model to be rendered.
	Optional<EvaluatedModel> m_evalModel;
	InstanceDrawMods instanceDrawMods;

	std::vector<MaterialOverride> m_materialOverrides;

	// External skeleton, useful for IK. Not sure for regular skinned meshes.
	bool useSkeleton = false;
	ObjectId rootSkeletonId;
	std::unordered_map<int, ObjectId> nodeToBoneId;

	std::vector<TraitModelRenderItem> m_tempRenderItems;
};

} // namespace sge
