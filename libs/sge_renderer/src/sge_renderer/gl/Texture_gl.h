#pragma once

#include "opengl_include.h"
#include "sge_renderer/renderer/renderer.h"

namespace sge {

//----------------------------------------------------------
// Texture
//----------------------------------------------------------
struct TextureGL : public Texture {
	TextureGL() = default;
	~TextureGL() { destroy(); }

	// Create a texture with(if possible) inital data.
	// Initial data should be ordered in that way
	// arrayElem0(mip0, mip1, mip2, ...)
	// arrayElem1(mip0, mip1, mip2, ...)
	// arrayElem2(mip0, mip1, mip2, ...)
	bool create(
	    const TextureDesc& desc, const TextureData initalData[], const SamplerDesc sampler = SamplerDesc()) final;

	virtual void destroy() final;
	virtual bool isValid() const final;

	const TextureDesc& getDesc() const final { return m_desc; }
	SamplerState* getSamplerState() final { return m_samplerState; }
	void setSamplerState(SamplerState* ss) final;

	GLuint GL_GetResource() { return m_glTexture; }

  private:
	void applySamplerDesc(const SamplerDesc& samplerDesc, bool shouldBindAndUnBindtexture);

	TextureDesc m_desc;
	GpuHandle<SamplerState> m_samplerState;
	GLuint m_glTexture = 0;
};

} // namespace sge
