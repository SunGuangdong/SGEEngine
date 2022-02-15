#include "GLContextStateCache.h"
#include "GraphicsCommon_gl.h"
#include "sge_log/Log.h"

namespace sge {

namespace {
	/// Checks if @Variable has a value of @Value.
	/// If they are the same the function returns false.
	/// If they are different the @Variable gets update and returns true.
	template <typename T>
	bool UPDATE_ON_DIFF(T& Variable, const T& Value)
	{
		if (Variable == Value) {
			return false;
		}

		Variable = Value;
		return true;
	}
} // namespace

//---------------------------------------------------------------------------------
// GLContextStateCache
//---------------------------------------------------------------------------------
void* GLContextStateCache::MapBuffer(const GLenum target, const GLenum access)
{
#if !defined(__EMSCRIPTEN__)
	// add some debug error checking because
	// not all buffer targets are supported
	IsBufferTargetSupported(target);

	//
	const BUFFER_FREQUENCY freq = GetBufferTargetByFrequency(target);

	if (m_boundBuffers[freq].buffer == 0) {
		sgeAssert(false && "Trying to call glMapBuffer on slot with no bound buffer!");
		return nullptr;
	}

	m_boundBuffers[freq].isMapped = true;
	void* result = glMapBuffer(target, access);
	DumpAllGLErrors();
	return result;
#else
	sgeLogError("WebGL doesn't support glMapBuffer");
	return nullptr;
#endif
}

void GLContextStateCache::UnmapBuffer(const GLenum target)
{
#if !defined(__EMSCRIPTEN__)
	// add some debug error checking because
	// not all buffer targets are supported
	IsBufferTargetSupported(target);

	//
	const BUFFER_FREQUENCY freq = GetBufferTargetByFrequency(target);

	if (m_boundBuffers[freq].buffer == 0) {
		sgeAssert(false && "Trying to call glMapBuffer on slot with no bound buffer!");
	}

	sgeAssert(m_boundBuffers[freq].isMapped == true);

	m_boundBuffers[freq].isMapped = false;
	glUnmapBuffer(target);
#else
	sgeLogError("WebGL doesn't support glUnmapBuffer");
#endif
}

void GLContextStateCache::BindBuffer(const GLenum bufferTarget, const GLuint buffer)
{
	// add some debug error checking because
	// not all buffer targets are supported
	IsBufferTargetSupported(bufferTarget);

	//
	const BUFFER_FREQUENCY freq = GetBufferTargetByFrequency(bufferTarget);

	if (m_boundBuffers[freq].isMapped) {
		sgeLogError("SGE GLContext API PROHIBITS Buffer Binding when currently bound buffer on that slot is mapped!");
	}

	if (UPDATE_ON_DIFF(m_boundBuffers[freq].buffer, buffer)) {
		glBindBuffer(bufferTarget, buffer);
		DumpAllGLErrors();
	}
}

//---------------------------------------------------------------------
void GLContextStateCache::SetVertexAttribSlotState(const bool bEnabled,
                                                   const GLuint index,
                                                   const GLuint buffer,
                                                   const GLuint size,
                                                   const GLenum type,
                                                   const GLboolean normalized,
                                                   const GLuint stride,
                                                   const GLuint byteOffset)
{
	VertexAttribSlotDesc& currentState = m_vertAttribPointers[index];

	// If currently the slot is enabled just disable it and bypass the call to
	// glVertexAttribPointer.
	// Calling glDisableVertexAttribArray("index") will invalidate the previous glVertexAttribPointer on "index"-th slot.
	bool justEnabled = false;

	if (bEnabled != currentState.isEnabled) {
		currentState.isEnabled = bEnabled;
		if (currentState.isEnabled) {
			justEnabled = true;
			glEnableVertexAttribArray(index);
		}
		else {
			glDisableVertexAttribArray(index);
		}

		DumpAllGLErrors();
	}

	if (currentState.isEnabled) {
		const bool stateDiff = (currentState.buffer != buffer) || (currentState.size != size) || (currentState.type != type) ||
		                       (currentState.normalized != normalized) || (currentState.stride != stride) ||
		                       (currentState.byteOffset != byteOffset);

		BindBuffer(GL_ARRAY_BUFFER, buffer);
		DumpAllGLErrors();

		if (stateDiff || justEnabled) {
			currentState.buffer = buffer;
			currentState.size = size;
			currentState.type = type;
			currentState.normalized = normalized;
			currentState.stride = stride;
			currentState.byteOffset = byteOffset;

			// Caution:
			// Integer vertex attributes needs to be called
			// with glVertexAttribIPointer and not with
			// with glVertexAttribPointer
			// Even if we specify GL_INT as a type. glGetError will not report any errors,
			// but in shader we will not get the integers we've specified.
			if (currentState.type == GL_INT || currentState.type == GL_UNSIGNED_INT) {
				glVertexAttribIPointer(index, currentState.size, currentState.type, currentState.stride,
				                       (GLvoid*)(std::ptrdiff_t(currentState.byteOffset)));
			}
			else {
				glVertexAttribPointer(index, currentState.size, currentState.type, currentState.normalized, currentState.stride,
				                      (GLvoid*)(std::ptrdiff_t(currentState.byteOffset)));
			}
			DumpAllGLErrors();
		}
	}
}

void GLContextStateCache::UseProgram(const GLuint program)
{
	if (UPDATE_ON_DIFF(m_program, program)) {
		glUseProgram(program);
	}
}

void GLContextStateCache::BindUniformBuffer(const GLuint index, const GLuint buffer)
{
	if (UPDATE_ON_DIFF(m_uniformBuffers[index], buffer)) {
		glBindBufferBase(GL_UNIFORM_BUFFER, index, buffer);
	}
}

void GLContextStateCache::SetActiveTexture(const GLenum activeSlot)
{
	sgeAssert(activeSlot >= GL_TEXTURE0 && activeSlot < GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);

	if (m_activeTextureSlot != activeSlot) {
		m_activeTextureSlot = activeSlot;
		glActiveTexture(activeSlot);
		DumpAllGLErrors();
	}
}

void GLContextStateCache::BindTexture(const GLenum texTarget, const GLuint texture)
{
	const int slotIndex = m_activeTextureSlot - GL_TEXTURE0;
	sgeAssert(slotIndex >= 0 && slotIndex < m_textures.size());

	BoundTexture& texSlotState = m_textures[slotIndex];

	// Check if the texture is already bound.
	if (texSlotState.resource == texture && texSlotState.texTarget == texTarget) {
		return;
	}

	// Update the cache and call the OpenGL API to actually bind the texture.
	texSlotState.resource = texture;
	texSlotState.texTarget = texTarget;

	glBindTexture(texTarget, texture);
	DumpAllGLErrors();
}

void GLContextStateCache::BindTextureEx(const GLenum texTarget, const GLenum activeSlot, const GLuint texture)
{
	SetActiveTexture(activeSlot);
	BindTexture(texTarget, texture);
	DumpAllGLErrors();
}

void GLContextStateCache::BindFBO(const GLuint fbo)
{
	if (m_frameBuffer == fbo)
		return;
	m_frameBuffer = fbo;

	// GL_FRAMEBUFFER is the only possible argument.... currently!
	// https://www.khronos.org/opengles/sdk/docs/man/xhtml/glBindFramebuffer.xml
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	DumpAllGLErrors();
}

void GLContextStateCache::setViewport(const sge::GLViewport& vp)
{
	if (!m_viewport.hasValue() || UPDATE_ON_DIFF(m_viewport.get(), vp)) {
		glViewport(vp.x, vp.y, vp.width, vp.height);
		m_viewport = vp;
		DumpAllGLErrors();
	}
}

void GLContextStateCache::ApplyRasterDesc(const RasterDesc& desc)
{
	// Backface culling.
	if (UPDATE_ON_DIFF(m_rasterDesc.cullMode, desc.cullMode)) {
		switch (desc.cullMode) {
			case CullMode::Back:
				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK);
				break;

			case CullMode::Front:
				glEnable(GL_CULL_FACE);
				glCullFace(GL_FRONT);
				break;

			case CullMode::None:
				glDisable(GL_CULL_FACE);
				break;

			default:
				// Unknown cull mode
				sgeAssert(false);
		}
	}

