#include "FrameTarget_d3d11.h"
#include "Texture_d3d11.h"

namespace sge {

bool FrameTargetD3D11::create()
{
	// for the sake of having Create!
	destroy();
	return true;
}

//--------------------------------------------------------------
bool FrameTargetD3D11::create(int numRenderTargets,
                              Texture* renderTargets[],
                              TargetDesc renderTargetDescs[],
                              Texture* depthStencil,
                              const TargetDesc& depthTargetDesc)
{
	destroy();

	if (numRenderTargets > 0) {
		sgeAssert(renderTargets);
		sgeAssert(renderTargetDescs);
	}

	for (int iTarget = 0; iTarget < numRenderTargets; ++iTarget) {
		setRenderTarget(iTarget, renderTargets[iTarget], renderTargetDescs[iTarget]);
	}

	setDepthStencil(depthStencil, depthTargetDesc);

	return true;
}

//--------------------------------------------------------------
void FrameTargetD3D11::setRenderTarget(const int slot, Texture* texture, const TargetDesc& targetDesc)
{
	m_renderTargets[slot] = texture;
	m_dx11RTVs[slot] = texture ? ((TextureD3D11*)texture)->D3D11_GetRTV(targetDesc) : NULL;

	updateAttachmentsInfo(texture);
}

//--------------------------------------------------------------
void FrameTargetD3D11::setDepthStencil(Texture* texture, const TargetDesc& targetDesc)
{
	m_depthBuffer = texture;
	m_dx11DSV = ((TextureD3D11*)texture)->D3D11_GetDSV(targetDesc);

	updateAttachmentsInfo(texture);
}

//--------------------------------------------------------------
void FrameTargetD3D11::destroy()
{
	m_frameTargetWidth = -1;
	m_frameTargetHeight = -1;

	for (auto& texture : m_renderTargets) {
		texture.Release();
	}
	m_depthBuffer.Release();

	for (auto& rtv : m_dx11RTVs) {
		rtv.Release();
	}
	m_dx11DSV.Release();
}

//--------------------------------------------------------------
bool FrameTargetD3D11::isValid() const
{
	for (unsigned int t = 0; t < SGE_ARRSZ(m_renderTargets); ++t) {
		if (m_renderTargets[t].IsResourceValid())
			return true;
	}

	return m_depthBuffer.IsResourceValid();
}

Texture* FrameTargetD3D11::getRenderTarget(const unsigned int index) const
{
	return m_renderTargets[index].GetPtr();
}

Texture* FrameTargetD3D11::getDepthStencil() const
{
	return m_depthBuffer.GetPtr();
}

bool FrameTargetD3D11::hasAttachment() const
{
	bool hasRenderTarget = false;

	for (int t = 0; t < SGE_ARRSZ(m_renderTargets); ++t) {
		hasRenderTarget |= m_renderTargets[t].GetPtr() != NULL;
	}

	return hasRenderTarget || (m_depthBuffer.GetPtr() != NULL);
}

void FrameTargetD3D11::updateAttachmentsInfo(Texture* texture)
{
	if (hasAttachment() == false) {
		m_frameTargetWidth = -1;
		m_frameTargetHeight = -1;
	}

	if (texture != NULL) {
		// Only Texture2D is currently implemented.
		if (texture->getDesc().textureType == UniformType::Texture2D) {
			if (m_frameTargetWidth == -1)
				m_frameTargetWidth = texture->getDesc().texture2D.width;
			if (m_frameTargetHeight == -1)
				m_frameTargetHeight = texture->getDesc().texture2D.height;
		}
		else if (texture->getDesc().textureType == UniformType::Texture3D) {
			if (m_frameTargetWidth == -1)
				m_frameTargetWidth = texture->getDesc().texture3D.width;
			if (m_frameTargetHeight == -1)
				m_frameTargetHeight = texture->getDesc().texture3D.height;
		}
		else if (texture->getDesc().textureType == UniformType::TextureCube) {
			if (m_frameTargetWidth == -1)
				m_frameTargetWidth = texture->getDesc().textureCube.width;
			if (m_frameTargetHeight == -1)
				m_frameTargetHeight = texture->getDesc().textureCube.height;
		}
		else {
			// Not implemented.
			sgeAssert(false);
		}
	}
}

bool FrameTargetD3D11::create2D(int width, int height, TextureFormat::Enum renderTargetFmt, TextureFormat::Enum depthTextureFmt)
{
	// Render Target texture.
	GpuHandle<Texture> renderTarget = getDevice()->requestResource<Texture>();
	if (renderTargetFmt != TextureFormat::Unknown) {
		renderTarget = getDevice()->requestResource<Texture>();

		const TextureDesc renderTargetDesc = TextureDesc::GetDefaultRenderTarget(width, height, renderTargetFmt);
		[[maybe_unused]] const bool succeeded = renderTarget->create(renderTargetDesc, NULL);
	}

	// Depth Stencil texture.
	GpuHandle<TextureD3D11> depthStencilTexture;
	if (TextureFormat::IsDepth(depthTextureFmt)) {
		depthStencilTexture = getDevice()->requestResource<Texture>();
		const TextureDesc depthStencilDesc = TextureDesc::GetDefaultDepthStencil(width, height, depthTextureFmt);
		[[maybe_unused]] const bool succeeded = depthStencilTexture->create(depthStencilDesc, NULL);
	}
	else {
		sgeAssert(depthTextureFmt == TextureFormat::Unknown);
	}

	// Create the frame target itself.
	TargetDesc singleColorTargetDesc[] = {TargetDesc::FromTex2D()};
	return create(renderTarget.IsResourceValid() ? 1 : 0, renderTarget.PtrPtr(), singleColorTargetDesc, depthStencilTexture,
	              TargetDesc::FromTex2D());
}


} // namespace sge
