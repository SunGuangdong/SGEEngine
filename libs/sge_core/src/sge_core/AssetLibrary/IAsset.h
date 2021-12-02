#pragma once

#include "IAssetInterface.h"
#include "sge_utils/sge_utils.h"
#include <memory>
#include <string>

namespace sge {

struct AssetLibrary;

/// @brief Describes the current status of an asset.
enum AssetStatus : int {
	AssetStatus_NotLoaded,  ///< The assets seems to exist but it is not loaded.
	AssetStatus_Loaded,     ///< The asset is loaded.
	AssetStatus_LoadFailed, ///< Loading the asset failed. Maybe the files is broken or it does not exist.
};

struct SGE_CORE_API Asset {
	friend AssetLibrary;

	Asset(std::string assetPath, AssetLibrary& ownerAssetLib)
	    : m_assetPath(std::move(assetPath))
	    , m_ownerAssetLib(ownerAssetLib) {
	}
	virtual ~Asset() = default;

	AssetStatus getStatus() const {
		return m_status;
	}

	const std::string& getPath() const {
		return m_assetPath;
	}

	virtual bool loadAssetFromFile(const char* filePath) = 0;

  public:
	struct LoadAssetFromFileData {
		/// The the modification time of file when we last tried to loaded it (no matter if we succeeded or not).
		sint64 lastAcessTime = 0;
	};

  protected:
	std::string m_assetPath;
	AssetStatus m_status = AssetStatus_NotLoaded;
	AssetLibrary& m_ownerAssetLib;
	LoadAssetFromFileData m_loadAssetFromFileData;
};

using AssetPtr = std::shared_ptr<Asset>;
using AssetWeak = std::weak_ptr<Asset>;

/// If the specified asset is loaded, the function
/// retrieves the specified asset interface if available.
/// If you just want to cast this to the interface type without caring 
/// if the asset is loaded or not use std::dynamic_pointer_cast.
template <typename TAssetInterface>
TAssetInterface* getLoadedAssetIface(AssetPtr& asset) {
	if (isAssetLoaded(asset)) {
		return dynamic_cast<TAssetInterface*>(asset.get());
	}

	return nullptr;
}

/// If the specified asset is loaded, the function
/// retrieves the specified asset interface if available.
/// If you just want to cast this to the interface type without caring
/// if the asset is loaded or not use std::dynamic_pointer_cast.
template <typename TAssetInterface>
const TAssetInterface* getLoadedAssetIface(const AssetPtr& asset) {
	if (isAssetLoaded(asset)) {
		return dynamic_cast<const TAssetInterface*>(asset.get());
	}

	return nullptr;
}

/// If the specified asset is loaded, the function
/// retrieves the specified asset interface if available.
/// If you just want to cast this to the interface type without caring
/// if the asset is loaded or not use std::dynamic_pointer_cast.
template <typename TAssetInterface>
TAssetInterface* getLoadedAssetIface(Asset& asset) {
	if (isAssetLoaded(asset)) {
		return dynamic_cast<TAssetInterface*>(&asset);
	}

	return nullptr;
}

/// If the specified asset is loaded, the function
/// retrieves the specified asset interface if available.
/// If you just want to cast this to the interface type without caring
/// if the asset is loaded or not use std::dynamic_pointer_cast.
template <typename TAssetInterface>
const TAssetInterface* getLoadedAssetIface(const Asset& asset) {
	if (isAssetLoaded(asset)) {
		return dynamic_cast<const TAssetInterface*>(&asset);
	}

	return nullptr;
}
/// Returns true if the specified asset is loaded.
inline bool isAssetLoaded(const Asset& asset) {
	bool loaded = asset.getStatus() == AssetStatus_Loaded;
	return loaded;
}

/// Returns true if the specified asset is loaded.
inline bool isAssetLoaded(const AssetPtr& asset) {
	bool loaded = asset && isAssetLoaded(*asset);
	return loaded;
}

} // namespace sge
