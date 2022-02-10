#pragma once

#include "sge_utils/sge_utils.h"

#include <memory>
#include <vector>

namespace sge {

/// ObjectArena is space that hold objects.
/// These object are uniquely identified by a @ArenaItem during their lifetime.
/// Use the @ArenaItem to access the object.
/// To not cache pointers to the returned objects, use the handle.
struct ObjectArenaUntyped {
	struct Handle {
		Handle() = default;
		Handle(int elementId, int revision)
		    : elementId(elementId)
		    , revision(revision)
		{
		}

		Handle(const Handle& ref)
		    : elementId(ref.elementId)
		    , revision(ref.revision)
		{
		}

		Handle(Handle&&) = default;
		Handle& operator=(const Handle&) = default;

		bool isValid() const { return elementId >= 0; }

		int elementId = -1;
		int revision = 0;
	};

	static constexpr size_t TNumElementsInChunk = 128;

	struct ObjectMemoryStatus {
		/// Each time we re-use this element this is incremented by one.
		/// This provides a way to see if handles point to an old element.
		int revision = 0;
		bool isInUse = false;
	};

	struct ObjectTypeInfo {
		void (*constructorFn)(void* dest) = nullptr;
		void (*destructorFn)(void* dest) = nullptr;
		size_t objectSizeInBytes = 0; ///< A cached version extracted from the TypeDesc.

		bool isValid() const { return constructorFn && destructorFn && objectSizeInBytes > 0; }
	};

	struct Chunk {
		ObjectTypeInfo objectInfo;

		std::unique_ptr<char[]> chunkData;
		std::unique_ptr<ObjectMemoryStatus[]> perObjStatus;
		int touchCount = 0; // the index + 1 of the largest element ever used in the chunk.

		/// The list of previously used elements that are now free.
		std::vector<int> freeList;

		Chunk(const Chunk&) = delete;
		Chunk& operator=(const Chunk&) = delete;

		Chunk() {}

		~Chunk()
		{
			if (chunkData && perObjStatus) {
				sgeAssert(objectInfo.isValid() &&
				          "Are you sure the type didn't change or if it is correct? It is expected that the type is destructable.");

				if (objectInfo.isValid()) {
					freeList.clear();
					for (int t = 0; t < TNumElementsInChunk; ++t) {
						if (perObjStatus[t].isInUse) {
							void* const elemMemLoc = getElementMemoryUnsafe(t);
							objectInfo.destructorFn(elemMemLoc);
						}
					}
				}
			}
		}

		void initialize(ObjectTypeInfo objectInfoToUse)
		{
			this->objectInfo = objectInfoToUse;
			sgeAssert(objectInfo.isValid());

			chunkData = std::make_unique<char[]>(TNumElementsInChunk * objectInfo.objectSizeInBytes);
			perObjStatus = std::make_unique<ObjectMemoryStatus[]>(TNumElementsInChunk);
		}

		void* getElementMemoryUnsafe(const int iElement) const
		{
			sgeAssert(chunkData && iElement >= 0 && iElement < TNumElementsInChunk && objectInfo.isValid());
			void* memory = chunkData.get() + objectInfo.objectSizeInBytes * iElement;
			return memory;
		}

		bool isFull() const { return touchCount >= TNumElementsInChunk && freeList.empty(); }

		void* findFreeElementNoConstructor(int& outIndexInChunk, int& outRevision)
		{
			outIndexInChunk = -1;
			if (freeList.size() > 0) {
				outIndexInChunk = freeList.back();
				freeList.pop_back();
			}
			else if (touchCount < TNumElementsInChunk) {
				outIndexInChunk = touchCount;
				touchCount += 1;
			}

			if (outIndexInChunk < 0) {
				return nullptr;
			}

			ObjectMemoryStatus* objStatus = &perObjStatus[outIndexInChunk];
			objStatus->isInUse = true;

			void* objectMemory = getElementMemoryUnsafe(outIndexInChunk);
			outRevision = objStatus->revision;

			return objectMemory;
		}

