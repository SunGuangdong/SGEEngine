/////////////////////////////////////////////////////////////////////////////////////////////////////
// [NOTE][CAUTION]
// Known problems :
//
// - Deleteing a frame buffer, that also deletes it's attachements,
// WILL NOT unbind its attachements form if any of them is bound.
//
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "opengl_include.h"

#include "sge_renderer/renderer/GraphicsCommon.h"
#include "sge_utils/containers/Optional.h"
#include "sge_utils/containers/Pair.h"
#include "sge_utils/containers/StaticArray.h"

namespace sge {

/// GLContextStateCache is the API wrapper around OpenGL.
/// Please make all OpenGL API calls trough this class. It keeps tracks of the current state
/// of all bound resources and and rendering states. This way we can reduce the needless API calls
/// to OpenGL if we are apply the same state that is already bound.
/// In addition this will automatically unbind all delete resources.
///
/// Known problems :
/// - Deleteing a frame buffer, that also deletes it's attachements,
///   WILL NOT unbind its attachements form if any of them is bound.
struct GLContextStateCache {
	struct ScissorRect {
		ScissorRect() = default;
		GLint x = 0;
		GLint y = 0;
		GLsizei width = 0;
		GLsizei height = 0;
	};


	struct VertexAttribSlotDesc {
		// glVertexAttribPointer arguments + the target buffer.
		VertexAttribSlotDesc(
		    bool a_enabled = false,
		    GLuint a_buffer = 0,      // the buffer to be bound as ARRAY BUFFER
		    GLuint a_size = 1,        // 0 isn't accepted by standard.
		    GLenum a_type = GL_FLOAT, // GL_NONE isn't accepted by standard.
		    GLboolean a_normalized = GL_FALSE,
		    GLuint a_stride = 0,     // vertex buffer element size.
		    GLuint a_byteOffset = 0) // data offset in the buffer stride.
		    : isEnabled(a_enabled)
		    , buffer(a_buffer)
		    , size(a_size)
		    , type(a_type)
		    , normalized(a_normalized)
		    , stride(a_stride)
		    , byteOffset(a_byteOffset)
		{
		}

		bool isEnabled;
		GLuint buffer;
		GLuint size;
		GLenum type;
		GLboolean normalized;
		GLuint stride;
		GLuint byteOffset;
	};

	// Bond textures description.
	struct BoundTexture {
		GLenum texTarget = GL_NONE; ///< GL_TEXTURE_2D ... ect
		GLuint resource = 0;        ///< The handle to the OpenGL resources.
	};

  public:
	GLContextStateCache()
	{
		//[TODO] Proper default OpenGL states.
		m_rasterDesc.backFaceCCW = false;
		m_rasterDesc.fillMode = FillMode::Solid;
		m_rasterDesc.cullMode = CullMode::Back;
		m_depthStencilDesc.depthTestEnabled = false;
		for (auto& v : m_boundBuffers) {
			v.buffer = 0;
			v.isMapped = false;
		}
	}

	// https://www.opengl.org/sdk/docs/man/html/glMapBuffer.xhtml
	void* MapBuffer(
	    const GLenum target,
	    const GLenum access // = GL_READ_ONLY, GL_WRITE_ONLY, GL_READ_WRITE
	);

	void UnmapBuffer(const GLenum target);

	/// A wrapper aound glBindBuffer.
	/// @param bufferTarget - GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER, ect...
	/// @param buffer - buffer to be bound.
	void BindBuffer(const GLenum bufferTarget, const GLuint buffer);

	//
	//[NOTE]Just don't use that function
	//@index - attribute pointer index
	//@enabled - should vertex attrib be enabled. If false or buffer == 0 then the call to glVertexAttribPointer is
	// bypassed
	//@attribData - glVertexAttribPointer arguments excluding index
	// void BindVertexAttribPointer2(const GLuint index, const bool enabled, const VertexAttribPointerData2&
	// attribData);
	void SetVertexAttribSlotState(
	    const bool bEnabled,
	    const GLuint index,
	    const GLuint buffer,
	    const GLuint size,
	    const GLenum type,
	    const GLboolean normalized,
	    const GLuint stride,
	    const GLuint byteOffset);

	/// Binds the specified shading program. Basically calls glUseProgram.
	/// @param program resource id to get bound.
	void UseProgram(const GLuint program);

