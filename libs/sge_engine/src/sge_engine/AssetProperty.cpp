#include "AssetProperty.h"
#include "sge_core/ICore.h"
#include "sge_engine/TypeRegister.h"

namespace sge {

bool AssetProperty::update() {
	if (isUpToDate()) {
		return false;
	}

	m_currentAsset = m_targetAsset;
	if (m_currentAsset.empty() == false) {
		m_asset = getCore()->getAssetLib()->getAssetFromFile(m_currentAsset.c_str());
	} else {
		m_asset = std::shared_ptr<Asset>();
	}

	return true;
}


void AssetProperty::setAsset(std::shared_ptr<Asset> asset) {
	m_asset = std::move(asset);
	m_targetAsset = m_asset->getPath();
	m_currentAsset = m_asset->getPath();
}

void AssetProperty::setTargetAsset(const char* const assetPath) {
	m_targetAsset = assetPath ? assetPath : "";
}

RelfAddTypeId(AssetProperty, 20'03'01'0001);
ReflBlock() {
	ReflAddType(AssetProperty) ReflMember(AssetProperty, m_targetAsset);
}

} // namespace sge