		bool deleteElementCallDestructor(const int elementIndex, const int expectedRevision)
		{
			ObjectMemoryStatus& objStatus = perObjStatus[elementIndex];
			if (objStatus.isInUse && objStatus.revision == expectedRevision) {
				// Call the destructor.
				void* object = getElementMemoryUnsafe(elementIndex);
				objectInfo.destructorFn(object);

				objStatus.isInUse = false;
				objStatus.revision++;

				freeList.push_back(elementIndex);

				return true;
			}

			return false;
		}
	};

  public:
	void initialize(ObjectTypeInfo objectInfoToUse)
	{
		this->objectInfo = objectInfoToUse;
		sgeAssert(objectInfo.isValid());
	}

	Handle newElement()
	{
		int chunkIndex;
		int indexInChunk;
		int revision;
		void* pObjMem = newElementNoInit(chunkIndex, indexInChunk, revision);

		if (pObjMem) {
			// Call the constructor.
			objectInfo.constructorFn(pObjMem);

			// Create the handle.
			int elementId = chunkIndex * TNumElementsInChunk + indexInChunk;
			Handle handle = Handle(elementId, revision);
			return handle;
		}

		return Handle();
	}

	void deleteElement(const Handle& handle)
	{
		if (handle.isValid() == false) {
			return;
		}

		int chunkIndex = handle.elementId / TNumElementsInChunk;
		int indexInChunk = handle.elementId % TNumElementsInChunk;

		m_chunks[chunkIndex]->deleteElementCallDestructor(indexInChunk, handle.revision);
	}

	void* get(const Handle& handle)
	{
		if (handle.isValid() == false) {
			return nullptr;
		}

		int chunkIndex = handle.elementId / TNumElementsInChunk;
		int indexInChunk = handle.elementId % TNumElementsInChunk;

		ObjectMemoryStatus& objMemStatus = m_chunks[chunkIndex]->perObjStatus[indexInChunk];

		if (objMemStatus.isInUse && objMemStatus.revision == handle.revision) {
			void* pObj = m_chunks[chunkIndex]->getElementMemoryUnsafe(indexInChunk);
			return pObj;
		}

		return nullptr;
	}

	const void* get(const Handle& handle) const
	{
		if (handle.isValid() == false) {
			return nullptr;
		}

		int chunkIndex = handle.elementId / TNumElementsInChunk;
		int indexInChunk = handle.elementId % TNumElementsInChunk;

		ObjectMemoryStatus& objMemStatus = m_chunks[chunkIndex]->perObjStatus[indexInChunk];

		if (objMemStatus.isInUse && objMemStatus.revision == handle.revision) {
			void* pObj = m_chunks[chunkIndex]->getElementMemoryUnsafe(indexInChunk);
			return pObj;
		}

		return nullptr;
	}

  private:
	void* newElementNoInit(int& chunkIndex, int& indexInChunk, int& revision)
	{
		chunkIndex = -1;
		indexInChunk = -1;
		revision = -1;
		void* pObjectMem = nullptr;

		for (int iChunk = 0; iChunk < m_chunks.size(); ++iChunk) {
			Chunk* chunk = m_chunks[iChunk].get();
			pObjectMem = chunk->findFreeElementNoConstructor(indexInChunk, revision);
			if (pObjectMem) {
				chunkIndex = iChunk;
				break;
			}
		}

		if (pObjectMem == nullptr) {
			// All chunks are full, allocate a new one.
			m_chunks.push_back(std::make_unique<Chunk>());
			m_chunks.back()->initialize(objectInfo);
			pObjectMem = m_chunks.back()->findFreeElementNoConstructor(indexInChunk, revision);
			chunkIndex = (int)m_chunks.size() - 1;
		}

		sgeAssert(revision >= 0);

		return pObjectMem;
	}

  private:
	ObjectTypeInfo objectInfo;
	std::vector<std::unique_ptr<Chunk>> m_chunks;
};

} // namespace sge
