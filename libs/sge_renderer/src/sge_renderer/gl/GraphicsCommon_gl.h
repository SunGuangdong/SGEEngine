#pragma once

#include "opengl_include.h"
#include "sge_renderer/renderer/renderer.h"

namespace sge {

GLenum ResourceUsage_GetGLNative(const ResourceUsage::Enum type);

// GL_READ_ONLY, GL_WRITE_ONLY, or GL_READ_WRITE
#if !defined(__EMSCRIPTEN__)
GLenum Map_GetGLNative(const Map::Enum map);
#endif

void TextureFormat_GetGLNative(
    const TextureFormat::Enum format, GLint& glInternalFormat, GLenum& glFormat, GLenum& glType);

GLenum TextureDesc_GetGLNativeTextureTartget(const TextureDesc& desc);

// [TODO] http://www.opengl.org/sdk/docs/man/html/glGetActiveUniform.xhtml
UniformType::Enum UniformType_FromGLNumericUniformType(const GLenum uniformType);

// Resturns a suitable data for glVertexAttribPointer.
// Example: for arg (Float3), the resuilt is (GL_FLOAT, 3, GL_FALSE).
void UniformType_ToGLUniformType(const UniformType::Enum type, GLenum& glType, GLint& elemCnt, GLboolean& normalized);

GLenum ShaderType_GetGLNative(const ShaderType::Enum type);

#if !defined(__EMSCRIPTEN__)
GLenum FillMode_GetGLNative(const FillMode::Enum& fillMode);
#endif

GLenum CullMode_GetGLNative(const CullMode::Enum& cullMode);

GLenum DepthComparisonFunc_GetGLNative(const DepthComparisonFunc::Enum& comaprisonFunc);

GLenum PrimitiveTopology_GetGLNative(const PrimitiveTopology::Enum& topology);

GLenum Blend_GetGLNative(Blend::Enum blend);

GLenum BlendOp_GetGLNative(BlendOp::Enum blendOp);

GLenum QueryType_GetGLnative(QueryType::Enum const queryType);

GLenum signedAxis_toTexCubeFaceIdx_OpenGL(const SignedAxis signedAxis);

} // namespace sge
