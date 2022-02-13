#include "AssetProperty.h"
#include "sge_core/ICore.h"
#include "sge_core/typelib/typeLib.h"

namespace sge {

AssetProperty::AssetProperty(const AssetProperty& ref)
{
	*this = ref;
}

AssetProperty& AssetProperty::operator=(const AssetProperty& ref)
{
	setAsset(ref.m_asset);
	m_acceptedAssetIfaceTypes = ref.m_acceptedAssetIfaceTypes;
	m_uiPossibleAssets = ref.m_uiPossibleAssets;

	return *this;
}

bool AssetProperty::update()
{
	bool hadAChange = m_assetChangeIndex.checkForChangeAndUpdate();
	return hadAChange;
}

void AssetProperty::setAsset(AssetPtr newAsset)
{
	if (m_asset != newAsset) {
		m_assetChangeIndex.markAChange();
		m_asset = std::move(newAsset);
	}
}

void AssetProperty::setAsset(const char* newAssetPath)
{
	setAsset(getCore()->getAssetLib()->getAssetFromFile(newAssetPath));
}

ReflAddTypeId(AssetProperty, 20'03'01'0001);
ReflBlock()
{
	ReflAddType(AssetProperty) ReflMember(AssetProperty, m_asset);
}

} // namespace sge
