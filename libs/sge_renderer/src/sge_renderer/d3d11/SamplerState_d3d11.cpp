#include "SamplerState_d3d11.h"
#include "GraphicsInterface_d3d11.h"

namespace sge {

//////////////////////////////////////////////////////////////////////////////
// SamplerStateD3D11
//////////////////////////////////////////////////////////////////////////////
bool SamplerStateD3D11::create(const SamplerDesc& desc)
{
	destroy();

	ID3D11Device* const d3ddev = getDevice<SGEDeviceD3D11>()->D3D11_GetDevice();

	D3D11_SAMPLER_DESC sd = SamplerDesc_D3D11_Native(desc);
	HRESULT const hr = d3ddev->CreateSamplerState(&sd, &m_samplerState);

	if (FAILED(hr)) {
		destroy();
		return false;
	}

	return true;
}

void SamplerStateD3D11::destroy()
{
	m_samplerState.Release();
}

bool SamplerStateD3D11::isValid() const
{
	return m_samplerState != nullptr;
}

} // namespace sge
