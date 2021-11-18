#pragma once

#include "IAsset.h"
#include "IAssetInterface.h"
#include "sge_core/materials/IMaterial.h"

namespace sge {

struct SGE_CORE_API AssetIface_Material : public IAssetInterface {
	virtual std::shared_ptr<IMaterial> getMaterial() const = 0;
};

struct SGE_CORE_API AssetMaterial : public Asset, public AssetIface_Material {
	AssetMaterial(std::string assetPath, AssetLibrary& ownerAssetLib)
	    : Asset(assetPath, ownerAssetLib) {
	}

	std::shared_ptr<IMaterial> getMaterial() const override {
		return m_material;
	}

	bool loadAssetFromFile(const char* const path) override;

  public:
	std::shared_ptr<IMaterial> m_material;
};

} // namespace sge
