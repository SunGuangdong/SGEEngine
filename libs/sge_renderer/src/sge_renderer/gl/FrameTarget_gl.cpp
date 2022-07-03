#include "GraphicsCommon_gl.h"
#include "GraphicsInterface_gl.h"
#include "Texture_gl.h"

#include "FrameTarget_gl.h"

namespace sge {

//---------------------------------------------------------------
// FrameTargetGL
//---------------------------------------------------------------
bool FrameTargetGL::create()
{
	return create(0, nullptr, nullptr, nullptr, TargetDesc());
}

bool FrameTargetGL::create(
    int numRenderTargets,
    Texture* renderTargets[],
    TargetDesc renderTargetDescs[],
    Texture* depthStencil,
    const TargetDesc& depthTargetDesc)
{
	destroy();

	m_frameTargetWidth = -1;
	m_frameTargetHeight = -1;

	GLContextStateCache* const glcon = getDevice<SGEDeviceImpl>()->GL_GetContextStateCache();

	// [TODO] Safety checks
	glcon->GenFrameBuffers(1, &m_frameBuffer_gl);
	glcon->BindFBO(m_frameBuffer_gl);

	// Attach the color and depth buffers.
	for (int t = 0; t < numRenderTargets; ++t) {
		// Initalize the cached widht and height of the all the textures in the frame buffer
		// and use those as a reference values
		if (m_frameTargetWidth == -1 && renderTargets[t] != NULL) {
			// [TODO][FRAME_TARGET_IMPL] Currently only the code for 2D textures at 0 mip level are supproted(that is me
			// just being lazy). Also parameters like, type, array size, multisampling are missing. Honestly this is a
			// big mess that somebody has to write at some point...
			if (renderTargetDescs[t].baseTextureType == UniformType::Texture2D &&
			    renderTargetDescs[t].texture2D.mipLevel == 0 && renderTargetDescs[t].texture2D.arrayIdx == 0) {
				m_frameTargetWidth = renderTargets[t]->getDesc().texture2D.width;
				m_frameTargetHeight = renderTargets[t]->getDesc().texture2D.height;
			}
		}

		setRenderTarget(t, renderTargets[t], renderTargetDescs[t]);
	}

	setDepthStencil(depthStencil, depthTargetDesc);

	DumpAllGLErrors();

	return true;
}

void FrameTargetGL::GL_CreateWindowFrameTarget(int width, int height)
{
	destroy();

	m_isWindowFrameBufferWrapper_gl = true;
	m_frameTargetWidth = width;
	m_frameTargetHeight = height;
}

void FrameTargetGL::setRenderTarget(const int slot, Texture* texture, const TargetDesc& targetDesc)
{
	if (m_isWindowFrameBufferWrapper_gl) {
		sgeAssert(false && "Invalid operation, modifying the window frame buffer is not possible!");
		return;
	}

	// [TODO] Safty checks and validation.
	m_renderTargets[slot] = texture;

	// Just unbinding nothing more to do here.
	if (texture == nullptr) {
		return;
	}

	if (texture->getDesc().textureType == UniformType::Texture2D) {
		updateAttachmentsInfo(texture);

		auto const& desc = texture->getDesc().texture2D;

		// [TODO][FRAME_TARGET_IMPL]
		if (desc.width != m_frameTargetWidth || desc.height != m_frameTargetHeight) {
			sgeAssert(false && "Texture has incompatible metrics with this frame target!");
			return;
		}

		// The shortcut with glFramebufferTexture3D doesn't work on nVidia.
		glFramebufferTexture2D(
		    GL_FRAMEBUFFER,
		    GL_COLOR_ATTACHMENT0 + slot,
		    GL_TEXTURE_2D,
		    ((TextureGL*)texture)->GL_GetResource(),
		    targetDesc.texture2D.mipLevel);

		DumpAllGLErrors();
	}
	else {
		// Unimplemented...
		sgeAssert(false);
	}
}

void FrameTargetGL::setDepthStencil(Texture* texture, const TargetDesc& targetDesc)
{
	if (m_isWindowFrameBufferWrapper_gl) {
		sgeAssert(false && "Invalid operation, modifying the window frame buffer is not possible!");
		return;
	}

	// [TODO] Safety checks and validation.
	m_depthBuffer = texture;

	if (texture == nullptr) {
		return;
	}

	GLContextStateCache* const glcon = getDevice<SGEDeviceImpl>()->GL_GetContextStateCache();

	const TextureDesc& texToBindDesc = texture->getDesc();

	// pick if this is a depth-stencil of just depth texture
	const GLenum attachmentType =
	    TextureFormat::IsDepthWithStencil(texToBindDesc.format) ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;

	if (texToBindDesc.textureType == UniformType::Texture2D) {
		updateAttachmentsInfo(texture);
		glcon->BindFBO(m_frameBuffer_gl);
		glFramebufferTexture2D(
		    GL_FRAMEBUFFER,
		    attachmentType,
		    GL_TEXTURE_2D,
		    ((TextureGL*)texture)->GL_GetResource(),
		    targetDesc.texture2D.mipLevel);
		DumpAllGLErrors();

		// Only works on AMD:
		// glFramebufferTexture3D(
		//	GL_FRAMEBUFFER, attachmentType, GL_TEXTURE_2D_ARRAY,
		//	texture->GL_GetResource().id, targetDesc.texture2D.mipLevel, targetDesc.texture2D.arrayIdx);
	}
	else if (texToBindDesc.textureType == UniformType::TextureCube) {
		updateAttachmentsInfo(texture);

		// Binding a face from a cube texture as a depth-stencil.
		const GLenum faceGL = signedAxis_toTexCubeFaceIdx_OpenGL(targetDesc.textureCube.face);
		glcon->BindFBO(m_frameBuffer_gl);
		glFramebufferTexture2D(
		    GL_FRAMEBUFFER,
		    attachmentType,
		    faceGL,
		    ((TextureGL*)texture)->GL_GetResource(),
		    targetDesc.textureCube.mipLevel);

		DumpAllGLErrors();
	}
	else {
		sgeAssert(false);
	}
}

void FrameTargetGL::destroy()
{
	if (m_frameBuffer_gl != 0) {
		auto glcon = getDevice<SGEDeviceImpl>()->GL_GetContextStateCache();
		glcon->DeleteFrameBuffers(1, &m_frameBuffer_gl);
		m_frameBuffer_gl = 0;
	}

	m_frameTargetWidth = -1;
	m_frameTargetHeight = -1;
	m_isWindowFrameBufferWrapper_gl = false;

	// release the render targets and the depth buffer
	for (auto& renderTargetTexture : m_renderTargets) {
		renderTargetTexture.Release();
	}

	m_depthBuffer.Release();
}

bool FrameTargetGL::isValid() const
{
	if (m_isWindowFrameBufferWrapper_gl) {
		return true;
	}

	if (m_frameBuffer_gl == 0) {
		return false;
	}

	// Assume as valid if there is alleast one rendertarget or depth buffer
	for (unsigned int t = 0; t < SGE_ARRSZ(m_renderTargets); ++t) {
		if (m_renderTargets[t].IsResourceValid())
			return true;
	}

	return m_depthBuffer.IsResourceValid();
}

Texture* FrameTargetGL::getRenderTarget(const unsigned int index) const
{
	if (m_isWindowFrameBufferWrapper_gl) {
		sgeAssert(false && "Invalid operation, calling getRenderTarget on the window frame buffer is not possible!");
		return NULL;
	}

	return m_renderTargets[index].GetPtr();
}

Texture* FrameTargetGL::getDepthStencil() const
{
	if (m_isWindowFrameBufferWrapper_gl) {
		sgeAssert(false && "Invalid operation, calling getDepthStencil on the window frame buffer is not possible!");
		return NULL;
	}

	return m_depthBuffer.GetPtr();
}

bool FrameTargetGL::hasAttachment() const
{
	bool hasRenderTarget = false;

	for (int t = 0; t < SGE_ARRSZ(m_renderTargets); ++t) {
		hasRenderTarget |= m_renderTargets[t].GetPtr() != NULL;
	}

	return hasRenderTarget || (m_depthBuffer.GetPtr() != NULL);
}

void FrameTargetGL::updateAttachmentsInfo(Texture* texture)
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

bool FrameTargetGL::create2D(
    int width, int height, TextureFormat::Enum renderTargetFmt, TextureFormat::Enum depthTextureFmt)
{
	// Render Target texture.
	GpuHandle<Texture> renderTarget = getDevice()->requestResource<Texture>();
	if (renderTargetFmt != TextureFormat::Unknown) {
		renderTarget = getDevice()->requestResource<Texture>();

		const TextureDesc renderTargetDesc = TextureDesc::GetDefaultRenderTarget(width, height, renderTargetFmt);
		const bool succeeded = renderTarget->create(renderTargetDesc, nullptr);
	}

	// Depth Stencil texture.
	GpuHandle<TextureGL> depthStencilTexture;
	if (TextureFormat::IsDepth(depthTextureFmt)) {
		depthStencilTexture = getDevice()->requestResource<Texture>();
		const TextureDesc depthStencilDesc = TextureDesc::GetDefaultDepthStencil(width, height, depthTextureFmt);
		SamplerDesc samplerDesc;
		samplerDesc.filter = TextureFilter::Min_Mag_Mip_Point;
		samplerDesc.addressModes[0] = TextureAddressMode::ClampEdge;
		samplerDesc.addressModes[1] = TextureAddressMode::ClampEdge;
		samplerDesc.addressModes[2] = TextureAddressMode::ClampEdge;
		const bool succeeded = depthStencilTexture->create(depthStencilDesc, nullptr);
	}
	else {
		sgeAssert(depthTextureFmt == TextureFormat::Unknown);
	}

	// Create the frame target itself.
	TargetDesc tex2DDesc = TargetDesc::FromTex2D();
	return create(
	    renderTarget.IsResourceValid() ? 1 : 0,
	    renderTarget.PtrPtr(),
	    &tex2DDesc,
	    depthStencilTexture,
	    TargetDesc::FromTex2D());
}


} // namespace sge
