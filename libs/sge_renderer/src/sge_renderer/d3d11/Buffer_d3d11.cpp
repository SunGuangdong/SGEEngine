#include "Buffer_d3d11.h"
#include "GraphicsInterface_d3d11.h"

namespace sge {

bool BufferD3D11::create(const BufferDesc& desc, const void* const pInitalData)
{
	destroy();

	ID3D11Device* const d3ddev = getDevice<SGEDeviceD3D11>()->D3D11_GetDevice();

	D3D11_SUBRESOURCE_DATA subresData = {0};
	subresData.pSysMem = pInitalData;

	D3D11_BUFFER_DESC const d3d11BuffDesc = BufferDesc_D3D11_Native(desc);

	HRESULT const hr = d3ddev->CreateBuffer(&d3d11BuffDesc, (pInitalData) ? &subresData : nullptr, &m_d3d11_buffer);

	if (FAILED(hr)) {
		sgeAssert(false);
		return false;
	}

	m_bufferDesc = desc;

	return true;
}

void BufferD3D11::destroy()
{
	// Unbind the buffer form the context state cache.
	D3D11ContextStateCache* const stateCache = getDevice<SGEDeviceD3D11>()->D3D11_GetContextStateCache();
	stateCache->BufferUnbind(m_d3d11_buffer);

	// Release the resources.
	m_d3d11_buffer.Release();
	m_bufferDesc = BufferDesc();
}

bool BufferD3D11::isValid() const
{
	return m_d3d11_buffer != nullptr;
}

void* BufferD3D11::map(const Map::Enum map, SGEContext* UNUSED(context))
{
	// TODO: Fugure out why I wrote this and document it!
	ID3D11DeviceContext* const d3dcon = getDevice<SGEDeviceD3D11>()->D3D11_GetImmContext();

	D3D11_MAPPED_SUBRESOURCE subres;
	d3dcon->Map((ID3D11Resource*)m_d3d11_buffer, 0, Map_D3D11_Native(map), 0, &subres);

	sgeAssert(subres.pData);

	return subres.pData;
}

void BufferD3D11::unMap(SGEContext* UNUSED(context))
{
	// TODO: Fugure out why I wrote this and document it!
	ID3D11DeviceContext* const d3dcon = getDevice<SGEDeviceD3D11>()->D3D11_GetImmContext();
	d3dcon->Unmap((ID3D11Resource*)m_d3d11_buffer, 0);
}

} // namespace sge