	/// Calls glBindBufferBase(GL_UNIFORM_BUFFER, index, buffer)
	/// @param index - binding location
	/// @param buffer - uniform buffer to be bound
	void BindUniformBuffer(const GLuint index, const GLuint buffer);

	/// A wrapper around glActiveTexture, witch sets the current slot we are modifying (GL_TEXTURE0, GL_TEXTURE1 ...).
	/// Prefer using @BindTextureEx.
	void SetActiveTexture(const GLenum activeSlot);

	/// A wrapper around glBindTexture.
	/// Prefer using @BindTextureEx.
	void BindTexture(const GLenum texTarget, const GLuint texture);

	/// A shorcut for @SetActiveTexture + @BindTexture.
	/// @param texTarget is the type of the texture: GL_TEXTURE_2D, GL_TEXTURE_3D and so on.
	/// @param activeSlot is the slot where the textue is going th get bound: GL_TEXTURE0, GL_TEXTURE1 ...
	/// @param texture is the resource id to be bound.
	void BindTextureEx(const GLenum texTarget, const GLenum activeSlot, const GLuint texture);

	void BindFBO(const GLuint fbo);
	void setViewport(const sge::GLViewport& vp);
	void ApplyRasterDesc(const RasterDesc& rasterDesc);
	void ApplyScissorsRect(GLint x, GLint y, GLsizei width, GLsizei height);

	void DepthMask(const GLboolean enabled);
	void ApplyDepthStencilDesc(const DepthStencilDesc& desc);

	void ApplyBlendState(const BlendDesc& blendDesc);

	// Almost equivalent to "DrawIndexed" in D3D11 (the base vertex location is missing)
	void DrawElements(
	    const GLenum primTopology,
	    const GLuint numIndices,
	    const GLenum elemArrayBufferFormat,
	    const GLvoid* indices,
	    const GLsizei instanceCount = 1);

	// equivalent to "Draw" in D3D11
	void DrawArrays(
	    const GLenum primTopology, const GLuint startVertex, const GLuint numVerts, const GLsizei instanceCount = 1);

	void GenBuffers(const GLsizei numBuffers, GLuint* const buffers);
	void DeleteBuffers(const GLsizei numBuffers, GLuint* const buffers);

	void GenTextures(const GLsizei numTextures, GLuint* const textures);
	void DeleteTextures(const GLsizei numTextures, GLuint* const textures);

	void GenFrameBuffers(const GLsizei n, GLuint* ids);
	void DeleteFrameBuffers(const GLsizei n, GLuint* ids);

	GLuint CreateProgram() { return glCreateProgram(); }
	void DeleteProgram(GLuint program);

  private:
	RasterDesc m_rasterDesc = {false, CullMode::Back, FillMode::Solid, false};
	ScissorRect m_scissorsRect;
	DepthStencilDesc m_depthStencilDesc;
	BlendDesc m_blendDesc; // [TODO][NOTE] Multiple render target blend states are supported for GL.

	// glBindBuffer possible target arguments in array friendly manner
	enum BUFFER_FREQUENCY {
		BUFFER_FREQUENCY_ARRAY = 0,
		BUFFER_FREQUENCY_ELEMENT_ARRAY,
		BUFFER_FREQUENCY_UNIFORM,

		NUM_BUFFER_FREQUENCY
	};

	static BUFFER_FREQUENCY GetBufferTargetByFrequency(const GLenum bufferTarget);
	static bool IsBufferTargetSupported(const GLenum bufferTarget);

	struct BoundBufferState {
		bool isMapped = false;
		GLuint buffer = 0;
	};

	// Found buffer for every buffer target (array, element array, uniform...)
	std::array<BoundBufferState, BUFFER_FREQUENCY::NUM_BUFFER_FREQUENCY> m_boundBuffers;
	std::array<VertexAttribSlotDesc, 16> m_vertAttribPointers;

	GLenum m_activeTextureSlot = GL_TEXTURE0;
	std::array<BoundTexture, 32> m_textures;

	GLuint m_program = 0;

	// aguments of glBindBufferBase(GL_UNIFORM_BUFFER, idx, uniformBuffers[idx])
	std::array<GLuint, 16> m_uniformBuffers;

	// THe bound framebuffer;
	GLuint m_frameBuffer = 0;

	/// The bound viewport. If the optional is empty than no viewport has been bound so far.
	Optional<sge::GLViewport> m_viewport;
};

} // namespace sge
