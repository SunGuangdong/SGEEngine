#pragma once

#include "sge_core/materials/IGeometryDrawer.h"
#include "sge_core/model/EvaluatedModel.h"
#include "sge_engine/Actor.h"
#include "sge_engine/AssetProperty.h"
#include "sge_engine/GameDrawer/IRenderItem.h"
#include "sge_utils/containers/Optional.h"
#include "sge_utils/react/ChangeIndex.h"

namespace sge {

struct ICamera;
struct TraitModel;
struct GeometryRenderItem;

/// ModelEntry is used to assign an AssetIface_Model3D model to TraitModel
/// to get rendered. These use the evaluated state provided by this interface and cannot be animated.
/// For animated models use @CustomModelEntry.
struct SGE_ENGINE_API ModelEntry {
	ModelEntry()
	    : m_assetProperty(assetIface_model3d)
	{
	}

	/// Invalidates the asset property focing an update.
	void invalidateCachedAssets() { m_assetProperty.clear(); }

	bool updateAssetProperty()
	{
		if (m_assetProperty.update()) {
			changeIndex.markAChange();
			onAssetModelChanged();
		}
		return changeIndex.checkForChangeAndUpdate();
	}

	void setModel(const char* assetPath, bool setupCustomEvalState = false);
	void setModel(AssetPtr& asset, bool setupCustomEvalState = false);

	/// Computes the bounding box of the attached 3D model.
	/// Including the m_additionalTransform,
	/// @worldTrasnform is used to compute correctly the bounding box
	/// when ignoreActorTransform is used.
	Box3f getBBoxOS(const mat4f& invWorldTrasnform) const;

  private:
	/// Called when the asset model has been changed.
	void onAssetModelChanged();

  public:
	ChangeIndex changeIndex;

	bool isRenderable = true;
	bool ignoreActorTransform = false; ///< true, when the actor transform should be ignored when rendering the 3D model.
	AssetProperty m_assetProperty;
	mat4f m_additionalTransform = mat4f::getIdentity();
	std::vector<std::shared_ptr<AssetIface_Material>> mtlOverrides;

	// Used when the trait is going to render an animated model.
	// This holds the evaluated 3D model to be rendered and if specified
	// the @m_assetProperty will be compleatly ignored.
	// This one is not serializable and does not appear in the user interface in any shape of form.
	Optional<EvaluatedModel> customEvalModel;
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
///    In that way you have a generic actor type that could be configured to your desiers.
///    In order for the game object to take into account the change you need in your Actor::postUpdate to
///    to update the trait, see if the model has been changed and maybe update the rigid body for that actor.
ReflAddTypeIdExists(TraitModel);
struct SGE_ENGINE_API TraitModel : public Trait {
	SGE_TraitDecl_Full(TraitModel);

	TraitModel() = default;

	void addModel(const char* assetPath, bool setupCustomEvalState = false);
	void addModel(AssetPtr& asset, bool setupCustomEvalState = false);
	void clearModels() { m_models.clear(); }

	/// Not called automatically see the class comment above.
	/// Updates the working models.
	/// Returns true if a model has been changed (no matter if it is valid or not),
	/// useful if other sytems might depend on it.
	bool postUpdate() { return updateAssetProperties(); }

	/// Returns the bounding box of all models.
	Box3f getBBoxOS() const;

	void getRenderItems(DrawReason drawReason, std::vector<GeometryRenderItem>& renderItems);

	void invalidateCachedAssets();

  private:
	bool updateAssetProperties();

  public:
	std::vector<ModelEntry> m_models;          ///< A list of all models in their settings to rendered by the trait.
	bool isRenderable = true;                  ///< True if the whole trait is renderable.
	bool uiDontOfferResizingModelCount = true; ///< if true the interface will not offer adding/removing more models to the trait.
	bool forceNoShadows = false;
};

} // namespace sge
