#pragma once

#include "GraphicsCommon_d3d11.h"
#include "sge_utils/containers/Pair.h"
#include "sge_utils/containers/StaticArray.h"
#include <array>

#include "include_d3d11.h"

namespace sge {

//---------------------------------------------------------------------------
// D3D11ContextStateCache
//---------------------------------------------------------------------------
struct D3D11ContextStateCache {
	template <typename T>
	bool UPDATE_ON_DIFF(T& Variable, const T& Value)
	{
		if (Variable == Value)
			return false;
		Variable = Value;
		return true;
	}

  public:
	D3D11ContextStateCache(ID3D11DeviceContext* dx11Context = NULL);

	void setPrimitiveTopology(const D3D11_PRIMITIVE_TOPOLOGY topology);

	void SetInputLayout(ID3D11InputLayout* inputLayout);

	void BindVertexBuffers(
	    ID3D11Buffer** ppBuffers, UINT* pStrides, UINT* pOffsetsByte, const UINT numBuffers, const UINT startSlot);

	void BindIndexBuffer(ID3D11Buffer* buffer, DXGI_FORMAT format, UINT byteOffset);

	void SetVS(ID3D11VertexShader* vertShader);
	void SetPS(ID3D11PixelShader* pixelShader);

	void BindConstantBuffers(const ShaderType::Enum stage, UINT startSlot, UINT numElements, ID3D11Buffer** pBuffers);

	// Checks if the "resource" is already bound as a render target or a depth stencil.
	bool IsResourceBoundAsRTVorDSV(const ID3D11Resource* resource);

	void BindSRVs(const ShaderType::Enum stage, UINT startSlot, UINT numElements, ID3D11ShaderResourceView** pSRVs);

	void BindSamplers(const ShaderType::Enum stage, UINT startSlot, UINT numElements, ID3D11SamplerState** pSamplers);

	void SetRasterizerState(ID3D11RasterizerState* rasterState);
	void SetScissors(const D3D11_RECT* rects, UINT numRects);
	void SetDepthStencilState(ID3D11DepthStencilState* depthStencilState);
	void SetBlendState(ID3D11BlendState* bs);

	// Searches if any bound SRV matches a value from "resources" and unbinds it.
	void ResolveBindRTVorDSVHazzard(ID3D11Resource* const* resources, const int numResources);

	void SetRenderTargetsAndDepthStencil(
	    const UINT startSlot, const UINT numRenderTargets, ID3D11RenderTargetView** rtvs, ID3D11DepthStencilView* dsv);

	void setViewport(const D3D11_VIEWPORT& viewport);

	void BufferUnbind(ID3D11Buffer* const buffer);

	void TextureUnbind(
	    ID3D11ShaderResourceView* const srvs[],
	    const int srvCnt,
	    ID3D11RenderTargetView* const rtvs[],
	    const int rtvCnt,
	    ID3D11DepthStencilView* const dsvs[],
	    const int dsvCnt);

	void InputLayoutUnbind(ID3D11InputLayout* const inputLayout);

  public:
	ID3D11DeviceContext* m_d3dcon;

	D3D11_PRIMITIVE_TOPOLOGY m_primitiveTopology;
	ID3D11InputLayout* m_inputLayout;

	struct BoundVertexBuffers {
		BoundVertexBuffers()
		{
			for (auto& v : buffer)
				v = NULL;
			for (auto& v : stride)
				v = 0;
			for (auto& v : byteOffset)
				v = 0;
		}

		static const int NUM_BUFFERS = GraphicsCaps::kVertexBufferSlotsCount;

		std::array<ID3D11Buffer*, NUM_BUFFERS> buffer;
		std::array<UINT, NUM_BUFFERS> stride;
		std::array<UINT, NUM_BUFFERS> byteOffset;
	};

	BoundVertexBuffers m_boundVertBuffers;

	ID3D11Buffer* m_indexBuffer;
	DXGI_FORMAT m_indexBufferFromat;
	UINT m_indexBufferByteOffset;

	ID3D11VertexShader* m_vertexShader;
	ID3D11PixelShader* m_pixelShader;

	struct ShadingStageResources {
		ShadingStageResources()
		{
			for (auto& v : cbuffers)
				v = nullptr;
			for (auto& v : srvs)
				v = nullptr;
			for (auto& v : samplerStates)
				v = nullptr;
		}

		std::array<ID3D11Buffer*, GraphicsCaps::kConstantBufferSlotsCount> cbuffers;
		std::array<ID3D11ShaderResourceView*, GraphicsCaps::kD3D11_SRV_Count> srvs;
		std::array<ID3D11SamplerState*, GraphicsCaps::kSampleSlotsCount> samplerStates;
	};

	std::array<ShadingStageResources, ShaderType::NumElems> m_boundResources;

	ID3D11RasterizerState* m_rasterState;
	std::vector<D3D11_RECT> m_scissorsRects;
	ID3D11DepthStencilState* m_depthStencilState;
	ID3D11BlendState* m_blendState;

	// render target and depth stencil
	std::array<ID3D11RenderTargetView*, GraphicsCaps::kRenderTargetSlotsCount> m_rtvs;
	ID3D11DepthStencilView* m_dsv;

	// The 1st element in the pair indicates if there is an active viewport.
	Pair<bool, D3D11_VIEWPORT> m_viewport;
};

} // namespace sge