	if (UPDATE_ON_DIFF(m_rasterDesc.backFaceCCW, desc.backFaceCCW)) {
		if (m_rasterDesc.backFaceCCW)
			glFrontFace(GL_CW);
		else
			glFrontFace(GL_CCW);
	}

	DumpAllGLErrors();

	// Fillmode.
#if !defined(__EMSCRIPTEN__) // WebGL 2 does't support fill mode, it is all solid.
	if (UPDATE_ON_DIFF(m_rasterDesc.fillMode, desc.fillMode)) {
		switch (desc.fillMode) {
			case FillMode::Solid:
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				break;

			case FillMode::Wireframe:
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				break;

			default:
				// Unknown fill mode
				sgeAssert(false);
		}
	}
#endif

	bool biasChanged = UPDATE_ON_DIFF(m_rasterDesc.depthBiasAdd, desc.depthBiasAdd);
	biasChanged = UPDATE_ON_DIFF(m_rasterDesc.depthBiasSlope, desc.depthBiasSlope) || biasChanged;
	if (biasChanged) {
		GLenum mode = GL_INVALID_ENUM;
		switch (desc.fillMode) {
			case FillMode::Solid:
				mode = GL_POLYGON_OFFSET_FILL;
				break;

			case FillMode::Wireframe:
#ifndef __EMSCRIPTEN__
				mode = GL_POLYGON_OFFSET_LINE;
#endif
				break;

			default:
				// Unknown fill mode
				sgeAssert(false);
		}

		if (mode != GL_INVALID_ENUM) {
			if (m_rasterDesc.depthBiasAdd != 0.f || m_rasterDesc.depthBiasSlope != 0.f) {
				glEnable(mode);
			}

			glPolygonOffset(m_rasterDesc.depthBiasSlope, m_rasterDesc.depthBiasAdd);
		}
		else {
			glPolygonOffset(0.f, 0.f);
		}
	}

