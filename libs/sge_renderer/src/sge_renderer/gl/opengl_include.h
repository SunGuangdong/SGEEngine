#pragma once

#include "sge_utils/sge_utils.h"
#include <memory>

#if !defined(__EMSCRIPTEN__)
    // clang-format off
// Clang Format is turned off here as the include order matters.
#include <GL/glew.h>
#include <GL/gl.h>
// clang-format on
#else
	#include <GLES3/gl3.h>
#endif

#define SGE_GL_UNKNOWN GL_ZERO

namespace sge {

/// Check the OpenGL context if it has any errors.
/// On desktop this will happen only when SGE_USE_DEBUG is defined.
/// For Emscripten it is forced disabled as it is too slow, you can enable it manually.
void DumpAllGLErrors();

inline GLenum GLUniformTypeToTextureType(const GLenum uniformType)
{
	switch (uniformType) {
		// 1D
#if !defined(__EMSCRIPTEN__)
		case GL_SAMPLER_1D:
			return GL_TEXTURE_1D;
		case GL_SAMPLER_1D_SHADOW:
			return GL_TEXTURE_1D;
		case GL_INT_SAMPLER_1D:
			return GL_TEXTURE_1D;
		case GL_UNSIGNED_INT_SAMPLER_1D:
			return GL_TEXTURE_1D;


		case GL_SAMPLER_1D_ARRAY:
			return GL_TEXTURE_1D_ARRAY;
		case GL_SAMPLER_1D_ARRAY_SHADOW:
			return GL_TEXTURE_1D_ARRAY;
		case GL_INT_SAMPLER_1D_ARRAY:
			return GL_TEXTURE_1D_ARRAY;
		case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:
			return GL_TEXTURE_1D_ARRAY;
#endif

		// 2D
		case GL_SAMPLER_2D:
			return GL_TEXTURE_2D;
		case GL_SAMPLER_2D_SHADOW:
			return GL_TEXTURE_2D;
		case GL_INT_SAMPLER_2D:
			return GL_TEXTURE_2D;
		case GL_UNSIGNED_INT_SAMPLER_2D:
			return GL_TEXTURE_2D;

		case GL_SAMPLER_2D_ARRAY:
			return GL_TEXTURE_2D_ARRAY;
		case GL_SAMPLER_2D_ARRAY_SHADOW:
			return GL_TEXTURE_2D_ARRAY;
		case GL_INT_SAMPLER_2D_ARRAY:
			return GL_TEXTURE_2D_ARRAY;
		case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
			return GL_TEXTURE_2D_ARRAY;

			// 2D MultiSampled
#if !defined(__EMSCRIPTEN__)
		case GL_SAMPLER_2D_MULTISAMPLE:
			return GL_TEXTURE_2D_MULTISAMPLE;
		case GL_INT_SAMPLER_2D_MULTISAMPLE:
			return GL_TEXTURE_2D_MULTISAMPLE;
		case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:
			return GL_TEXTURE_2D_MULTISAMPLE;

		case GL_SAMPLER_2D_MULTISAMPLE_ARRAY:
			return GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
		case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
			return GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
		case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
			return GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
#endif

		// Cube
		case GL_SAMPLER_CUBE:
			return GL_TEXTURE_CUBE_MAP;
		case GL_SAMPLER_CUBE_SHADOW:
			return GL_TEXTURE_CUBE_MAP;
		case GL_INT_SAMPLER_CUBE:
			return GL_TEXTURE_CUBE_MAP;
		case GL_UNSIGNED_INT_SAMPLER_CUBE:
			return GL_TEXTURE_CUBE_MAP;

#if !defined(__EMSCRIPTEN__)
		case GL_SAMPLER_CUBE_MAP_ARRAY:
			return GL_TEXTURE_CUBE_MAP;
		case GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW:
			return GL_TEXTURE_CUBE_MAP;
		case GL_INT_SAMPLER_CUBE_MAP_ARRAY:
			return GL_TEXTURE_CUBE_MAP;
		case GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY:
			return GL_TEXTURE_CUBE_MAP;
#endif

		// 3D
		case GL_SAMPLER_3D:
			return GL_TEXTURE_3D;
		case GL_INT_SAMPLER_3D:
			return GL_TEXTURE_3D;
		case GL_UNSIGNED_INT_SAMPLER_3D:
			return GL_TEXTURE_3D;
	}

	// Unknown unsupported texture type.
	sgeAssert(false);
	return GL_NONE;
}

struct GLViewport {
	GLViewport(GLint _x = 0, GLint _y = 0, GLsizei w = 0, GLsizei h = 0)
	    : x(_x)
	    , y(_y)
	    , width(w)
	    , height(h)
	{
	}

	bool operator==(const GLViewport& other)
	{
		return x == other.x && y == other.y && width == other.width && height == other.height;
	}

	bool operator!=(const GLViewport& other) { return !(*this == other); }

	GLint x;
	GLint y;
	GLsizei width;
	GLsizei height;
};

} // namespace sge
