#pragma once

#include "sge_core/sgecore_api.h"

#include <string>

namespace sge {

struct SGE_CORE_API IMaterialData {
	IMaterialData(uint32 materialFamilyId)
	    : materialFamilyId(materialFamilyId) {
	}
	virtual ~IMaterialData() = default;

	uint32 materialFamilyId = 0;
	float alphaMultipler = 1.f;
	bool needsAlphaSorting = false;
};

struct SGE_CORE_API IMaterial {
	IMaterial() = default;
	virtual ~IMaterial() = default;

	virtual IMaterialData* getMaterialDataLocalStorage() = 0;

	virtual std::string toJson() = 0;
	virtual void fromJson(const char* json) = 0;
};

} // namespace sge
