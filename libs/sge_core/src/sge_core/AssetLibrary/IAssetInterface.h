#pragma once

#include "sge_core/sgecore_api.h"

namespace sge {

/// IAssetInterface provides as a base class for all Asset interfaces.
struct SGE_CORE_API IAssetInterface {
	IAssetInterface() = default;
	virtual ~IAssetInterface() = default;
};

} // namespace sge
