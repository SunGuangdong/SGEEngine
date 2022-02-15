#pragma once

#include "opengl_include.h"
#include "sge_renderer/renderer/renderer.h"

namespace sge {

struct TextureGL;

//----------------------------------------------------------
// FrameTargetGL
// In terms of GL this is a FrameBuffer object.
// In terms of D3D11 this just a container that contains a bunch of
// render targets and a depth buffer.
//----------------------------------------------------------
struct FrameTargetGL : public FrameTarget {
	FrameTargetGL() {}
	~FrameTargetGL() { destroy(); }

	// Sets render target and depth stencil elements can be NULL.
	bool create(
	    int numRenderTargets,
	    Texture* renderTargets[],
	    TargetDesc renderTargetDescs[],
	    Texture* depthStencil,
	    const TargetDesc& depthTargetDesc) final;

	// In order Create to succeed all textures must share:
	// - the same type
	// - width, height, array size
	// - multisampling settings
	bool create() final;

	// Just a shortcut that makes a single 2D render target and optionally a 2D depth stencil texture.
	bool create2D(
	    int width,
	    int height,
	    TextureFormat::Enum renderTargetFmt = TextureFormat::R8G8B8A8_UNORM,
	    TextureFormat::Enum depthTextureFmt = TextureFormat::D24_UNORM_S8_UINT) final;

	// Attaches(overrides) the color atachment to the FrameTargetGL
	void setRenderTarget(const int slot, Texture* texture, const TargetDesc& targetDesc) final;
	void setDepthStencil(Texture* texture, const TargetDesc& targetDesc) final;

	void destroy() final;

	// Valid if has at least has 1 render target or a depth stencil.
	bool isValid() const final;

	Texture* getRenderTarget(const unsigned int index) const final;
	Texture* getDepthStencil() const final;

	int getWidth() const final { return m_frameTargetWidth; }
	int getHeight() const final { return m_frameTargetHeight; }

	bool hasAttachment() const final;

	// Wraps around the default OpenGL FrameBuffer(with id 0).
	void GL_CreateWindowFrameTarget(int width, int height);
	GLuint GL_GetResource() const { return m_frameBuffer_gl; }
	bool GL_IsWindowFrameBufferWrapper() const { return m_isWindowFrameBufferWrapper_gl; }

  private:
	void updateAttachmentsInfo(Texture* texture);

	// Some statistics about the currently bound textures that are useful.
	int m_frameTargetWidth = -1;
	int m_frameTargetHeight = -1;

	GpuHandle<Texture> m_renderTargets[GraphicsCaps::kRenderTargetSlotsCount];
	GpuHandle<Texture> m_depthBuffer;

	GLuint m_frameBuffer_gl = 0;
	bool m_isWindowFrameBufferWrapper_gl = false;
};

} // namespace sge
