#pragma once

#include "sge_core/sgecore_api.h"
#include "sge_utils/json/json.h"
#include <memory>

namespace sge {

struct Model;
struct KeyFrames;

class SGE_CORE_API ModelWriter {
  public:
	struct DataChunk {
		DataChunk() = default;

		DataChunk(int id, const void* data, size_t sizeBytes)
		    : id(id)
		    , data(data)
		    , sizeBytes(sizeBytes) {}

		int id;
		const void* data;
		size_t sizeBytes;
	};

	ModelWriter() = default;
	~ModelWriter() {}

	bool write(const Model& modelToWrite, IWriteStream* iws);
	bool write(const Model& modelToWrite, const char* const filename);


  private:
	// Returns the chunk id.
	int newDataChunkFromPtr(const void* const ptr, const size_t sizeBytes);
	char* newDataChunkWithSize(size_t sizeBytes, int& outChunkId);

	// This function assumes that the vector wont be resized(aka. the data pointer won't change).
	template <typename T>
	int newChunkFromStdVector(const std::vector<T>& data) {
		return newDataChunkFromPtr(data.data(), data.size() * sizeof(T));
	}

	// The actiual writer is implemented in those functions.
	void writeAnimations();
	void writeNodes();     // Add the "nodes" to the root.
	void writeMaterials(); // Add the "materials" to the root.
	void writeMeshes();
	void writeCollisionData();

	/// @brief Generates the json and allocated a data chunks for the specified keyframes.
	JsonValue* generateKeyFrames(const KeyFrames& keyfames);

	JsonValueBuffer jvb;
	std::vector<DataChunk> dataChunks; // A list of the data chunks that will end up written to the file.
	JsonValue* root;                   // The file header json element.
	const Model* model;                // The working model

	/// Ultra ugly, we assume that these elements are going to get moved.
	/// [MODERL_WRITER_MOVE_ASSUME]
	std::vector<std::vector<char>> m_dynamicallyAlocatedPointersToDelete;
};

} // namespace sge
