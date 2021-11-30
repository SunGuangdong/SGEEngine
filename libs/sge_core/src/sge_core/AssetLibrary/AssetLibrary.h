#pragma once

#include <map>
#include <memory>
#include <string>

#include "sge_core/sgecore_api.h"
#include "sge_utils/sge_utils.h"

#include "AssetAudio.h"
#include "AssetMaterial.h"
#include "AssetModel3D.h"
#include "AssetSpriteAnim.h"
#include "AssetText.h"
#include "AssetTexture2D.h"

namespace sge {

/// An enum for each asset interface that is supported.
/// If you add a new asset interface type make sure to
/// add it here and handle it where it is needed.
enum AssetIfaceType : int {
	assetIface_unknown = 0,
	assetIface_texture2d,
	assetIface_model3d,
	assetIface_spriteAnim,
	assetIface_audio,
	assetIface_text,
	assetIface_mtl,
	assetIface_count,
};

/// @brief Returns a name suitable for displaying to the user for the specified asset type.
SGE_CORE_API const char* assetIface_getName(const AssetIfaceType type);

/// @brief Guesses the potential assset type based in the specified extension. Keep in mind that this is a guess.
/// @param [in] ext the extension to be used for guessing. Should not include the dot.
///                 The comparison will be case insensitive ("txt" is the same as "TXT").
/// @param [in] includeExternalExtensions True if unconverted file types should be concidered,
///             like fbx/obj/dae could be guessed as a 3d model.
/// @return the guessed asset type.
SGE_CORE_API AssetIfaceType assetIface_guessFromExtension(const char* const ext, bool includeExternalExtensions);

/// AssetLibrary loads and bookkeeps loaded assets by it
/// and enables other assets to load dependancy asset.
/// For example a 3D model might refer to a texture via a material.
struct SGE_CORE_API AssetLibrary {
	AssetLibrary() = default;
	~AssetLibrary() = default;

	/// Sets the specified directory to be a default asset directory.
	/// While an asset could be loaded from any path (it is just a file after all)
	/// This directory is used for creating a "dir-tree" for aviable assets.
	void scanForAvailableAssets(const char* path);

	/// Returns the requested asset.
	/// The input path will internally get converted to relative path to the current working directory.
	/// This converted path will get used to identify the asset later.
	/// @localDirectory is specified, the asset would be attempted to get loaded from the local directory
	/// (basically localDirectory + path), otherwise it would fallback to the currnet-working directory.
	AssetPtr getAssetFromFile(const char* path, const char* localDirectory = nullptr, bool loadIfMissing = true);

	template <typename TAssetIface>
	std::shared_ptr<TAssetIface> getAssetIface(const char* path, const char* localDirectory = nullptr, bool loadIfMissing = true) {
		AssetPtr assetPtr = getAssetFromFile(path, localDirectory, loadIfMissing);
		if (assetPtr) {
			return std::dynamic_pointer_cast<TAssetIface>(assetPtr);
		}

		return nullptr;
	}

	/// Returns true if an asset with the specified path is allocated.
	bool hasAsset(const std::string& path) const;

	/// Retrieves the internal state of all assets.
	const std::map<std::string, AssetPtr>& getAllAssets() const;

	/// Tries to reload the specified asset. The asset will not be reloaded
	/// the the modified time of the input file hasn't changed.
	bool reloadAssetModified(AssetPtr& asset);

	template <typename TAsset>
	std::shared_ptr<TAsset> newAsset(std::string assetPath) {
		if (hasAsset(assetPath)) {
			sgeAssert(false);
			return nullptr;
		}

		auto assetPtr = std::make_shared<TAsset>(assetPath, *this);

		m_allAssets[assetPath] = assetPtr;
		return assetPtr;
	}

	const std::string getAssetsDirAbs() const {
		return m_gameAssetsDir;
	}

	void queueAssetForReload(AssetPtr& a) {
		if (a) {
			m_assetsToReload.insert(a);
		}
	}

	void reloadAssets() {
		for (AssetPtr a : m_assetsToReload) {
			a->loadAssetFromFile(a->getPath().c_str());
		}

		m_assetsToReload.clear();
	}

  private:
	/// Resolves a relative path to an asset to an relative to the current working dir.
	/// This is needed as some assets might refer to other assets relativly to them,
	/// for example a 3d model referncing a texture in it's current directory.
	/// In order to load the file onces (if other assets reference it) we need to have a unified
	/// path to these assets.
	std::string resloveAssetPathToRelative(const char* pathRaw) const;
	AssetPtr newAsset(std::string assetPath, AssetIfaceType type);

  private:
	std::string m_gameAssetsDir;
	std::map<std::string, AssetPtr> m_allAssets;
	std::set<AssetPtr> m_assetsToReload;
};

/// Returns true if the specified asset supports the specified interface.
/// The asset does not need to be loaded to return true.
SGE_CORE_API bool isAssetSupportingInteface(const Asset& asset, AssetIfaceType type);

/// Returns true if the specified asset supports the specified interface.
/// The asset does not need to be loaded to return true.
SGE_CORE_API bool isAssetSupportingInteface(const AssetPtr& asset, AssetIfaceType type);

/// Returns true if the specified asset is loaded and supports the specified interface.
SGE_CORE_API bool isAssetLoaded(const Asset& asset, AssetIfaceType type);

/// Returns true if the specified asset is loaded and supports the specified interface.
SGE_CORE_API bool isAssetLoaded(const AssetPtr& asset, AssetIfaceType type);

} // namespace sge
