#pragma once

#include "IAsset.h"
#include "IAssetInterface.h"
#include "sge_core/materials/GenericGeomListShader/GenericGeomListShader.h"

namespace sge {

struct SGE_CORE_API IAssetIface_GeomLitShader : public IAssetInterface {
	virtual GenericGeomLitShader* getGeomLitShader() = 0;
};

struct AssetGeomLitShader : public Asset, public IAssetIface_GeomLitShader {
	AssetGeomLitShader(std::string assetPath, AssetLibrary& ownerAssetLib)
	    : Asset(assetPath, ownerAssetLib)
	{
	}

	bool loadAssetFromFile(const char* path) override;

	virtual GenericGeomLitShader* getGeomLitShader() override { return &genericShader; }

  public:
	std::string mainShaderFile;
	GenericGeomLitShader genericShader;
};

} // namespace sge
