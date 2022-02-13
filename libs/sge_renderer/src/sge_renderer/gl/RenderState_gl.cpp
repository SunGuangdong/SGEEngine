#include "RenderState_gl.h"

namespace sge {

//--------------------------------------------------------------------------
// RasterizerStateGL
//--------------------------------------------------------------------------
bool RasterizerStateGL::create(const RasterDesc& desc)
{
	m_bufferedDesc = desc;
	m_isValid = true;
	return true;
}

void RasterizerStateGL::destroy()
{
	m_isValid = false;
}

bool RasterizerStateGL::isValid() const
{
	return m_isValid;
}

//--------------------------------------------------------------------------
// DepthStencilStateGL
//--------------------------------------------------------------------------
bool DepthStencilStateGL::create(const DepthStencilDesc& desc)
{
	m_bufferedDesc = desc;
	m_isValid = true;
	return true;
}

void DepthStencilStateGL::destroy()
{
	m_isValid = false;
}

bool DepthStencilStateGL::isValid() const
{
	return m_isValid;
}

//--------------------------------------------------------------------------
// BlendStateGL
//--------------------------------------------------------------------------
bool BlendStateGL::create(const BlendStateDesc& desc)
{
	if (desc.independentBlend != false) {
		sgeAssert(false && "TIndependent blend state is not supported on GL!\n");
	}

	m_bufferedDesc = desc;
	m_isValid = true;
	return true;
}

void BlendStateGL::destroy()
{
	m_isValid = false;
}

bool BlendStateGL::isValid() const
{
	return m_isValid;
}

} // namespace sge
