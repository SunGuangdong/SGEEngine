#pragma once

#include "sge_core/AssetLibrary/AssetLibrary.h"
#include "sge_engine_api.h"
#include "sge_utils/utils/StaticArray.h"

namespace sge {

/// @brief A Helper class usually used as a property to game object.
/// This is a reference to an asset, by using the @update() method you can check if new asset has been assigned.
struct SGE_ENGINE_API AssetProperty {
	AssetProperty() = delete;

	explicit AssetProperty(AssetType assetType) {
		m_acceptedAssetTypes.push_back(assetType);
	}

	AssetProperty(AssetType assetType0, AssetType assetType1) {
		m_acceptedAssetTypes.push_back(assetType0);
		m_acceptedAssetTypes.push_back(assetType1);
	}

	AssetProperty(AssetType assetType0, AssetType assetType1, AssetType assetType2) {
		m_acceptedAssetTypes.push_back(assetType0);
		m_acceptedAssetTypes.push_back(assetType1);
		m_acceptedAssetTypes.push_back(assetType2);
	}

	AssetProperty(AssetType assetType, const char* const initialTargetAsset) {
		m_acceptedAssetTypes.push_back(assetType);
		setTargetAsset(initialTargetAsset);
	}

	/// @brief Should be called by the user. Checks if the target asset has changed and if its loads the new asset.
	/// @return True if new asset was assigned.
	bool update();

	bool isUpToDate() const {
		return m_targetAsset == m_currentAsset;
	}

	void clear() {
		m_currentAsset.clear();
		m_asset = nullptr;
	}

	/// @brief Changes directly the current asset.
	/// @param asset
	void setAsset(std::shared_ptr<Asset> asset);

	/// @brief Sets the asset that will generate a change when calling @update.
	/// Useful when others depend on knowing what this asset is.
	/// @param assetPath
	void setTargetAsset(const char* const assetPath);

	std::shared_ptr<Asset>& getAsset() {
		return m_asset;
	}
	const std::shared_ptr<Asset>& getAsset() const {
		return m_asset;
	}

	template <typename TAssetIface>
	TAssetIface* getAssetInterface() {
		return getAssetIface<TAssetIface>(m_asset);
	}

	template <typename TAssetIface>
	const TAssetIface* getAssetInterface() const {
		return getAssetIface<TAssetIface>(m_asset);
	}

	AssetProperty& operator=(const AssetProperty& ref) {
		m_targetAsset = ref.m_targetAsset;
		m_acceptedAssetTypes = ref.m_acceptedAssetTypes;
		m_uiPossibleAssets = ref.m_uiPossibleAssets;

		// m_currentAsset not copied so when an object is duplicated it would need to force inititalize it,
		// which is needed so it could initialize its dependencies like rigid body.
		// m_asset is not copied for the same reason as m_currentAsset.
		// TODO: the above might no longer be needed.
		return *this;
	}

  public:
	// Caution: there is a custom copy logic.
	StaticArray<AssetType, int(assetType_count)> m_acceptedAssetTypes;
	std::string m_targetAsset;

	std::string m_currentAsset;
	std::shared_ptr<Asset> m_asset;
	std::vector<std::string> m_uiPossibleAssets;
};

} // namespace sge
