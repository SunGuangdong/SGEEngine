#include "ShadingProgramPermuatorCache.h"
#include "sge_utils/base64/base64.h"
#include "sge_utils/hash/hash_combine.h"
#include "sge_utils/io/FileStream.h"
#include "sge_utils/json/json.h"
#include "sge_utils/text/Path.h"
#include "sge_utils/text/format.h"

namespace sge {

bool ShadingProgramPermuatorCache::saveToFile(const char* cacheFilename) const
{
	std::string fileDir = extractFileDir(cacheFilename, true);
	if (!fileDir.empty()) {
		createDirectory(fileDir.c_str());
	}

	JsonValueBuffer jvb;

	JsonValue* jRoot = jvb(JID_MAP);
	JsonValue* jSrcFiles = jRoot->setMember("sourceFiles", jvb(JID_ARRAY));

	// Find all files that are used for compiling the shader and compute a hash based on their contents.
	// Later this hash is going to be used to see if the file has been changed or not.
	for (const auto& fileNameHashPair : sourceFileContentsHash) {
		JsonValue* jFileHashPair = jSrcFiles->arrPush(jvb(JID_MAP));
		jFileHashPair->setMember("file", jvb(fileNameHashPair.first));
		jFileHashPair->setMember("hash", jvb(fileNameHashPair.second));
	}

	// Grab the bytecode for each shading program shaders,
	// and encode it to base64 (in order for it to be a valid json string).
	JsonValue* jPermutationsBytecodes = jRoot->setMember("shaderBytecodesPerPermutation", jvb(JID_ARRAY));
	for (const ShadingProgramByteCode& bytecode : perPermutBytecode) {
		JsonValue* jShadeProgByteCode = jPermutationsBytecodes->arrPush(jvb(JID_MAP));

		std::string vsDataEncoded = base64Encode(bytecode.vsBytecode.data(), bytecode.vsBytecode.size() * sizeof(bytecode.vsBytecode[0]));
		std::string psDataEncoded = base64Encode(bytecode.psBytecode.data(), bytecode.psBytecode.size() * sizeof(bytecode.psBytecode[0]));

		jShadeProgByteCode->setMember("vsDataEncoded", jvb(vsDataEncoded));
		jShadeProgByteCode->setMember("psDataEncoded", jvb(psDataEncoded));
	}

	// Write the created json to a file.
	JsonWriter jw;
	bool succeeded = jw.WriteInFile(cacheFilename, jRoot, true);
	return succeeded;
}

bool ShadingProgramPermuatorCache::loadFileFile(const char* cacheFilename)
{
	try {
		// Reset the structure to its default state.
		*this = ShadingProgramPermuatorCache();

		if (isStringEmpty(cacheFilename)) {
			return false;
		}

		FileReadStream frs;
		if (frs.open(cacheFilename) == false) {
			return false;
		}

		JsonParser jp;
		if (jp.parse(&frs) == false) {
			sgeAssert(false && "The shader cache file seems to be broken.");
			return false;
		}

		const JsonValue* jRoot = jp.getRoot();

		// Load the file hashes.
		const JsonValue& jSrcFiles = jRoot->getMemberOrThrow("sourceFiles");
		for (int iFilePair = 0; iFilePair < jSrcFiles.arrSize(); ++iFilePair) {
			const JsonValue* jFileHashPair = jSrcFiles.arrAt(iFilePair);
			std::string file = jFileHashPair->getMemberOrThrow("file").GetStringOrThrow();
			unsigned hash = (unsigned)jFileHashPair->getMemberOrThrow("hash").getNumberAs<int>();

			this->sourceFileContentsHash[file] = hash;
		}

		// Load the shaders bytecode.
		const JsonValue& jPermutationsBytecodes = jRoot->getMemberOrThrow("shaderBytecodesPerPermutation");
		for (int iPerm = 0; iPerm < jPermutationsBytecodes.arrSize(); ++iPerm) {
			const JsonValue* jShadeProgByteCode = jPermutationsBytecodes.arrAt(iPerm);
			std::string vsBytecodeEncoded = jShadeProgByteCode->getMemberOrThrow("vsDataEncoded").GetStringOrThrow();
			std::string psBytecodeEncoded = jShadeProgByteCode->getMemberOrThrow("psDataEncoded").GetStringOrThrow();

			std::vector<char> vsDecoded;
			base64Decode(vsBytecodeEncoded.data(), vsBytecodeEncoded.size(), vsDecoded);

			std::vector<char> psDecoded;
			base64Decode(psBytecodeEncoded.data(), psBytecodeEncoded.size(), psDecoded);

			perPermutBytecode.emplace_back(ShadingProgramByteCode{std::move(vsDecoded), std::move(psDecoded)});
		}

		return true;
	}
	catch (...) {
		sgeAssertFalse("The cache file seems to be broken");
		*this = ShadingProgramPermuatorCache();
		return false;
	}
}



bool ShadingProgramPermuatorCache::verifyThatCacheIsUpDoDate() const
{
	std::vector<char> fileData;
	for (const auto& fileNameHashPair : sourceFileContentsHash) {
		fileData.clear();
		FileReadStream::readFile(fileNameHashPair.first.c_str(), fileData);
		unsigned realDataHash = hash_djb2(fileData.data(), fileData.size());

		if (realDataHash != fileNameHashPair.second) {
			return false;
		}
	}

	return true;
}


} // namespace sge