	// Scissors.
	if (UPDATE_ON_DIFF(m_rasterDesc.useScissor, desc.useScissor)) {
		if (desc.useScissor)
			glEnable(GL_SCISSOR_TEST);
		else
			glDisable(GL_SCISSOR_TEST);
	}

	DumpAllGLErrors();
}

void GLContextStateCache::ApplyScissorsRect(GLint x, GLint y, GLsizei width, GLsizei height)
{
	const bool diff = m_scissorsRect.x != x || m_scissorsRect.y != y || m_scissorsRect.width != width || m_scissorsRect.height != height;

	if (diff == false) {
		return;
	}

	m_scissorsRect.x = x;
	m_scissorsRect.y = y;
	m_scissorsRect.width = width;
	m_scissorsRect.height = height;

	glScissor(x, y, width, height);
	DumpAllGLErrors();
}

void GLContextStateCache::DepthMask(const GLboolean enabled)
{
	if (UPDATE_ON_DIFF(m_depthStencilDesc.depthWriteEnabled, enabled == GL_TRUE)) {
		if (enabled)
			glDepthMask(GL_TRUE);
		else
			glDepthMask(GL_FALSE);
		DumpAllGLErrors();
	}
}

void GLContextStateCache::ApplyDepthStencilDesc(const DepthStencilDesc& desc)
{
	if (UPDATE_ON_DIFF(m_depthStencilDesc.depthTestEnabled, desc.depthTestEnabled)) {
		if (desc.depthTestEnabled)
			glEnable(GL_DEPTH_TEST);
		else
			glDisable(GL_DEPTH_TEST);
		DumpAllGLErrors();
	}

	DepthMask(desc.depthWriteEnabled ? GL_TRUE : GL_FALSE);

	if (UPDATE_ON_DIFF(m_depthStencilDesc.comparisonFunc, desc.comparisonFunc)) {
		glDepthFunc(DepthComparisonFunc_GetGLNative(desc.comparisonFunc));
		DumpAllGLErrors();
	}
}

void GLContextStateCache::ApplyBlendState(const BlendDesc& blendDesc)
{
	if (UPDATE_ON_DIFF(m_blendDesc, blendDesc)) {
		if (m_blendDesc.enabled)
			glEnable(GL_BLEND);
		else
			glDisable(GL_BLEND);
		//}

		// if(m_blendDesc != blendDesc)
		//{
		glBlendFuncSeparate(Blend_GetGLNative(m_blendDesc.srcBlend), Blend_GetGLNative(m_blendDesc.destBlend),
		                    Blend_GetGLNative(m_blendDesc.alphaSrcBlend), Blend_GetGLNative(m_blendDesc.alphaDestBlend));

		glBlendEquationSeparate(BlendOp_GetGLNative(m_blendDesc.blendOp), BlendOp_GetGLNative(m_blendDesc.alphaBlendOp));

		m_blendDesc = blendDesc;
	}

	DumpAllGLErrors();
}

void GLContextStateCache::DrawElements(const GLenum primTopology,
                                       const GLuint numIndices,
                                       const GLenum elemArrayBufferFormat,
                                       const GLvoid* indices,
                                       const GLsizei instanceCount)
{
	if (instanceCount == 1)
		glDrawElements(primTopology, numIndices, elemArrayBufferFormat, indices);
	else
		glDrawElementsInstanced(primTopology, numIndices, elemArrayBufferFormat, indices, instanceCount);

	DumpAllGLErrors();
}

void GLContextStateCache::DrawArrays(const GLenum primTopology,
                                     const GLuint startVertex,
                                     const GLuint numVerts,
                                     const GLsizei instanceCount)
{
	if (instanceCount == 1)
		glDrawArrays(primTopology, startVertex, numVerts);
	else
		glDrawArraysInstanced(primTopology, startVertex, numVerts, instanceCount);

	DumpAllGLErrors();
}

