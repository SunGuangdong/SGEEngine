#include "Query_gl.h"
#include "GraphicsCommon_gl.h"
#include "GraphicsInterface_gl.h"

namespace sge {

bool QueryGL::create(QueryType::Enum const queryType)
{
#if 1 || !defined __EMSCRIPTEN__
	destroy();

	glGenQueries(1, &m_glQuery);
	DumpAllGLErrors();

	if (m_glQuery == 0) {
		sgeAssert(false);
		return false;
	}

	m_queryType = queryType;

	return true;
#else
	return false;
#endif
}

void QueryGL::destroy()
{
#if 1 || !defined __EMSCRIPTEN__
	if (m_glQuery) {
		// sgeAssert(glIsQuery(m_glQuery));
		// DumpAllGLErrors();
		glDeleteQueries(1, &m_glQuery);
		DumpAllGLErrors();
	}
#endif
}

bool QueryGL::isValid() const
{
#if 1 || !defined __EMSCRIPTEN__
	#ifdef SGE_USE_DEBUG
	if (m_glQuery) {
		glIsQuery(m_glQuery);
		DumpAllGLErrors();
	}
	#endif

	return m_glQuery != 0;
#else
	return false;
#endif
}

} // namespace sge
