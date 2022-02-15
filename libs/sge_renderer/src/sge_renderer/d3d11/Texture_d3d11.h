#pragma once

#include "GraphicsCommon_d3d11.h"
#include "sge_renderer/renderer/renderer.h"

namespace sge {

//----------------------------------------------------------
// Texture
//----------------------------------------------------------
struct TextureD3D11 : public Texture {
	TextureD3D11() = default;
	~TextureD3D11() { destroy(); }

	// Create a texture with(if possible) inital data.
	// Initial data should be ordered in that way
	// arrayElem0(mip0, mip1, mip2, ...)
	// arrayElem1(mip0, mip1, mip2, ...)
	// arrayElem2(mip0, mip1, mip2, ...)
	bool create(
	    const TextureDesc& desc, const TextureData initalData[], const SamplerDesc samplerDesc = SamplerDesc()) final;

	virtual void destroy() final;
	virtual bool isValid() const final;

	const TextureDesc& getDesc() const final { return m_desc; }
	SamplerState* getSamplerState() final { return m_samplerState; }
	void setSamplerState(SamplerState* ss) final { m_samplerState = ss; }

	ID3D11SamplerState* D3D11_GetSamplerState();
	ID3D11Resource* D3D11_GetTextureResource() { return m_dx11Texture; }

	//[TODO] maybe add posibility to create SRV form subresource
	ID3D11ShaderResourceView* D3D11_GetSRV();

	//[TODO] Add RTVs for Texture3D TextureCube ect..
	// generates a render target view for the given arr Idx and mipLevel, for MS texture mip level is ignored
	ID3D11RenderTargetView* D3D11_GetRTV(const TargetDesc& targetDesc);

	//[TODO] add DSV get form Texture3D TextureCube ect..
	ID3D11DepthStencilView* D3D11_GetDSV(const TargetDesc& targetDesc);

	bool D3D11_WrapOverD3D11TextureResource(
	    SGEDevice* pDevice, ID3D11Texture2D* d3d11Texture2D, const TextureDesc& desc);

  private:
	TextureDesc m_desc;
	GpuHandle<SamplerState> m_samplerState;
	TComPtr<ID3D11Resource> m_dx11Texture;

	TComPtr<ID3D11ShaderResourceView> m_srv;
	std::vector<std::pair<TargetDesc, TComPtr<ID3D11RenderTargetView>>> m_rtvs;
	std::vector<std::pair<TargetDesc, TComPtr<ID3D11DepthStencilView>>> m_dsvs;
};

} // namespace sge