void GLContextStateCache::GenBuffers(const GLsizei numBuffers, GLuint* const buffers)
{
	sgeAssert(buffers != nullptr && numBuffers > 0);
	glGenBuffers(numBuffers, buffers);
}

void GLContextStateCache::DeleteBuffers(const GLsizei numBuffers, GLuint* const buffers)
{
	sgeAssert(buffers != nullptr && numBuffers > 0);

	// The tricky part. remove all mentions of any buffer from buffers
	// from the context state cache(aka. unbind everything).
	for (int iBuffer = 0; iBuffer < numBuffers; ++iBuffer) {
		const GLuint buffer = buffers[iBuffer];

		// Active state.
		for (BoundBufferState& boundBuffer : m_boundBuffers) {
			if (boundBuffer.buffer == buffer) {
				// Its illigal to unbind a mapped buffer.  Your logic is probably wrong.
				sgeAssert(boundBuffer.isMapped == false);
				boundBuffer.buffer = 0;
			}
		}

		// Input assembler
		for (int t = 0; t < m_vertAttribPointers.size(); ++t) {
			auto& vad = m_vertAttribPointers[t];
			if (vad.buffer == buffer) {
				SetVertexAttribSlotState(false, t, 0, 1, GL_FLOAT, GL_FALSE, 0, 0);
			}
		}

		// uniform buffers.
		for (GLuint& ubuffer : m_uniformBuffers) {
			if (ubuffer == buffer) {
				ubuffer = 0;
			}
		}
	}

	// And finally delete the buffers.
	glDeleteBuffers(numBuffers, buffers);
}

void GLContextStateCache::GenTextures(const GLsizei numTextures, GLuint* const textures)
{
	sgeAssert(textures != nullptr && numTextures > 0);
	glGenTextures(numTextures, textures);
	DumpAllGLErrors();
}

void GLContextStateCache::DeleteTextures(const GLsizei numTextures, GLuint* const textures)
{
	// "Unbind" the texture from the state cache.
	for (int iTexture = 0; iTexture < numTextures; ++iTexture) {
		const GLuint tex = textures[iTexture];

		for (int iSlot = 0; iSlot < m_textures.size(); ++iSlot) {
			BoundTexture& boundTex = m_textures[iSlot];
			if (boundTex.resource == tex) {
				BindTextureEx(boundTex.texTarget, GL_TEXTURE0 + iSlot, 0);
			}
		}
	}

	glDeleteTextures(numTextures, textures);
}

void GLContextStateCache::GenFrameBuffers(const GLsizei n, GLuint* ids)
{
	sgeAssert(n > 0 && ids != NULL);
	glGenFramebuffers(n, ids);
}

void GLContextStateCache::DeleteFrameBuffers(const GLsizei n, GLuint* ids)
{
	for (int t = 0; t < n; ++t) {
		// HACK: if the currently bound fbo is deleted, I really don't know what happens to the OpenGL state.
		// In order to make the next fbo call successful we imagine that a fbo with index max(GLuint) is bound.
		if (ids[t] == m_frameBuffer) {
			m_frameBuffer = std::numeric_limits<GLuint>::max();
		}
	}

	sgeAssert(n > 0 && ids != NULL);
	glDeleteFramebuffers(n, ids);
}
void GLContextStateCache::DeleteProgram(GLuint program)
{
	if (program == m_program) {
		m_program = 0;
	}

	glDeleteProgram(program);
}

GLContextStateCache::BUFFER_FREQUENCY GLContextStateCache::GetBufferTargetByFrequency(const GLenum bufferTarget)
{
	switch (bufferTarget) {
		case GL_ARRAY_BUFFER:
			return BUFFER_FREQUENCY_ARRAY;
		case GL_ELEMENT_ARRAY_BUFFER:
			return BUFFER_FREQUENCY_ELEMENT_ARRAY;
		case GL_UNIFORM_BUFFER:
			return BUFFER_FREQUENCY_UNIFORM;
	}

	// unimplemented frequency type
	sgeAssert(false);

	// Make the compiler happy and return something.
	return BUFFER_FREQUENCY_ARRAY;
}

bool GLContextStateCache::IsBufferTargetSupported(const GLenum bufferTarget)
{
	switch (bufferTarget) {
		case GL_ARRAY_BUFFER:
		case GL_ELEMENT_ARRAY_BUFFER:
		case GL_UNIFORM_BUFFER:
			return true;
	}
#ifdef SGE_USE_DEBUG
	sgeAssert(false &&
	          "Unsupported buffer target encountered! Everything might be fine, but this something really rare so I wanted to let you know "
	          "that it happend.");
#endif
	return false;
}
//
// struct BoundBufferState {
//	bool isMapped = false;
//	GLuint buffer = 0;
//};

} // namespace sge
