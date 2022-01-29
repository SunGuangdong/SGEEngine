#pragma once

#include "sge_utils/sge_utils.h"
#include <algorithm>
#include <vector>

namespace sge {

template <typename T, int chunk_size = 100>
struct ChunkContainer {
  public:
	enum { CHUNK_SIZE = chunk_size };

	T* at(const int idx) const {
		const size_t chunk = idx / CHUNK_SIZE;

		if (chunk >= chunks.size() || (chunk == chunks.size() - 1 && idx % CHUNK_SIZE >= lastChunkTouchCount)) {
			// Accessing an unallocated element.
			sgeAssert(false);
			return NULL;
		}

		return &chunks[chunk][idx % CHUNK_SIZE];
	}

	T* operator[](const int idx) { return at(idx); }
	const T* operator[](const int idx) const { return at(idx); }

	T* new_element() {
		if (freeList.empty() == false) {
			T* retval = at(freeList.back());
			new (retval) T;
			freeList.pop_back();
			return retval;
		}

		if (chunks.size() > 0 && lastChunkTouchCount < CHUNK_SIZE) {
			T* retval = &chunks.back()[lastChunkTouchCount];
			new (retval) T;
			lastChunkTouchCount += 1;
			return retval;
		}

		// Allocate a new chunk and repeate the process.
		chunks.push_back((T*)malloc(CHUNK_SIZE * sizeof(T)));
		lastChunkTouchCount = 0;
		return new_element();
	}

	// Frees an element in the conteiner.
	void free_element(const int idx) {
		// Check if the element is already deleted.
		auto itr = std::find(begin(freeList), end(freeList), idx);
		if (itr != freeList.end()) {
			return;
		}

		const size_t chunk = idx / CHUNK_SIZE;
		sgeAssert(chunk < chunks.size());

		// Call the destructor and add it to the free list.
		if (idx % CHUNK_SIZE < lastChunkTouchCount) {
			chunks[chunk][idx % CHUNK_SIZE].~T();
			freeList.push_back(idx);
		}
	}

	// Find the index of a pointer.
	// -1 is returned if missing.
	int find_pointer_index(T* const ptr) {
		// Find the chunk of that element
		for (int iChunk = 0; iChunk < (int)chunks.size(); ++iChunk) {
			T* const chunk = chunks[iChunk];
			T* const chunkEnd = chunk + CHUNK_SIZE;
			if ((ptr >= chunk) && (ptr < chunkEnd)) {
				return (int)(iChunk * CHUNK_SIZE + (ptr - chunk));
			}
		}

		return -1;
	}

	// Finds the highest used index.
	// [CAUTION] Keep in mind that it is possible for a element with idx < returnted value to be unitialized.
	// -1 if there if the list is empty.
	int get_highest_count() const {
		if (chunks.size() == 0)
			return 0;
		return ((int)chunks.size() - 1) * CHUNK_SIZE + lastChunkTouchCount;
	}

	bool is_in_freelist(const int idx) const {
		auto itr = std::find(begin(freeList), end(freeList), idx);
		return (itr != freeList.end());
	}

	void free_element(T* const ptr) {
		int idx = find_pointer_index(ptr);

		// Assert that this element exists in that container.
		if (idx < 0) {
			sgeAssert(false);
			return;
		}

		free_element(idx);
	}

	void clear() {
		const int totalSize = (int)(lastChunkTouchCount + ((chunks.size() != 0) ? (chunks.size() - 1) * CHUNK_SIZE : 0));

		// Free all elements.
		// free_element checks internally if the element is already freed!
		for (int t = 0; t < totalSize; ++t) {
			free_element(t);
		}

		for (auto& chunk : chunks)
			free(chunk);

		chunks.clear();
		freeList.clear();
		lastChunkTouchCount = 0;
	}

	ChunkContainer() {}
	~ChunkContainer() { clear(); }

  private:
	std::vector<T*> chunks;      // The allocated chunks.
	std::vector<int> freeList;   // List of free elements(in all chunks).
	int lastChunkTouchCount = 0; // The number of touched elements in the last chunk.
};

} // namespace sge
