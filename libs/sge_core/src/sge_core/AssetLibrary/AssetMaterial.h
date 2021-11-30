#pragma once

#include "IAsset.h"
#include "IAssetInterface.h"
#include "sge_core/materials/IMaterial.h"

namespace sge {

struct SGE_CORE_API AssetIface_Material : public IAssetInterface {
	AssetIface_Material() = default;
	virtual ~AssetIface_Material() = default;
	virtual std::shared_ptr<IMaterial> getMaterial() const = 0;
};

struct SGE_CORE_API AssetIface_Material_Simple : public AssetIface_Material {
	std::shared_ptr<IMaterial> getMaterial() const override {
		return mtl;
	}

	std::shared_ptr<IMaterial> mtl;
};

struct SGE_CORE_API AssetMaterial : public Asset, public AssetIface_Material {
	AssetMaterial(std::string assetPath, AssetLibrary& ownerAssetLib)
	    : Asset(assetPath, ownerAssetLib) {
	}

	std::shared_ptr<IMaterial> getMaterial() const override {
		return m_material;
	}

	bool loadAssetFromFile(const char* const path) override;
	bool saveAssetToFile(const char* const path) const;

  public:
	std::shared_ptr<IMaterial> m_material;
};

} // namespace sge
