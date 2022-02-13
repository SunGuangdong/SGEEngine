#pragma once

#include "sge_core/materials/IGeometryDrawer.h"
#include "sge_core/sgecore_api.h"
#include "sge_log/Log.h"
#include "sge_utils/sge_utils.h"
#include <memory>
#include <string>
#include <unordered_map>


namespace sge {
struct IGeometryDrawer;
struct IMaterial;
struct JsonValue;

struct SGE_CORE_API MaterialFamilyDesc {
	using GeometryDrawerAllocFn = IGeometryDrawer* (*)();
	using MaterialAllocFn = std::unique_ptr<IMaterial> (*)();

	uint32 familyUniqueNumber;
	std::string displayName;
	GeometryDrawerAllocFn geomDrawAllocFn = nullptr;
	MaterialAllocFn mtlAllocFn = nullptr;
};

struct MaterialFamilyLibrary {
	struct MaterialFamilyData {
		MaterialFamilyDesc familyDesc;
		std::unique_ptr<IGeometryDrawer> geometryDrawer = nullptr;
	};

	void registerFamily(const MaterialFamilyDesc& desc)
	{
		auto findItr = m_mtlFamilies.find(desc.familyUniqueNumber);
		if (findItr == m_mtlFamilies.end()) {
			MaterialFamilyData& familyData = m_mtlFamilies[desc.familyUniqueNumber];
			familyData.familyDesc = desc;
			familyData.geometryDrawer.reset(desc.geomDrawAllocFn());

			if (familyData.geometryDrawer == nullptr) {
				sgeLogError("Failed to allocated IGeometryDrawer for family %s.\n", desc.displayName.c_str());
				sgeAssert(false);
			}
		}
		else {
			sgeLogError("A Material Family with id %ull is already registered.\n", desc.familyUniqueNumber);
			sgeAssert(false);
		}
	}

	const MaterialFamilyData* findFamilyById(uint32 familyUniqueNumber) const
	{
		auto findItr = m_mtlFamilies.find(familyUniqueNumber);
		if (findItr != m_mtlFamilies.end()) {
			return &findItr->second;
		}

		return nullptr;
	}

	const MaterialFamilyData* findFamilyByName(const char* familyName) const
	{
		for (const auto& pair : m_mtlFamilies) {
			if (pair.second.familyDesc.displayName == familyName) {
				return &pair.second;
			}
		}

		return nullptr;
	}


	const std::unordered_map<uint32, MaterialFamilyData>& getAllFamilies() const { return m_mtlFamilies; }

	std::unique_ptr<IMaterial> loadMaterialFromJson(const JsonValue* jRoot, const char* materialDirectory) const;

  private:
	std::unordered_map<uint32, MaterialFamilyData> m_mtlFamilies;
};

} // namespace sge
