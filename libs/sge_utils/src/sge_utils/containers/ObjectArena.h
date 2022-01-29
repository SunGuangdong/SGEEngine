#pragma once

#include <memory>
#include <vector>

namespace sge {

/// ArenaItem is a handle specifiying a specific item in an ObjectArena.
struct ArenaItem {
	ArenaItem() = default;
	ArenaItem(int elementId, int revision)
	    : elementId(elementId)
	    , revision(revision)
	{
	}

	bool isValid() const
	{
		return elementId >= 0;
	}

	int elementId = -1;
	int revision = 0;
};

/// ObjectArena is space that hold objects.
/// These object are uniquely identified by a @ArenaItem during their lifetime.
/// Use the @ArenaItem to access the object.
/// To not cache pointers to the returned objects, use the handle.
template <class TEelement>
struct ObjectArena {
	static constexpr size_t TNumElementsInChunk = 128;

	struct ChunkElement {
		AlignedStorage<sizeof(TEelement), alignof(TEelement)> elementData;
		/// Each time we re-use this element this is incremented by one.
		/// This provides a way to see if handles point to an old element.
		int revision = 0;
		bool isInUse = false;
	};

	struct Chunk {
		std::unique_ptr<ChunkElement[]> chunkData;
		int touchCount = 0; // the index + 1 of the largest element ever used in the chunk.

		/// The list of previously used elements that are now free.
		std::vector<int> freeList;

		Chunk(const Chunk&) = delete;
		Chunk& operator=(const Chunk&) = delete;

		Chunk()
		{
			chunkData = std::make_unique<ChunkElement[]>(TNumElementsInChunk);
		}

		~Chunk()
		{
			freeList.clear();

			for (int t = 0; t < TNumElementsInChunk; ++t) {
				if (chunkData[t].isInUse) {
					reinterpret_cast<TEelement*>(&chunkData[t].elementData)->~TEelement();
				}
			}
		}

		bool isFull() const
		{
			return touchCount >= TNumElementsInChunk && freeList.empty();
		}

		ChunkElement* findFreeElementNoConstructor(int& outIndexInChunk)
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

			ChunkElement* element = &chunkData[outIndexInChunk];

			element->isInUse = true;


			return element;
		}

		bool deleteElementCallDestructor(const int elementIndex, const int expectedRevision)
		{
			ChunkElement* const element = &chunkData[elementIndex];

			if (element->isInUse && element->revision == expectedRevision) {
				// Call the destructor.
				reinterpret_cast<TEelement*>(&element->elementData)->~TEelement();

				element->isInUse = false;
				element->revision++;

				freeList.push_back(elementIndex);

				return true;
			}

			return false;
		}
	};

  public:
	template <typename... TArgs>
	ArenaItem newElement(TArgs&&... args)
	{
		int chunkIndex;
		int indexInChunk;
		ChunkElement* chunkElement = newElementNoInit(chunkIndex, indexInChunk);

		if (chunkElement) {
			// Call the constructor.
			new (&chunkElement->elementData) TEelement(std::forward<TArgs&&>(args)...);

			// Create the handle.
			int elementId = chunkIndex * TNumElementsInChunk + indexInChunk;
			ArenaItem handle = ArenaItem(elementId, chunkElement->revision);
			return handle;
		}

		return ArenaItem();
	}

	void deleteElement(const ArenaItem& handle)
	{
		if (handle.isValid() == false) {
			return;
		}

		int chunkIndex = handle.elementId / TNumElementsInChunk;
		int indexInChunk = handle.elementId % TNumElementsInChunk;

		m_chunks[chunkIndex]->deleteElementCallDestructor(indexInChunk, handle.revision);
	}

	TEelement* get(const ArenaItem& handle)
	{
		if (handle.isValid() == false) {
			return nullptr;
		}

		int chunkIndex = handle.elementId / TNumElementsInChunk;
		int indexInChunk = handle.elementId % TNumElementsInChunk;

		ChunkElement& chunkElement = m_chunks[chunkIndex]->chunkData[indexInChunk];

		if (chunkElement.isInUse && chunkElement.revision == handle.revision) {
			return (TEelement*)&chunkElement.elementData;
		}

		return nullptr;
	}

	const TEelement* get(const ArenaItem& handle) const
	{
		if (handle.isValid() == false) {
			return nullptr;
		}

		int chunkIndex = handle.elementId / TNumElementsInChunk;
		int indexInChunk = handle.elementId % TNumElementsInChunk;

		const ChunkElement& chunkElement = m_chunks[chunkIndex]->chunkData[indexInChunk];

		if (chunkElement.isInUse && chunkElement.revision == handle.revision) {
			return (const TEelement*)&chunkElement.elementData;
		}

		return nullptr;
	}

  private:
	ChunkElement* newElementNoInit(int& chunkIndex, int& indexInChunk)
	{
		chunkIndex = -1;
		indexInChunk = -1;
		ChunkElement* element = nullptr;

		for (int iChunk = 0; iChunk < m_chunks.size(); ++iChunk) {
			Chunk* chunk = m_chunks[iChunk].get();
			element = chunk->findFreeElementNoConstructor(indexInChunk);
			if (element) {
				chunkIndex = iChunk;
				break;
			}
		}

		if (element == nullptr) {
			// All chunks are full, allocate a new one.
			m_chunks.push_back(std::make_unique<Chunk>());

			element = m_chunks.back()->findFreeElementNoConstructor(indexInChunk);
			chunkIndex = (int)m_chunks.size() - 1;
		}

		return element;
	}

  private:
	std::vector<std::unique_ptr<Chunk>> m_chunks;
};
} // namespace sge
