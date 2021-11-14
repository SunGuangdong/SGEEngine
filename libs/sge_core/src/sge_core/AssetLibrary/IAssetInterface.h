#pragma once


#include "sge_core/sgecore_api.h"

#include "sge_renderer/renderer/renderer.h"

namespace sge {



struct SGE_CORE_API IAssetInterface {
	IAssetInterface() = default;
	virtual ~IAssetInterface() = default;
};



} // namespace sge
