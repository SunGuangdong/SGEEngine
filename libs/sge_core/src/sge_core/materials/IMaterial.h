#pragma once

#include "sge_core/sgecore_api.h"

#include <string>

namespace sge {

struct SGE_CORE_API IMaterialData {
	IMaterialData() = default;
	virtual ~IMaterialData() = default;
};

struct SGE_CORE_API IMaterial {
	IMaterial() = default;
	virtual ~IMaterial() = default;

	virtual IMaterialData* getMaterialDataLocalStorage() = 0;

	virtual std::string toJson() = 0;
	virtual void fromJson(const char* json) = 0;

	virtual bool getNeedsAlphaSorting() {
		return false;
	}

	virtual float getAlphaMult() {
		return 1.f;
	}
};

} // namespace sge
