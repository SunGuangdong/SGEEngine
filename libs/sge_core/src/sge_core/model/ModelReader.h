#pragma once

#include "Model.h"
#include "sge_core/sgecore_api.h"
#include "sge_utils/io/IStream.h"

namespace sge {

/// ModelReader is the class that can load the @Model form a file.
/// All it does it just deserialized the information.
struct SGE_CORE_API ModelReader {
	ModelReader() = default;
	~ModelReader() {}

	bool loadModel(const ModelLoadSettings loadSets, IReadStream* const irs, Model& model);

  private:
	struct DataChunkDesc {
		int chunkId = 0;
		size_t byteOffset = 0;
		size_t sizeBytes = 0;
	};

  private:
	IReadStream* irs;
	std::vector<DataChunkDesc> dataChunksDesc;

	const DataChunkDesc& FindDataChunkDesc(const int chunkId) const;

	/// CAUTION: These functions assume that @irs points at the BEGINING of data chunks.
	template <typename T>
	void loadDataChunk(std::vector<T>& resultBuffer, const int chunkId);
	void loadDataChunkRaw(void* const ptr, const size_t ptrExpectedSize, const int chunkId);
};


} // namespace sge
