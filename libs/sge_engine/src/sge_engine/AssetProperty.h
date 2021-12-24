#pragma once

#include "sge_core/AssetLibrary/AssetLibrary.h"
#include "sge_engine_api.h"
#include "sge_utils/ChangeIndex.h"
#include "sge_utils/utils/StaticArray.h"

namespace sge {

/// @brief A Helper class usually used as a property to game object.
/// This is a reference to an asset, 
/// by using the @update() method you can check if new asset has been assigned.
struct SGE_ENGINE_API AssetProperty {
	/// Asset propery needs a list of accepted assets.
	/// Please use one of the other constructors.
	AssetProperty() = default;

	AssetProperty(const AssetProperty& ref);
	AssetProperty& operator=(const AssetProperty& ref);

	explicit AssetProperty(AssetIfaceType assetType) {
		m_acceptedAssetIfaceTypes.push_back(assetType);
	}

	AssetProperty(AssetIfaceType assetType0, AssetIfaceType assetType1) {
		m_acceptedAssetIfaceTypes.push_back(assetType0);
		m_acceptedAssetIfaceTypes.push_back(assetType1);
	}

	AssetProperty(AssetIfaceType assetType0, AssetIfaceType assetType1, AssetIfaceType assetType2) {
		m_acceptedAssetIfaceTypes.push_back(assetType0);
		m_acceptedAssetIfaceTypes.push_back(assetType1);
		m_acceptedAssetIfaceTypes.push_back(assetType2);
	}

	AssetProperty(AssetIfaceType assetType, const char* const initialAsset) {
		m_acceptedAssetIfaceTypes.push_back(assetType);
		setAsset(initialAsset);
	}

	

	/// @brief Should be called by the user.
	/// @return True if new asset was assigned.
	bool update();

	void clear() {
		m_assetChangeIndex.markAChange();
		m_asset = nullptr;
	}

	/// @brief Changes the asset.
	void setAsset(AssetPtr newAsset);

	/// @brief Changes the asset.
	void setAsset(const char* newAssetPath);

	/// Retrienves the current asset.
	AssetPtr& getAsset() {
		return m_asset;
	}

	/// Retrienves the current asset.
	const AssetPtr& getAsset() const {
		return m_asset;
	}

	/// Returns the asset interface of the currently assigned asset
	/// if the asset is loaded and supports the specifiedi nterface.
	template <typename TAssetIface>
	TAssetIface* getAssetInterface() {
		return getLoadedAssetIface<TAssetIface>(m_asset);
	}

	/// Returns the asset interface of the currently assigned asset
	/// if the asset is loaded and supports the specifiedi nterface.
	template <typename TAssetIface>
	const TAssetIface* getAssetInterface() const {
		return getLoadedAssetIface<TAssetIface>(m_asset);
	}

	/// Returns the change index of the asset.
	/// Useful if traking for changes via @update method isn't viable.
	const ChangeIndex& getChangeIndex() const {
		return m_assetChangeIndex;
	}

  public:
	AssetPtr m_asset;
	ChangeIndex m_assetChangeIndex;

	/// A list of all accepted asset interfaces by this property.
	/// Caution: there is a custom copy logic for @m_acceptedAssetIfaceTypes.
	StaticArray<AssetIfaceType, int(assetIface_count)> m_acceptedAssetIfaceTypes;
	std::vector<std::string> m_uiPossibleAssets;
};

} // namespace sge
