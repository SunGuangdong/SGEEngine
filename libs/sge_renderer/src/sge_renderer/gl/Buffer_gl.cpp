#include "Buffer_gl.h"
#include "GraphicsCommon_gl.h"
#include "GraphicsInterface_gl.h"

namespace sge {

//-----------------------------------------------------------------------------
// Buffer
//-----------------------------------------------------------------------------
bool BufferGL::create(const BufferDesc& desc, const void* const pInitalData)
{
	destroy();

	GLContextStateCache* glcon = getDevice<SGEDeviceImpl>()->GL_GetContextStateCache();

	GLenum bufferBinding = 0;
	if (desc.bindFlags & ResourceBindFlags::VertexBuffer)
		bufferBinding = GL_ARRAY_BUFFER;
	else if (desc.bindFlags & ResourceBindFlags::IndexBuffer)
		bufferBinding = GL_ELEMENT_ARRAY_BUFFER;
	else if (desc.bindFlags & ResourceBindFlags::ConstantBuffer)
		bufferBinding = GL_UNIFORM_BUFFER;
	else
		return false;

	m_bufferDesc = desc;

	glcon->GenBuffers(1, &m_glBuffer);
	DumpAllGLErrors();

	glcon->BindBuffer(bufferBinding, m_glBuffer);
	DumpAllGLErrors();

	// Setup the inital data
	// [TODO][GLCON]
	glBufferData(bufferBinding, desc.sizeBytes, pInitalData, ResourceUsage_GetGLNative(desc.usage));
	DumpAllGLErrors();

#if defined(__EMSCRIPTEN__)
	m_emsc_mapBufferHelper.resize(desc.sizeBytes);
#endif

	return true;
}

void BufferGL::destroy()
{
	m_bufferDesc = BufferDesc();

	if (m_glBuffer != 0) {
		GLContextStateCache* glcon = getDevice<SGEDeviceImpl>()->GL_GetContextStateCache();
		glcon->DeleteBuffers(1, &m_glBuffer);
		m_glBuffer = 0;
	}
}

bool BufferGL::isValid() const
{
	return m_glBuffer != 0;
}

GLenum BufferGL::GL_GetTargetBufferType() const
{
	GLenum target = GL_NONE;

	if (isConstantBuffer())
		target = GL_UNIFORM_BUFFER;
	else if (isVertexBuffer())
		target = GL_ARRAY_BUFFER;
	else if (isIndexBuffer())
		target = GL_ELEMENT_ARRAY_BUFFER;
	else {
		/// Unknown buffer type
		sgeAssert(false);
	}

	return target;
}

void* BufferGL::map([[maybe_unused]] const Map::Enum map, SGEContext* UNUSED(sgecon))
{
#if defined(__EMSCRIPTEN__)
	sgeAssert(m_emsc_mapBufferHelper.size() == m_bufferDesc.sizeBytes);
	return (void*)m_emsc_mapBufferHelper.data();
#else
	GLContextStateCache* const glcon = getDevice<SGEDeviceImpl>()->GL_GetContextStateCache();

	glcon->BindBuffer(GL_GetTargetBufferType(), m_glBuffer);
	void* result = glcon->MapBuffer(GL_GetTargetBufferType(), Map_GetGLNative(map));
	DumpAllGLErrors();
	return result;
#endif
}

void BufferGL::unMap(SGEContext* UNUSED(sgecon))
{
#if defined(__EMSCRIPTEN__)
	GLContextStateCache* const glcon = getDevice<SGEDeviceImpl>()->GL_GetContextStateCache();

	glcon->BindBuffer(GL_GetTargetBufferType(), m_glBuffer);
	glBufferData(GL_GetTargetBufferType(), m_emsc_mapBufferHelper.size(), m_emsc_mapBufferHelper.data(), GL_STATIC_DRAW);
#else
	GLContextStateCache* const glcon = getDevice<SGEDeviceImpl>()->GL_GetContextStateCache();
	glcon->UnmapBuffer(GL_GetTargetBufferType());
#endif
}

} // namespace sge
