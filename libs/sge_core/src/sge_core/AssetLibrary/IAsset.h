#pragma once

#include "IAssetInterface.h"
#include <memory>

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
		sint64 lastAcessTime = 0; ///< The the modification time of file when we last tried to loaded it (no matter if we succeeded or not).
	};

  protected:
	std::string m_assetPath;
	AssetStatus m_status = AssetStatus_NotLoaded;
	AssetLibrary& m_ownerAssetLib;
	LoadAssetFromFileData m_loadAssetFromFileData;
};

template <typename TAssetInterface>
TAssetInterface* getAssetIface(std::shared_ptr<Asset>& asset) {
	if (isAssetLoaded(asset)) {
		return dynamic_cast<TAssetInterface*>(asset.get());
	}

	return nullptr;
}

template <typename TAssetInterface>
const TAssetInterface* getAssetIface(const std::shared_ptr<Asset>& asset) {
	if (isAssetLoaded(asset)) {
		return dynamic_cast<const TAssetInterface*>(asset.get());
	}

	return nullptr;
}

template <typename TAssetInterface>
TAssetInterface* getAssetIface(Asset& asset) {
	if (isAssetLoaded(asset)) {
		return dynamic_cast<TAssetInterface*>(&asset);
	}

	return nullptr;
}

template <typename TAssetInterface>
const TAssetInterface* getAssetIface(const Asset& asset) {
	if (isAssetLoaded(asset)) {
		return dynamic_cast<const TAssetInterface*>(&asset);
	}

	return nullptr;
}


inline bool isAssetLoaded(const Asset& asset) {
	bool loaded = asset.getStatus() == AssetStatus_Loaded;
	return loaded;
}

inline bool isAssetLoaded(const std::shared_ptr<Asset>& asset) {
	bool loaded = asset && isAssetLoaded(*asset);
	return loaded;
}


} // namespace sge
