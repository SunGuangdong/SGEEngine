#include "Query_d3d11.h"
#include "GraphicsInterface_d3d11.h"

namespace sge {

bool QueryD3D11::create(QueryType::Enum const queryType)
{
	ID3D11Device* const d3ddev = getDevice<SGEDeviceD3D11>()->D3D11_GetDevice();

	m_queryType = queryType;

	D3D11_QUERY_DESC d3d11QueryDesc = {};
	d3d11QueryDesc.MiscFlags = 0;
	d3d11QueryDesc.Query = QueryType_D3D11_Native(queryType);

	HRESULT const hr = d3ddev->CreateQuery(&d3d11QueryDesc, &m_d3d11_query);

	if (FAILED(hr)) {
		return false;
	}

	return true;
}

void QueryD3D11::destroy()
{
	m_d3d11_query.Release();
}

bool QueryD3D11::isValid() const
{
	return m_d3d11_query != nullptr;
}

} // namespace sge
