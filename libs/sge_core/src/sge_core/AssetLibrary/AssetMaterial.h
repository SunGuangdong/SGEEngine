#pragma once

#include "IAsset.h"
#include "IAssetInterface.h"
#include "sge_core/materials/IMaterial.h"

namespace sge {

struct SGE_CORE_API AssetIface_Material : public IAssetInterface {
	AssetIface_Material() = default;
	virtual ~AssetIface_Material() = default;
	virtual IMaterial* getMaterial() const = 0;
};

struct SGE_CORE_API AssetIface_Material_Simple : public AssetIface_Material {
	IMaterial* getMaterial() const override { return mtl.get(); }
	std::unique_ptr<IMaterial> mtl;
};

struct SGE_CORE_API AssetMaterial : public Asset, public AssetIface_Material {
	AssetMaterial(std::string assetPath, AssetLibrary& ownerAssetLib)
	    : Asset(assetPath, ownerAssetLib)
	{
	}

	IMaterial* getMaterial() const override { return mtl.get(); }

	bool loadAssetFromFile(const char* const path) override;
	bool saveAssetToFile(const char* const path) const;

  public:
	std::unique_ptr<IMaterial> mtl;
};

} // namespace sge
