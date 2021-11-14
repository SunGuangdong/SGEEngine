#pragma once

#include <map>
#include <memory>
#include <string>

#include "sge_core/sgecore_api.h"
#include "sge_utils/sge_utils.h"

#include "IAsset.h"

#include "AssetAudio.h"
#include "AssetModel3D.h"
#include "AssetSpriteAnim.h"
#include "AssetText.h"
#include "AssetTexture2D.h"


namespace sge {

struct Asset;


enum AssetType : int {
	assetType_unknown = 0,
	assetType_texture2d,
	assetType_model3d,
	assetType_spriteAnim,
	assetType_audio,
	assetType_text,

	assetType_count,
};


/// @brief Returns a name suitable for displaying to the user for the specified asset type.
SGE_CORE_API const char* assetType_getName(const AssetType type);

/// @brief Guesses the potential assset type based in the specified extension. Keep in mind that this is a guess.
/// @param [in] ext the extension to be used for guessing. Should not include the dot.
///                 The comparison will be case insensitive ("txt" is the same as "TXT").
/// @param [in] includeExternalExtensions True if unconverted file types should be concidered,
///             like fbx/obj/dae could be guessed as a 3d model.
/// @return the guessed asset type.
SGE_CORE_API AssetType assetType_guessFromExtension(const char* const ext, bool includeExternalExtensions);

/// AssetLibrary loads and bookkeeps loaded assets by it
/// and enables other assets to load dependancy asset (3D model might refer to a texture via a material).
struct SGE_CORE_API AssetLibrary {
	AssetLibrary() = default;
	~AssetLibrary() = default;

	/// Sets the specified directory to be a default asset directory.
	/// While an asset could be loaded from any path (it is just a file after all)
	/// This directory is used for creating a "dir-tree" for aviable assets.
	void scanForAvailableAssets(const char* path);

	std::shared_ptr<Asset> getAssetFromFile(const char* path, bool loadIfMissing = true);
	bool hasAsset(const std::string& path) const;
	const std::map<std::string, std::shared_ptr<Asset>>& getAllAssets() const;
	bool reloadAssetModified(std::shared_ptr<Asset>& asset);

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

  private:
	/// Resolves a relative path to an asset to an relative to the current working dir.
	/// This is needed as some assets might refer to other assets relativly to them,
	/// for example a 3d model referncing a texture in it's current directory.
	/// In order to load the file onces (if other assets reference it) we need to have a unified
	/// path to these assets.
	std::string resloveAssetPathToRelative(const char* pathRaw) const;
	std::shared_ptr<Asset> newAsset(std::string assetPath, AssetType type);

  private:
	std::string m_gameAssetsDir;
	std::map<std::string, std::shared_ptr<Asset>> m_allAssets;
};


SGE_CORE_API bool isAssetSupportingInteface(const Asset& asset, AssetType type);
SGE_CORE_API bool isAssetSupportingInteface(const std::shared_ptr<Asset>& asset, AssetType type);

SGE_CORE_API bool isAssetLoaded(const Asset& asset, AssetType type);
SGE_CORE_API bool isAssetLoaded(const std::shared_ptr<Asset>& asset, AssetType type);



} // namespace sge
