#pragma once

#include "IAsset.h"
#include "IAssetInterface.h"

namespace sge {
struct SGE_CORE_API IAssetInterface_Text : public IAssetInterface {
	virtual const std::string& getText() const = 0;
};

struct SGE_CORE_API AssetText : public Asset, public IAssetInterface_Text {
	AssetText(std::string assetPath, AssetLibrary& ownerAssetLib)
	    : Asset(assetPath, ownerAssetLib) {
	}

	virtual const std::string& getText() const override {
		return m_text;
	}

	bool loadAssetFromFile(const char* const path) override;

  private:
	std::string m_text;
};


} // namespace sge
