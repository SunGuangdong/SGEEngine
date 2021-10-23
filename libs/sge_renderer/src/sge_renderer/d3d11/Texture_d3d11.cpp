#include "Texture_d3d11.h"
#include "GraphicsInterface_d3d11.h"
#include "SamplerState_d3d11.h"

namespace sge {

//---------------------------------------------------------------
// Texture
//---------------------------------------------------------------
bool TextureD3D11::create(const TextureDesc& desc, const TextureData initalData[], const SamplerDesc samplerDesc) {
	destroy();

	ID3D11Device* const d3ddev = getDevice<SGEDeviceD3D11>()->D3D11_GetDevice();
	m_desc = desc;

	DXGI_FORMAT srv, dsv, typeless;
	TextureFormat_D3D11_Native(desc.format, srv, dsv, typeless);

	D3D11_USAGE d3dUsage = D3D11_USAGE_DEFAULT;
	UINT d3dBindFlags = 0;
	UINT d3dCpuAccess = 0;
	DXGI_FORMAT d3dFormat = DXGI_FORMAT_UNKNOWN; // the format of the resource, NOT THE VIEW(s)

	switch (desc.usage) {
		case TextureUsage::ImmutableResource: {
			d3dUsage = D3D11_USAGE_IMMUTABLE;
			d3dBindFlags = D3D11_BIND_SHADER_RESOURCE;
			d3dCpuAccess = 0;
			d3dFormat = srv;
		} break;

		case TextureUsage::DynamicResource: {
			d3dUsage = D3D11_USAGE_DYNAMIC;
			d3dBindFlags = D3D11_BIND_SHADER_RESOURCE;
			d3dCpuAccess = D3D11_CPU_ACCESS_WRITE;
			d3dFormat = srv;
		} break;

		case TextureUsage::RenderTargetResource: {
			d3dUsage = D3D11_USAGE_DEFAULT;
			d3dBindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
			d3dCpuAccess = 0;
			d3dFormat = srv;
		} break;

		case TextureUsage::RenderTargetOnly: {
			d3dUsage = D3D11_USAGE_DEFAULT;
			d3dBindFlags = D3D11_BIND_RENDER_TARGET;
			d3dCpuAccess = 0;
			d3dFormat = srv;
		} break;

		case TextureUsage::DepthStencilResource: {
			d3dUsage = D3D11_USAGE_DEFAULT;
			d3dBindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
			d3dCpuAccess = 0;
			d3dFormat = typeless; //[TODO]maybe this should be typeless when the texture is used for resource, there was a special case for
			                      // feature level 9?
		} break;

		case TextureUsage::DepthStencilOnly: {
			d3dUsage = D3D11_USAGE_DEFAULT;
			d3dBindFlags = D3D11_BIND_DEPTH_STENCIL;
			d3dCpuAccess = 0;
			d3dFormat = dsv; //[TODO]maybe this should be typeless when the texture is used for resource, there was a special case for
			                 // feature level 9?
		} break;

		default:
			sgeAssert(false);
	}

	// create the texture resource
	HRESULT hr = E_FAIL;

	if (desc.textureType == UniformType::Texture1D) {
		D3D11_TEXTURE1D_DESC d3dDesc;
		const auto& tex1D = desc.texture1D;

		d3dDesc.ArraySize = tex1D.arraySize;
		d3dDesc.MipLevels = tex1D.numMips;
		d3dDesc.Width = tex1D.width;
		d3dDesc.Format = d3dFormat;
		d3dDesc.MiscFlags = 0;
		d3dDesc.CPUAccessFlags = d3dCpuAccess;
		d3dDesc.Usage = d3dUsage;
		d3dDesc.BindFlags = d3dBindFlags;

		sgeAssert(tex1D.numMips == 0); // 0 mipmaps should mean generate all mipmaps bu this is not supported yet

		const int expectedTexDataSize = desc.texture1D.arraySize * desc.texture1D.numMips;

		std::vector<D3D11_SUBRESOURCE_DATA> d3dSubresData(expectedTexDataSize);
		for (int iData = 0; iData < expectedTexDataSize; ++iData) {
			d3dSubresData[iData] = TextureData_D3D11_Native(initalData[iData]);
		}

		hr = d3ddev->CreateTexture1D(&d3dDesc, d3dSubresData.data(), (ID3D11Texture1D**)&m_dx11Texture);

		// Generate RTV,SRV,DSV its
		sgeAssert(false);
	} else if (desc.textureType == UniformType::Texture2D) {
		D3D11_TEXTURE2D_DESC d3dDesc;
		const auto& tex2D = desc.texture2D;

		const bool shouldAutoGenMipMaps = desc.generateMips && tex2D.numMips == 1 && initalData != nullptr;
		if (shouldAutoGenMipMaps && initalData == nullptr) {
			sgeAssert(false && "Auto generated mip-maps need to have inital data!");
			return false;
		}

		d3dDesc.ArraySize = tex2D.arraySize;
		d3dDesc.MipLevels = tex2D.numMips;
		d3dDesc.Width = tex2D.width;
		d3dDesc.Height = tex2D.height;
		d3dDesc.SampleDesc.Count = tex2D.numSamples;
		d3dDesc.SampleDesc.Quality = tex2D.sampleQuality;
		d3dDesc.MiscFlags = 0;
		if (shouldAutoGenMipMaps) {
			d3dUsage = D3D11_USAGE_DEFAULT;
			d3dDesc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
			d3dBindFlags |= D3D11_BIND_RENDER_TARGET;
			d3dDesc.MipLevels = 0;
		}
		d3dDesc.Format = d3dFormat;
		d3dDesc.CPUAccessFlags = d3dCpuAccess;
		d3dDesc.Usage = d3dUsage;
		d3dDesc.BindFlags = d3dBindFlags;

		// A bit of a reasoning for early level error finding, sometimes the user fails to correctly specfy these.
		if (shouldAutoGenMipMaps == false) {
			sgeAssert(d3dDesc.MipLevels != 0 && "Zero MipLevels means that the mipmap level should be auto generated!");
		}
		sgeAssert(d3dDesc.SampleDesc.Count != 0 && "Zero samples for texture sampler is invalid");

		int expectedTexDataSize = initalData ? tex2D.arraySize * tex2D.numMips : 0;
		if (shouldAutoGenMipMaps) {
			expectedTexDataSize = 1;
		}

		std::vector<D3D11_SUBRESOURCE_DATA> d3dSubresData(expectedTexDataSize);
		for (int iData = 0; iData < expectedTexDataSize; ++iData) {
			d3dSubresData[iData] = TextureData_D3D11_Native(initalData[iData]);
		}

		hr = d3ddev->CreateTexture2D(&d3dDesc, shouldAutoGenMipMaps ? nullptr : d3dSubresData.data(), (ID3D11Texture2D**)&m_dx11Texture);

		if (shouldAutoGenMipMaps) {
			ID3D11DeviceContext* context = nullptr;
			d3ddev->GetImmediateContext(&context);
			if (context) {
				context->UpdateSubresource(m_dx11Texture, 0, NULL, d3dSubresData[0].pSysMem, d3dSubresData[0].SysMemPitch, 0);
				context->GenerateMips(D3D11_GetSRV());
			}
		}

		// Generate RTV,SRV,DSV that PROBABLY will be needed
		D3D11_GetSRV();
		D3D11_GetRTV(TargetDesc::FromTex2D());
		D3D11_GetDSV(TargetDesc::FromTex2D());
	} else if (desc.textureType == UniformType::TextureCube) {
		// In D3D11 cube textures are nothing more but an array of 2D texture with
		// arraySize multiple of 6.
		D3D11_TEXTURE2D_DESC d3dDesc;
		const TextureCubeDesc& texCUBE = desc.textureCube;

		d3dDesc.ArraySize = texCUBE.arraySize * 6;
		d3dDesc.MipLevels = texCUBE.numMips;
		d3dDesc.Width = texCUBE.width;
		d3dDesc.Height = texCUBE.height;
		d3dDesc.SampleDesc.Count = texCUBE.numSamples;
		d3dDesc.SampleDesc.Quality = texCUBE.sampleQuality;
		d3dDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
		d3dDesc.Format = d3dFormat;
		d3dDesc.CPUAccessFlags = d3dCpuAccess;
		d3dDesc.Usage = d3dUsage;
		d3dDesc.BindFlags = d3dBindFlags;

		sgeAssert(texCUBE.numMips != 0); // 0 mipmaps should mean generate all mipmaps but this is not supported yet
		sgeAssert(d3dDesc.ArraySize != 0);

		const int expectedTexDataSize = initalData ? d3dDesc.ArraySize * d3dDesc.MipLevels : 0;

		std::vector<D3D11_SUBRESOURCE_DATA> d3dSubresData(expectedTexDataSize);
		for (int iData = 0; iData < expectedTexDataSize; ++iData) {
			d3dSubresData[iData] = TextureData_D3D11_Native(initalData[iData]);
		}

		hr = d3ddev->CreateTexture2D(&d3dDesc, d3dSubresData.data(), (ID3D11Texture2D**)&m_dx11Texture);
		if (FAILED(hr)) {
			sgeAssert(false);
		}

		// Generate RTV,SRV,DSV that PROBABLY will be needed
		D3D11_GetSRV();
	} else if (desc.textureType == UniformType::Texture3D) {
		D3D11_TEXTURE3D_DESC d3dDesc;
		const auto& tex3D = desc.texture3D;

		d3dDesc.MipLevels = tex3D.numMips;
		d3dDesc.Width = tex3D.width;
		d3dDesc.Height = tex3D.height;
		d3dDesc.Depth = tex3D.depth;
		d3dDesc.MiscFlags = 0;
		d3dDesc.Format = d3dFormat;
		d3dDesc.CPUAccessFlags = d3dCpuAccess;
		d3dDesc.Usage = d3dUsage;
		d3dDesc.BindFlags = d3dBindFlags;

		sgeAssert(tex3D.numMips == 0); // 0 mipmaps should mean generate all mipmaps bu this is not supported yet

		const int expectedTexDataSize = tex3D.numMips;
		std::vector<D3D11_SUBRESOURCE_DATA> d3dSubresData(expectedTexDataSize);
		for (int iData = 0; iData < expectedTexDataSize; ++iData) {
			d3dSubresData[iData] = TextureData_D3D11_Native(initalData[iData]);
		}

		hr = d3ddev->CreateTexture3D(&d3dDesc, d3dSubresData.data(), (ID3D11Texture3D**)&m_dx11Texture);

		// Generate RTV,SRV,DSV its
		sgeAssert(false);
	} else {
		//[TODO]not implemented texture type
		sgeAssert(false);
	}

	if (FAILED(hr)) {
		sgeAssert(false);
		destroy();
		return false;
	}

	// cache the PREFFERED sampler state
	m_samplerState = getDevice<SGEDeviceD3D11>()->requestSamplerState(samplerDesc);

	return true;
}
//---------------------------------------------------------
ID3D11ShaderResourceView* TextureD3D11::D3D11_GetSRV() {
	if (!isValid()) {
		return nullptr;
	}

	if (!TextureUsage::CanBeShaderResource(m_desc.usage)) {
		return nullptr;
	}

	if (m_srv) {
		return m_srv;
	}


	// Caution:
	// A special case for Depth Stencil Resource textures
	// The texture format is typeless at creation.
	// Here we must specify non-typeless SRV compatible DXGI_FORMAT;

	// If nullptr, CreateShaderResourceView will create a view that accesses the full resource.
	D3D11_SHADER_RESOURCE_VIEW_DESC* pSrv = NULL;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvIfDepthResource;

	if (m_desc.usage == TextureUsage::DepthStencilResource) {
		DXGI_FORMAT srv, dsv, typeless;
		TextureFormat_D3D11_Native(m_desc.format, srv, dsv, typeless);

		if (m_desc.textureType == UniformType::Texture2D) {
			sgeAssert(m_desc.texture2D.arraySize == 1);
			srvIfDepthResource.Format = srv;
			if (m_desc.texture2D.numSamples == 1) {
				srvIfDepthResource.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				srvIfDepthResource.Texture2D.MostDetailedMip = 0;
				srvIfDepthResource.Texture2D.MipLevels = -1;
			} else {
				srvIfDepthResource.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
				sgeAssert(false && "Not Implemented");
			}
		} else if (m_desc.textureType == UniformType::TextureCube) {
			sgeAssert(m_desc.textureCube.arraySize == 1);
			srvIfDepthResource.Format = srv;
			if (m_desc.textureCube.numSamples == 1) {
				srvIfDepthResource.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
				srvIfDepthResource.TextureCube.MostDetailedMip = 0;
				srvIfDepthResource.TextureCube.MipLevels = -1;
			} else {
				srvIfDepthResource.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
				sgeAssert(false && "Not Implemented");
			}
		} else {
			sgeAssert(false);
		}

		pSrv = &srvIfDepthResource;
	}

	ID3D11Device* const d3ddev = getDevice<SGEDeviceD3D11>()->D3D11_GetDevice();
	HRESULT hr = d3ddev->CreateShaderResourceView(m_dx11Texture, pSrv, &m_srv);

	if (FAILED(hr)) {
		sgeAssert(false);
		return nullptr;
	}

	return m_srv;
}

ID3D11RenderTargetView* TextureD3D11::D3D11_GetRTV(const TargetDesc& targetDesc) {
	if (!isValid()) {
		return nullptr;
	}

	if (!TextureUsage::CanBeRenderTarget(m_desc.usage)) {
		return nullptr;
	}

	if (targetDesc.baseTextureType != m_desc.textureType) {
		sgeAssert(false && "Trying to create a texture RTV with wrong base texture type!");
	}

	// the texture could be 2D or 2DMS texture
	const D3D11_RTV_DIMENSION viewDimension =
	    (m_desc.texture2D.numSamples == 1) ? D3D11_RTV_DIMENSION_TEXTURE2DARRAY : D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;

	// search if that RTV already exists
	for (const auto& v : m_rtvs) {
		if (v.first == targetDesc)
			return v.second;
	}

	// Try to create a new RTV
	ID3D11Device* const d3ddev = getDevice<SGEDeviceD3D11>()->D3D11_GetDevice();

	DXGI_FORMAT srv, dsv, typeless;
	TextureFormat_D3D11_Native(m_desc.format, srv, dsv, typeless);

	D3D11_RENDER_TARGET_VIEW_DESC descRT;
	descRT.Format = srv;

	if (targetDesc.baseTextureType == UniformType::Texture1D) {
		descRT.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1DARRAY;
		descRT.Texture1DArray.ArraySize = targetDesc.texture1D.arrayIdx;
		descRT.Texture1DArray.MipSlice = targetDesc.texture1D.mipLevel;
	} else if (targetDesc.baseTextureType == UniformType::Texture2D) {
		descRT.ViewDimension =
		    (m_desc.texture2D.numSamples == 1) ? D3D11_RTV_DIMENSION_TEXTURE2DARRAY : D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;

		if (viewDimension == D3D11_RTV_DIMENSION_TEXTURE2DARRAY) {
			descRT.Texture2DArray.ArraySize = 1;
			descRT.Texture2DArray.FirstArraySlice = targetDesc.texture2D.arrayIdx;
			descRT.Texture2DArray.MipSlice = targetDesc.texture2D.mipLevel;
		} else if (viewDimension == D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY) {
			descRT.Texture2DMSArray.ArraySize = 1;
			descRT.Texture2DMSArray.FirstArraySlice = targetDesc.texture2D.arrayIdx;
		}
	} else if (targetDesc.baseTextureType == UniformType::TextureCube) {
		descRT.ViewDimension =
		    (m_desc.texture2D.numSamples == 1) ? D3D11_RTV_DIMENSION_TEXTURE2DARRAY : D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;

		const int faceArrayIdx = signedAxis_toTexCubeFaceIdx_D3D11(targetDesc.textureCube.face);

		if (viewDimension == D3D11_RTV_DIMENSION_TEXTURE2DARRAY) {
			descRT.Texture2DArray.ArraySize = 1;
			descRT.Texture2DArray.FirstArraySlice = faceArrayIdx;
			descRT.Texture2DArray.MipSlice = targetDesc.textureCube.mipLevel;
		} else if (viewDimension == D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY) {
			descRT.Texture2DMSArray.ArraySize = 1;
			descRT.Texture2DMSArray.FirstArraySlice = faceArrayIdx;
		}
	} else {
		sgeAssert(false);
	}

	TComPtr<ID3D11RenderTargetView> rtv;
	const HRESULT hr = d3ddev->CreateRenderTargetView(m_dx11Texture, &descRT, &rtv);

	if (FAILED(hr)) {
		sgeAssert(false);
		return nullptr;
	}

	m_rtvs.push_back({targetDesc, rtv});
	return rtv;
}

ID3D11DepthStencilView* TextureD3D11::D3D11_GetDSV(const TargetDesc& targetDesc) {
	if (!isValid()) {
		return nullptr;
	}

	if (!TextureUsage::CanBeDepthStencil(m_desc.usage)) {
		return nullptr;
	}

	// the texture could be 2D or 2DMS texture
	const D3D11_DSV_DIMENSION viewDimension =
	    (m_desc.texture2D.numSamples == 1) ? D3D11_DSV_DIMENSION_TEXTURE2DARRAY : D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;

	// search if that DSV already exists
	for (const auto& v : m_dsvs) {
		if (v.first == targetDesc)
			return v.second;
	}

	// Try to create new DSV
	ID3D11Device* const d3ddev = getDevice<SGEDeviceD3D11>()->D3D11_GetDevice();

	DXGI_FORMAT srv, dsvFmt, typeless;
	TextureFormat_D3D11_Native(m_desc.format, srv, dsvFmt, typeless);

	D3D11_DEPTH_STENCIL_VIEW_DESC descDS;
	descDS.ViewDimension = viewDimension;
	descDS.Format = dsvFmt;
	descDS.Flags = 0;

	if (targetDesc.baseTextureType == UniformType::Texture1D) {
		descDS.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1DARRAY;
		descDS.Texture1DArray.ArraySize = targetDesc.texture1D.arrayIdx;
		descDS.Texture1DArray.MipSlice = targetDesc.texture1D.mipLevel;
	} else if (targetDesc.baseTextureType == UniformType::Texture2D) {
		descDS.ViewDimension =
		    (m_desc.texture2D.numSamples == 1) ? D3D11_DSV_DIMENSION_TEXTURE2DARRAY : D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;

		if (viewDimension == D3D11_DSV_DIMENSION_TEXTURE2DARRAY) {
			descDS.Texture2DArray.ArraySize = 1;
			descDS.Texture2DArray.FirstArraySlice = targetDesc.texture2D.arrayIdx;
			descDS.Texture2DArray.MipSlice = targetDesc.texture2D.mipLevel;
		} else if (viewDimension == D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY) {
			descDS.Texture2DMSArray.ArraySize = 1;
			descDS.Texture2DMSArray.FirstArraySlice = targetDesc.texture2D.arrayIdx;
		}
	} else if (targetDesc.baseTextureType == UniformType::TextureCube) {
		descDS.ViewDimension =
		    (m_desc.texture2D.numSamples == 1) ? D3D11_DSV_DIMENSION_TEXTURE2DARRAY : D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;

		const int faceArrayIdx = signedAxis_toTexCubeFaceIdx_D3D11(targetDesc.textureCube.face);

		if (viewDimension == D3D11_DSV_DIMENSION_TEXTURE2DARRAY) {
			descDS.Texture2DArray.ArraySize = 1;
			descDS.Texture2DArray.FirstArraySlice = faceArrayIdx;
			descDS.Texture2DArray.MipSlice = targetDesc.textureCube.mipLevel;
		} else if (viewDimension == D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY) {
			descDS.Texture2DMSArray.ArraySize = 1;
			descDS.Texture2DMSArray.FirstArraySlice = faceArrayIdx;
		}
	} else {
		sgeAssert(false);
	}

	TComPtr<ID3D11DepthStencilView> dsv;
	const HRESULT hr = d3ddev->CreateDepthStencilView(m_dx11Texture, &descDS, &dsv);

	if (FAILED(hr)) {
		sgeAssert(false);
		return nullptr;
	}

	m_dsvs.push_back({targetDesc, dsv});
	return dsv;
}

//---------------------------------------------------------
void TextureD3D11::destroy() {
	// Unbind the buffer form the context state cache.
	D3D11ContextStateCache* const stateCache = getDevice<SGEDeviceD3D11>()->D3D11_GetContextStateCache();

	if (m_srv != NULL) {
		stateCache->TextureUnbind(&m_srv.p, 1, NULL, 0, NULL, 0);
	}

	for (auto& rtv : m_rtvs) {
		stateCache->TextureUnbind(NULL, 0, &rtv.second.p, 1, NULL, 0);
	}

	for (auto& dsv : m_dsvs) {
		stateCache->TextureUnbind(NULL, 0, NULL, 0, &dsv.second.p, 1);
	}

	// And finally realease the resources.

	m_dx11Texture.Release();

	m_srv.Release();
	m_rtvs.clear();
	m_dsvs.clear();

	m_samplerState.Release();
	m_desc = TextureDesc();
}

//---------------------------------------------------------
bool TextureD3D11::isValid() const {
	return m_dx11Texture != nullptr;
}

//---------------------------------------------------------
bool TextureD3D11::D3D11_WrapOverD3D11TextureResource(SGEDevice* UNUSED(pDevice),
                                                      ID3D11Texture2D* d3d11Texture2D,
                                                      const TextureDesc& desc) {
	// clear the current state
	destroy();

	m_dx11Texture = d3d11Texture2D;
	m_desc = desc;

	D3D11_GetSRV();

	// render target view
	if (desc.textureType == UniformType::Texture2D) {
		D3D11_GetRTV(TargetDesc::FromTex2D());
	} else {
		// [TODO]
		sgeAssert(false);
	}

	// depth stencil view
	if (desc.textureType == UniformType::Texture2D) {
		D3D11_GetDSV(TargetDesc::FromTex2D());
	} else {
		// [TODO]
		sgeAssert(false);
	}


	return true;
}

ID3D11SamplerState* TextureD3D11::D3D11_GetSamplerState() {
	return (m_samplerState.IsResourceValid()) ? m_samplerState.as<SamplerStateD3D11>()->D3D11_GetResource() : NULL;
}
} // namespace sge
