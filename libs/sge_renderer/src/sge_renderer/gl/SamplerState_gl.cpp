#include "SamplerState_gl.h"

namespace sge {


bool SamplerStateGL::create(const SamplerDesc& desc)
{
	m_cachedDesc = desc;
	return true;
}

void SamplerStateGL::destroy()
{
}

bool SamplerStateGL::isValid() const
{
	return true;
}

} // namespace sge
