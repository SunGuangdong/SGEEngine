#include "opengl_include.h"

namespace sge {
// clang-format off
void DumpGLError(const GLenum opengl_error_code) {
	// Those error message are directly taken form Khronos Group OpenGL documentation.
	switch(opengl_error_code)
	{
	case GL_NO_ERROR:
		sgeAssert(false && "[GL_NO_ERROR] No error has been recorded.The value of this symbolic constant is guaranteed to be 0.\n"); break;
	case GL_INVALID_ENUM:
		sgeAssert(false && "[GL_INVALID_ENUM] An unacceptable value is specified for an enumerated argument.The offending command is ignored and has no other side effect than to set the error flag.\n"); break;
	case GL_INVALID_VALUE:
		sgeAssert(false && "[GL_INVALID_VALUE] A numeric argument is out of range.The offending command is ignored and has no other side effect than to set the error flag.\n"); break;
	case GL_INVALID_OPERATION:
		sgeAssert(false && "[GL_INVALID_OPERATION] The specified operation is not allowed in the current state.The offending command is ignored and has no other side effect than to set the error flag.\n"); break;
	case GL_INVALID_FRAMEBUFFER_OPERATION:
		sgeAssert(false && "[GL_INVALID_FRAMEBUFFER_OPERATION] The framebuffer object is not complete.The offending command is ignored and has no other side effect than to set the error flag.\n"); break;
	case GL_OUT_OF_MEMORY:
		sgeAssert(false && "[GL_OUT_OF_MEMORY] There is not enough memory left to execute the command.The state of the GL is undefined, except for the state of the error flags, after this error is recorded.\n"); break;
#if !defined(__EMSCRIPTEN__)
	case GL_STACK_UNDERFLOW:
		sgeAssert(false && "[GL_STACK_UNDERFLOW] An attempt has been made to perform an operation that would cause an internal stack to underflow.\n"); break;
	case GL_STACK_OVERFLOW:
		sgeAssert(false && "[GL_STACK_OVERFLOW] An attempt has been made to perform an operation that would cause an internal stack to overflow.\n"); break;
#endif
	default :
		// unknown error type
		sgeAssert(false && "Unknown OpenGL error!\n");
		sgeAssert(false);
	}
}
// clang-format on

void DumpAllGLErrors()
{
#if !defined(SGE_USE_DEBUG)
	// Dumping errors for EMSCRIPTEN builds is disabled by default it is way too slow, even in debug.
	#if !defined(__EMSCRIPTEN__)
	GLenum opengl_error_code = glGetError();
	while (opengl_error_code != GL_NO_ERROR) {
		// Dump the error message for the error.
		DumpGLError(opengl_error_code);
		sgeAssert(false);
		// Advance to the next error
		opengl_error_code = glGetError();
	}
	#endif
#endif
}

} // namespace sge
