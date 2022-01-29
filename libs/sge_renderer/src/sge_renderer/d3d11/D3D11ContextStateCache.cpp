#include "D3D11ContextStateCache.h"
#include "sge_log/Log.h"
#include "sge_utils/containers/ArrayView.h"
#include "sge_utils/containers/Range.h"
#include "sge_utils/microsoft/comptr.h"

namespace sge {

D3D11ContextStateCache::D3D11ContextStateCache(ID3D11DeviceContext* dx11Context)
{
	m_d3dcon = dx11Context;

	m_primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
	m_inputLayout = NULL;

	// m_boundVertBuffers <- this one id initialized by default

	m_indexBuffer = NULL;
	m_indexBufferFromat = DXGI_FORMAT_UNKNOWN;
	m_indexBufferByteOffset = 0;

	m_vertexShader = NULL;
	m_pixelShader = NULL;

	// m_boundResources <- this one id initialized by default

	m_rasterState = NULL;
	m_depthStencilState = NULL;

	for (auto& v : m_rtvs)
		v = NULL;
	m_dsv = NULL;

	// Mark the viewport as unset.
	m_viewport.first = false;
}

void D3D11ContextStateCache::setPrimitiveTopology(const D3D11_PRIMITIVE_TOPOLOGY topology)
{
	if (UPDATE_ON_DIFF(m_primitiveTopology, topology)) {
		m_d3dcon->IASetPrimitiveTopology(topology);
	}
}

void D3D11ContextStateCache::SetInputLayout(ID3D11InputLayout* inputLayout)
{
	if (UPDATE_ON_DIFF(m_inputLayout, inputLayout)) {
		m_d3dcon->IASetInputLayout(inputLayout);
	}
}

void D3D11ContextStateCache::BindVertexBuffers(
    ID3D11Buffer** ppBuffers, UINT* pStrides, UINT* pOffsetsByte, const UINT numBuffers, const UINT startSlot)
{
#if 0
	// Just as a note, this performed better ony my AMD 7700, but not that well on GeForce 960M.
	m_d3dcon->IASetVertexBuffers(
				startSlot, 
				numBuffers, 
				ppBuffers,
				pStrides,
				pOffsetsByte);

	return;
#endif

	sgeAssert(numBuffers + startSlot <= m_boundVertBuffers.buffer.size());
	sgeAssert(ppBuffers);
	sgeAssert(pStrides);
	sgeAssert(pOffsetsByte);

	UINT bindSlotStartAccum = startSlot;
	UINT bindNumElems = 0;

	for (UINT t = 0; t < numBuffers; ++t) {
		const bool isLastBuffer = (t == (numBuffers - 1));

		const bool changed = (m_boundVertBuffers.buffer[t] != ppBuffers[t - startSlot]) ||
		                     (m_boundVertBuffers.stride[t] != pStrides[t - startSlot]) ||
		                     (m_boundVertBuffers.byteOffset[t] != pOffsetsByte[t - startSlot]);

		if (changed) {
			m_boundVertBuffers.buffer[t] = ppBuffers[t - startSlot];
			m_boundVertBuffers.stride[t] = pStrides[t - startSlot];
			m_boundVertBuffers.byteOffset[t] = pOffsetsByte[t - startSlot];
			bindNumElems++;
		}

		if ((!changed || isLastBuffer) && bindNumElems) {
			m_d3dcon->IASetVertexBuffers(bindSlotStartAccum, bindNumElems, &m_boundVertBuffers.buffer[bindSlotStartAccum],
			                             &m_boundVertBuffers.stride[bindSlotStartAccum],
			                             &m_boundVertBuffers.byteOffset[bindSlotStartAccum]);

			// Update the next binding pos and reset counters.
			bindSlotStartAccum = startSlot + t + 1;
			bindNumElems = 0;
		}
	}
}

void D3D11ContextStateCache::BindIndexBuffer(ID3D11Buffer* buffer, DXGI_FORMAT format, UINT byteOffset)
{
	if (m_indexBuffer == buffer && m_indexBufferFromat == format && m_indexBufferByteOffset == byteOffset) {
		// nothing to update
		return;
	}

	m_indexBuffer = buffer;
	m_indexBufferFromat = format;
	m_indexBufferByteOffset = byteOffset;

	if (format == DXGI_FORMAT_UNKNOWN && buffer != 0) {
		//"%s: Trying to bind index buffer with DXGI_FORMAT_UNKNOWN. This can crash the UMD!\n", __FUNCTION__);
		sgeAssert(false);
		return; //[TODO] not quite shure for that one...
	}

	m_d3dcon->IASetIndexBuffer(buffer, format, byteOffset);
}

void D3D11ContextStateCache::SetVS(ID3D11VertexShader* vertShader)
{
	if (UPDATE_ON_DIFF(m_vertexShader, vertShader)) {
		m_d3dcon->VSSetShader(vertShader, 0, 0);
	}
}

void D3D11ContextStateCache::SetPS(ID3D11PixelShader* pixelShader)
{
	if (UPDATE_ON_DIFF(m_pixelShader, pixelShader)) {
		m_d3dcon->PSSetShader(pixelShader, 0, 0);
	}
}

void D3D11ContextStateCache::BindConstantBuffers(const ShaderType::Enum stage, UINT startSlot, UINT numElements, ID3D11Buffer** pBuffers)
{
	bool shouldCallAPI = false;
	for (unsigned int t = startSlot; t < numElements; ++t) {
		if (m_boundResources[stage].cbuffers[t] != pBuffers[t]) {
			shouldCallAPI = true;
			break;
		}
	}

	if (shouldCallAPI) {
		memcpy(m_boundResources[stage].cbuffers.data() + startSlot, pBuffers, sizeof(ID3D11Buffer*) * numElements);

		switch (stage) {
			case ShaderType::VertexShader:
				m_d3dcon->VSSetConstantBuffers(startSlot, numElements, m_boundResources[stage].cbuffers.data() + startSlot);
				break;
			case ShaderType::PixelShader:
				m_d3dcon->PSSetConstantBuffers(startSlot, numElements, m_boundResources[stage].cbuffers.data() + startSlot);
				break;
			default:
				sgeAssert(false);
		}
	}

	UINT bindSlotStartAccum = startSlot;
	UINT bindNumElems = 0;

	for (UINT t = 0; t < numElements; ++t) {
		UINT slot = startSlot + t;
		const bool updated = UPDATE_ON_DIFF(m_boundResources[stage].cbuffers[slot], pBuffers[t]);
		const bool isLastBuffer = (t == (numElements - 1));

		if (updated) {
			bindNumElems++;
		}

		if (!updated || isLastBuffer) {
			// If there is anything new to bind...
			if (bindNumElems != 0) {
				auto& cbuffers = m_boundResources[stage].cbuffers;
				switch (stage) {
					case ShaderType::VertexShader:
						m_d3dcon->VSSetConstantBuffers(bindSlotStartAccum, bindNumElems, &cbuffers[bindSlotStartAccum]);
						break;
					case ShaderType::PixelShader:
						m_d3dcon->PSSetConstantBuffers(bindSlotStartAccum, bindNumElems, &cbuffers[bindSlotStartAccum]);
						break;
					default:
						sgeAssert(false);
				}
			}

			// update the next binding pos and reset counters
			bindSlotStartAccum = startSlot + t + 1;
			bindNumElems = 0;
		}
	}
}

// Check if the "resource" is already bound as a render target or a depth stencil.
bool D3D11ContextStateCache::IsResourceBoundAsRTVorDSV(const ID3D11Resource* resource)
{
	// Check against all RTVs.
	for (size_t iRtv = 0; iRtv < m_rtvs.size(); ++iRtv) {
		if (m_rtvs[iRtv] != NULL) {
			TComPtr<ID3D11Resource> rtvResource;
			m_rtvs[iRtv]->GetResource(&rtvResource);

			if (rtvResource == resource) {
				return true;
			}
		}
	}

	// Check against DSV.
	if (m_dsv != NULL) {
		TComPtr<ID3D11Resource> dsvResource;
		m_dsv->GetResource(&dsvResource);

		if (dsvResource == resource) {
			return true;
		}
	}

	return false;
}

void D3D11ContextStateCache::BindSRVs(const ShaderType::Enum stage, UINT startSlot, UINT numElements, ID3D11ShaderResourceView** pSRVs)
{
	UINT bindSlotStartAccum = startSlot;
	UINT bindNumElems = 0;

	for (UINT t = 0; t < numElements; ++t) {
		const UINT slot = startSlot + t;
		const bool isLastBuffer = (t == (numElements - 1));

		const bool updated = UPDATE_ON_DIFF(m_boundResources[stage].srvs[slot], pSRVs[t]);

		// Check if the updated resource isn't already bound as a render target. If so bind NULL on this slot.
		if (updated) {
			if (pSRVs[t] != nullptr) {
				TComPtr<ID3D11Resource> resource;
				pSRVs[t]->GetResource(&resource);

				if (IsResourceBoundAsRTVorDSV(resource)) {
					sgeAssert(false && "Trying to bind SRV resource that is already bound as RTV/DSV. SGE will bind NULL.");
					m_boundResources[stage].srvs[slot] = nullptr;
				}
			}

			bindNumElems++;
		}

		if (!updated || isLastBuffer) {
			if (bindNumElems > 0) // If there is anything new to bind.
			{
				auto& srvs = m_boundResources[stage].srvs;
				switch (stage) {
					case ShaderType::VertexShader:
						m_d3dcon->VSSetShaderResources(bindSlotStartAccum, bindNumElems, &srvs[bindSlotStartAccum]);
						break;
					case ShaderType::PixelShader:
						m_d3dcon->PSSetShaderResources(bindSlotStartAccum, bindNumElems, &srvs[bindSlotStartAccum]);
						break;
					default:
						sgeAssert(false);
				}
			}

			// Update the next binding pos and reset the counters.
			bindSlotStartAccum = startSlot + t + 1;
			bindNumElems = 0;
		}
	}
}

void D3D11ContextStateCache::BindSamplers(const ShaderType::Enum stage, UINT startSlot, UINT numElements, ID3D11SamplerState** pSamplers)
{
	UINT bindSlotStartAccum = startSlot;
	UINT bindNumElems = 0;

	for (UINT t = 0; t < numElements; ++t) {
		UINT slot = startSlot + t;
		const bool updated = UPDATE_ON_DIFF(m_boundResources[stage].samplerStates[slot], pSamplers[t]);
		const bool isLastBuffer = (t == (numElements - 1));

		if (updated) {
			bindNumElems++;
		}

		if (!updated || isLastBuffer) {
			// If there is anything new to bind...
			if (bindNumElems != 0) {
				auto& samplers = m_boundResources[stage].samplerStates;
				switch (stage) {
					case ShaderType::VertexShader:
						m_d3dcon->VSSetSamplers(bindSlotStartAccum, bindNumElems, &samplers[bindSlotStartAccum]);
						break;
					case ShaderType::PixelShader:
						m_d3dcon->PSSetSamplers(bindSlotStartAccum, bindNumElems, &samplers[bindSlotStartAccum]);
						break;
					default:
						sgeAssert(false);
				}
			}

			// Update the next binding pos and reset counters.
			bindSlotStartAccum = startSlot + t + 1;
			bindNumElems = 0;
		}
	}
}

void D3D11ContextStateCache::SetRasterizerState(ID3D11RasterizerState* rasterState)
{
	if (UPDATE_ON_DIFF(m_rasterState, rasterState)) {
		m_d3dcon->RSSetState(rasterState);
	}
}

void D3D11ContextStateCache::SetScissors(const D3D11_RECT* rects, UINT numRects)
{
	bool shouldCallAPIFunc = numRects != m_scissorsRects.size();

	if (shouldCallAPIFunc == false) {
		for (UINT t = 0; t < m_scissorsRects.size(); ++t) {
			if (rects[t] == m_scissorsRects[t]) {
				shouldCallAPIFunc = true;
				break;
			}
		}
	}

	if (shouldCallAPIFunc) {
		m_d3dcon->RSSetScissorRects(numRects, rects);
	}
}

void D3D11ContextStateCache::SetDepthStencilState(ID3D11DepthStencilState* depthStencilState)
{
	if (UPDATE_ON_DIFF(m_depthStencilState, depthStencilState)) {
		m_d3dcon->OMSetDepthStencilState(depthStencilState, 0);
	}
}

void D3D11ContextStateCache::SetBlendState(ID3D11BlendState* bs)
{
	if (UPDATE_ON_DIFF(m_blendState, bs)) {
		m_d3dcon->OMSetBlendState(m_blendState, NULL, 0xffffffff);
	}
}

// Searches is any bound SRV matches a value from resources and unbinds it.
void D3D11ContextStateCache::ResolveBindRTVorDSVHazzard(ID3D11Resource* const* resources, const int numResources)
{
	if (resources == NULL)
		return;

	for (int iStage = 0; iStage < ShaderType::NumElems; ++iStage)
		for (int iSlot = 0; iSlot < GraphicsCaps::kD3D11_SRV_Count; ++iSlot)
			for (int iResource = 0; iResource < numResources; ++iResource) {
				if (resources[iResource] == NULL)
					continue;

				TComPtr<ID3D11Resource> srvResource;
				if (m_boundResources[iStage].srvs[iSlot])
					m_boundResources[iStage].srvs[iSlot]->GetResource(&srvResource);

				if (srvResource == resources[iResource]) {
					sgeLogWarn(
					    "[RES-HAZZARD][D3D11]Trying to bind resource(stage:%d, slot:%d) as RTV/DSV while it's already bound as SRV."
					    "SRV will be unbound! The D3D11 warrning will be silenced by this action!\n",
					    iStage, iSlot);
					ID3D11ShaderResourceView* nullSRVs[] = {NULL};
					BindSRVs((ShaderType::Enum)iStage, iSlot, 1, nullSRVs);
				}

				if (srvResource)
					srvResource.Release();
			}
}

void D3D11ContextStateCache::SetRenderTargetsAndDepthStencil(const UINT startSlot,
                                                             const UINT numRenderTargets,
                                                             ID3D11RenderTargetView** rtvs,
                                                             ID3D11DepthStencilView* dsv)
{
	bool shouldCallAPIFunc = false;

	int lastRTVDiffIndex = -1;
	for (unsigned int t = startSlot; t < (startSlot + numRenderTargets); ++t) {
		const bool updated = UPDATE_ON_DIFF(m_rtvs[t], rtvs[t]);
		shouldCallAPIFunc = shouldCallAPIFunc || updated;

		if (updated) {
			lastRTVDiffIndex = t;
		}
	}

	// Check for updated Depth-Stencil.
	{
		const bool updated = UPDATE_ON_DIFF(m_dsv, dsv);
		shouldCallAPIFunc = shouldCallAPIFunc || updated;
	}

	// API CALL
	if (shouldCallAPIFunc) {
		// A pointer that contains the resources for all render targets and the depth-stencil.
		const int numResources = lastRTVDiffIndex + 1 + 1;
		ID3D11Resource** const resources = (ID3D11Resource**)alloca(sizeof(ID3D11Resource*) * (numResources));

		for (int t = startSlot; t <= lastRTVDiffIndex; ++t) {
			if (rtvs[t])
				rtvs[t]->GetResource(&resources[t - startSlot]);
			else
				resources[t - startSlot] = NULL;
		}

		// Depth-Stencil.
		if (dsv)
			dsv->GetResource(&resources[numResources - 1]);
		else
			resources[numResources - 1] = NULL;

		// Unbind from SRV all "resources" that are bound as SRVs... Aka bind them for "writing".
		ResolveBindRTVorDSVHazzard(resources, numResources);

		// Release the resources..
		for (int t = 0; t < numResources; ++t) {
			if (resources[t]) {
				resources[t]->Release();
			}
		}

		// Issue the actual D3D API call.
		m_d3dcon->OMSetRenderTargets(lastRTVDiffIndex + 1, m_rtvs.data(), m_dsv);
	}
}


void D3D11ContextStateCache::setViewport(const D3D11_VIEWPORT& viewport)
{
	if (memcmp(&viewport, &m_viewport.second, sizeof(m_viewport)) == 0)
		return;

	m_viewport.first = true;

	m_viewport.second = viewport;
	m_d3dcon->RSSetViewports(1, &m_viewport.second);
}

void D3D11ContextStateCache::BufferUnbind(ID3D11Buffer* const buffer)
{
	for (int t = 0; t < BoundVertexBuffers::NUM_BUFFERS; ++t) {
		if (m_boundVertBuffers.buffer[t] == buffer) {
			m_boundVertBuffers.buffer[t] = 0;
			m_boundVertBuffers.stride[t] = 0;
			m_boundVertBuffers.byteOffset[t] = 0;
		}
	}

	if (m_indexBuffer == buffer) {
		m_indexBuffer = NULL;
	}

	for (ShadingStageResources& ssr : m_boundResources)
		for (auto& cbuffer : ssr.cbuffers) {
			if (cbuffer == buffer)
				cbuffer = NULL;
		}
}

void D3D11ContextStateCache::TextureUnbind(ID3D11ShaderResourceView* const srvs[],
                                           const int srvCnt,
                                           ID3D11RenderTargetView* const rtvs[],
                                           const int rtvCnt,
                                           ID3D11DepthStencilView* const dsvs[],
                                           const int dsvCnt)
{
	if (srvs && srvCnt) {
		for (const auto srv : ArrayView<ID3D11ShaderResourceView* const>(srvs, srvCnt))
			for (int iStage = 0; iStage < ShaderType::NumElems; ++iStage)
				for (int iSlot = 0; iSlot < m_boundResources[iStage].srvs.size(); ++iSlot) {
					if (srv == m_boundResources[iStage].srvs[iSlot]) {
						ID3D11ShaderResourceView* nullSRVs[] = {NULL};
						BindSRVs((ShaderType::Enum)iStage, iSlot, 1, nullSRVs);
					}
				}
	}

	if (rtvs && rtvCnt) {
		ID3D11RenderTargetView* rtv_null_array[] = {NULL};

		for (const auto rtv : ArrayView<ID3D11RenderTargetView* const>(rtvs, rtvCnt))
			for (int iSlot = 0; iSlot < m_rtvs.size(); ++iSlot) {
				if (rtv == m_rtvs[iSlot]) {
					SetRenderTargetsAndDepthStencil(iSlot, 1, rtv_null_array, NULL);
				}
			}
	}

	if (dsvs && dsvCnt)
		for (const auto dsv : ArrayView<ID3D11DepthStencilView* const>(dsvs, dsvCnt)) {
			if (dsv == m_dsv) {
				SetRenderTargetsAndDepthStencil(0, 0, NULL, NULL);
				break;
			}
		}
}

void D3D11ContextStateCache::InputLayoutUnbind(ID3D11InputLayout* const inputLayout)
{
	if (inputLayout == m_inputLayout) {
		m_d3dcon->IASetInputLayout(NULL);
		m_inputLayout = NULL;
	}
}

} // namespace sge
