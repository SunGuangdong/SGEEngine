#pragma once

#include <memory>

#include "IAsset.h"

namespace sge {

struct Texture;

struct AssetTextureMeta {
	/// The sampler description defined by the asset file.
	/// This is no necessery the actual descriptor assigned to @tex.
	SamplerDesc assetSamplerDesc;

	/// True if the texture is semi-trasparent. That means that there are alpha values != for zero or one.
	bool isSemiTransparent = false;

	/// True if mip maps need to be generated when the texture gets loaded.
	bool shouldGenerateMips = true;
};

struct SGE_CORE_API AssetIface_Texture2D : public IAssetInterface {
	AssetIface_Texture2D() = default;
	virtual ~AssetIface_Texture2D() = default;

	virtual Texture* getTexture() = 0;
	virtual Texture* getTexture() const = 0;
	virtual const AssetTextureMeta& getTextureMeta() const = 0;
	virtual void setTextureMeta(const AssetTextureMeta& newMeta) = 0;
};

struct SGE_CORE_API AssetTexture2d : public Asset, public AssetIface_Texture2D {
	AssetTexture2d(std::string assetPath, AssetLibrary& ownerAssetLib)
	    : Asset(assetPath, ownerAssetLib) {
	}

	Texture* getTexture() override {
		return m_texture.GetPtr();
	}

	Texture* getTexture() const override {
		return m_texture.GetPtr();
	}

	const AssetTextureMeta& getTextureMeta() const override {
		return m_textureMeta;
	}

	void setTextureMeta(const AssetTextureMeta& newMeta) override {
		m_textureMeta = newMeta;
	}

	/// Loads the specified asset form the specified path.
	bool loadAssetFromFile(const char* const path) override;

	bool saveTextureSettingsToInfoFile() const;

  private:
	enum DDSLoadCode {
		ddsLoadCode_fine = 0,
		ddsLoadCode_fileDoesntExist,
		ddsLoadCode_importOrCreationFailed,
	};

	DDSLoadCode loadDDS(const char* const rawPath);

  public:
	GpuHandle<Texture> m_texture;
	AssetTextureMeta m_textureMeta;
};

} // namespace sge
