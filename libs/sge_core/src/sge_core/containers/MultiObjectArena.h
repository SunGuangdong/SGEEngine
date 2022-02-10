#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "sge_core/typelib/typeLib.h"
#include "sge_utils/containers/ObjectArenaUntyped.h"

namespace sge {

/// MultiObjectArena can allocate, managed and uniquly identify (via a @Handle)
/// Object of any type.
/// The structure uses the reflection system, however if you need you can modify(copy) it.
/// It only needs functions for default constructor, destructor and the size in bytes.
struct MultiObjectArena {
	struct Handle {
		Handle() = default;

		Handle(ObjectArenaUntyped::Handle handleInTypeArena, int localTypeIndex)
		    : handleInTypeArena(handleInTypeArena)
		    , localTypeIndex(localTypeIndex)
		{
		}

		Handle(const Handle& ref)
		    : handleInTypeArena(ref.handleInTypeArena)
		    , localTypeIndex(ref.localTypeIndex)
		{
		}

		Handle(Handle&&) = default;
		Handle& operator=(const Handle&) = default;

		bool isValid() const { return handleInTypeArena.isValid() && localTypeIndex >= 0; }

		ObjectArenaUntyped::Handle handleInTypeArena;
		int localTypeIndex = -1;
	};


	Handle newElement(TypeId typeId, void** ppNewElement)
	{
		Handle outHandle;
		ObjectArenaUntyped* arena = nullptr;

		// Check if we've already create an arena for that specific type.
		auto itrTypeIndex = typeIdToLocalTypeIndex.find(typeId);
		if (itrTypeIndex != typeIdToLocalTypeIndex.end()) {
			// Are for that type already exists. Just use it.
			arena = perTypeArena[itrTypeIndex->second].get();
		}
		else {
			const TypeDesc* typeDesc = typeLib().find(typeId);
			if (typeDesc == nullptr) {
				sgeAssert(false &&
				          "We cannot find the reflection of that object, and we need it to call its constructors and destructors.");
				return Handle();
			}

			ObjectArenaUntyped::ObjectTypeInfo typeInfo;
			typeInfo.constructorFn = typeDesc->constructorFn;
			typeInfo.destructorFn = typeDesc->destructorFn;
			typeInfo.objectSizeInBytes = typeDesc->sizeBytes;

			if (typeInfo.isValid() == false) {
				sgeAssert(false && "The specified type needs to default constructable and destructable.");
				return Handle();
			}

			// Create a new arena for that type.
			typeIdToLocalTypeIndex[typeId] = int(perTypeArena.size());
			perTypeArena.push_back(std::make_unique<ObjectArenaUntyped>());
			std::unique_ptr<ObjectArenaUntyped>& newArena = perTypeArena.back();
			newArena->initialize(typeInfo);

			arena = newArena.get();
			;
		}

		if (arena) {
			ObjectArenaUntyped::Handle handleInTypeArena = arena->newElement();

			if (ppNewElement) {
				*ppNewElement = arena->get(handleInTypeArena);
			}

			return Handle(handleInTypeArena, itrTypeIndex->second);
		}

		sgeAssert(false && "Should never happen");
		return Handle();
	}

	void deleteElement(const Handle& handleToItem)
	{
		if (!handleToItem.isValid()) {
			return;
		}

		perTypeArena[handleToItem.localTypeIndex]->deleteElement(handleToItem.handleInTypeArena);
	}

	void* get(const Handle& handleToItem)
	{
		if (!handleToItem.isValid()) {
			return nullptr;
		}

		void* pObj = perTypeArena[handleToItem.localTypeIndex]->get(handleToItem.handleInTypeArena);
		return pObj;
	}

	const void* get(const Handle& handleToItem) const
	{
		if (!handleToItem.isValid()) {
			return nullptr;
		}

		void* pObj = perTypeArena[handleToItem.localTypeIndex]->get(handleToItem.handleInTypeArena);
		return pObj;
	}

	void clear()
	{
		perTypeArena.clear();
		typeIdToLocalTypeIndex.clear();
	}

  private:
	std::unordered_map<TypeId, int> typeIdToLocalTypeIndex;
	std::vector<std::unique_ptr<ObjectArenaUntyped>> perTypeArena;
};

} // namespace sge
