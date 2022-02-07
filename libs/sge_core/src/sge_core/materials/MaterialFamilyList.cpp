#include "MaterialFamilyList.h"
#include "sge_core/materials/IMaterial.h"
#include "sge_utils/json/json.h"

namespace sge {

std::unique_ptr<IMaterial> MaterialFamilyLibrary::loadMaterialFromJson(const JsonValue* jMtlRoot, const char* materialDirectory) const
{
	if (jMtlRoot == nullptr) {
		return std::unique_ptr<IMaterial>();
	}

	try {
		const char* const familyName = jMtlRoot->getMemberOrThrow("family").GetStringOrThrow();

		const MaterialFamilyData* family = this->findFamilyByName(familyName);
		if (family == nullptr || family->familyDesc.mtlAllocFn == nullptr) {
			return false;
		}

		std::unique_ptr<IMaterial> newMtl = family->familyDesc.mtlAllocFn();
		bool succeeded = newMtl && newMtl->fromJson(jMtlRoot, materialDirectory);

		if (succeeded) {
			return newMtl;
		}
	}
	catch (...) {
	}

	sgeAssert(false);
	return std::unique_ptr<IMaterial>();
}

} // namespace sge
