#pragma once

#include "include_d3d11.h"

#include "sge_renderer/renderer/GraphicsCommon.h"

namespace sge {

D3D11_VIEWPORT Viewport_D3D11_Native(const Rect2s& viewport);

UINT ResourceBindFlags_D3D11_Native(const unsigned bindFlags);

void ResourceUsage_D3D11_Native(ResourceUsage::Enum type, D3D11_USAGE& usage, UINT& cpuAcessFlags);

D3D11_MAP Map_D3D11_Native(const Map::Enum map);

void TextureFormat_D3D11_Native(
    const TextureFormat::Enum format, DXGI_FORMAT& srv, DXGI_FORMAT& dsv, DXGI_FORMAT& typeless);

D3D11_SUBRESOURCE_DATA TextureData_D3D11_Native(const TextureData& texData);

D3D11_TEXTURE_ADDRESS_MODE TextureAddressMode_D3D11_Native(const TextureAddressMode::Enum& mode);

D3D11_FILTER TextureFilter_D3D11_Native(const TextureFilter::Enum& filter);

D3D11_SAMPLER_DESC SamplerDesc_D3D11_Native(SamplerDesc const& desc);

DXGI_FORMAT UniformType_GetDX_DXGI_FORMAT(const UniformType::Enum uniformType);

D3D11_BUFFER_DESC BufferDesc_D3D11_Native(const BufferDesc& desc);

D3D11_FILL_MODE FillMode_D3D11_Native(const FillMode::Enum& fillMode);

D3D11_CULL_MODE CullMode_D3D11_Native(const CullMode::Enum& cullMode);
;

D3D11_COMPARISON_FUNC DepthComparisonFunc_D3D11_Native(const DepthComparisonFunc::Enum& comaprisonFunc);

D3D_PRIMITIVE_TOPOLOGY PrimitiveTopology_D3D11_Native(const PrimitiveTopology::Enum& topology);

D3D11_RASTERIZER_DESC RasterDesc_GetD3D11Nativce(const RasterDesc& desc);

D3D11_DEPTH_STENCIL_DESC DepthStencilDesc_D3D11_Native(const DepthStencilDesc& desc);

D3D11_BLEND Blend_D3D11_Native(Blend::Enum blend);

D3D11_BLEND_OP BlendOp_D3D11_Native(BlendOp::Enum blendOp);

D3D11_RENDER_TARGET_BLEND_DESC BlendDesc_D3D11_Native(const BlendDesc& desc);

D3D11_BLEND_DESC BlendStateDesc_D3D11_Native(const BlendStateDesc& desc);

D3D11_QUERY QueryType_D3D11_Native(QueryType::Enum const queryType);

D3D11_TEXTURECUBE_FACE signedAxis_toTexCubeFaceIdx_D3D11(const SignedAxis sa);
} // namespace sge
