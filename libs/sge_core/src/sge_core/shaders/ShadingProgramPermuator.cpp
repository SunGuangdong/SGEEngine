#include "ShadingProgramPermuator.h"
#include "sge_core/ICore.h"
#include "sge_log/Log.h"
#include "sge_utils/utils/FileStream.h"
#include "sge_utils/utils/base64/base64.h"
#include "sge_utils/utils/hash_combine.h"
#include "sge_utils/utils/json.h"
#include "sge_utils/utils/strings.h"


namespace sge {

struct ShadingProgramsCacheFile {
	struct ShadingProgramByteCode {
		std::vector<char> vsBytecode;
		std::vector<char> psBytecode;
	};

  public:
	bool saveToFile(const char* cacheFilename) const {
		JsonValueBuffer jvb;

		JsonValue* jRoot = jvb(JID_MAP);
		JsonValue* jSrcFiles = jRoot->setMember("sourceFiles", jvb(JID_ARRAY));

		for (const auto& fileNameHashPair : sourceFileContentsHash) {
			JsonValue* jFileHashPair = jSrcFiles->arrPush(jvb(JID_MAP));
			jFileHashPair->setMember("file", jvb(fileNameHashPair.first));
			jFileHashPair->setMember("hash", jvb(fileNameHashPair.second));
		}

		JsonValue* jPermutationsBytecodes = jRoot->setMember("shaderBytecodesPerPermutation", jvb(JID_ARRAY));
		for (const ShadingProgramByteCode& bytecode : perPermutBytecode) {
			JsonValue* jShadeProgByteCode = jPermutationsBytecodes->arrPush(jvb(JID_MAP));

			std::string vsDataEncoded =
			    base64Encode(bytecode.vsBytecode.data(), bytecode.vsBytecode.size() * sizeof(bytecode.vsBytecode[0]));
			std::string psDataEncoded =
			    base64Encode(bytecode.psBytecode.data(), bytecode.psBytecode.size() * sizeof(bytecode.psBytecode[0]));

			jShadeProgByteCode->setMember("vsDataEncoded", jvb(vsDataEncoded));
			jShadeProgByteCode->setMember("psDataEncoded", jvb(psDataEncoded));

#if 1
			std::vector<char> vsdec;
			base64Decode(vsDataEncoded.c_str(), vsDataEncoded.size(), vsdec);
			sgeAssert(bytecode.vsBytecode == vsdec);

			std::vector<char> psdec;
			base64Decode(psDataEncoded.c_str(), psDataEncoded.size(), psdec);
			sgeAssert(bytecode.psBytecode == psdec);
#endif
		}

		JsonWriter jw;
		bool succeeded = jw.WriteInFile(cacheFilename, jRoot, true);
		return succeeded;
	}

	bool verifyThatCacheIsUpDoDate() const {
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

	bool loadFileFile(const char* cacheFilename) {
		try {
			*this = ShadingProgramsCacheFile();

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
		} catch (...) {
			sgeAssertFalse("The cache file seems to be broken");
			*this = ShadingProgramsCacheFile();
			return false;
		}
	}


  public:
	std::map<std::string, unsigned> sourceFileContentsHash;
	std::vector<ShadingProgramByteCode> perPermutBytecode;
};


bool ShadingProgramPermuator::createFromFile(SGEDevice* sgedev,
                                             const char* const filename,
                                             const char* const precompiledCacheFile,
                                             const std::vector<OptionPermuataor::OptionDesc>& compileTimeOptions,
                                             const std::vector<Unform>& uniformsToCacheInLUT,
                                             std::set<std::string>* outIncludedFiles) {
	std::vector<char> fileContents;
	if (FileReadStream::readFile(filename, fileContents)) {
		if (outIncludedFiles != nullptr) {
			outIncludedFiles->insert(filename);
		}

		fileContents.push_back('\0');

		dependantFiles.insert(filename);

		return create(sgedev, fileContents.data(), precompiledCacheFile, compileTimeOptions, uniformsToCacheInLUT, outIncludedFiles);
	}

	return false;
}

bool ShadingProgramPermuator::create(SGEDevice* sgedev,
                                     const char* const shaderCode,
                                     const char* const precompiledCacheFile,
                                     const std::vector<OptionPermuataor::OptionDesc>& compileTimeOptions,
                                     const std::vector<Unform>& uniformsToCacheInLUT,
                                     std::set<std::string>* outIncludedFiles) {
	*this = ShadingProgramPermuator();

	// Verify the safety indices.
	for (int t = 0; t < uniformsToCacheInLUT.size(); ++t) {
		if (uniformsToCacheInLUT[t].safetyIndex != t) {
			sgeAssert(false);
			return false;
		}
	}

	compileTimeOptionsPermutator.build(compileTimeOptions);

	const int numPerm = int(compileTimeOptionsPermutator.getAllPermunations().size());
	if (numPerm == 0) {
		return false;
	}

	// Check if the cache is valid.
	ShadingProgramsCacheFile programCompiledCache;
	bool isCacheLoaded = false;
	if (!isStringEmpty(precompiledCacheFile)) {
		isCacheLoaded = programCompiledCache.loadFileFile(precompiledCacheFile);
	}

	// Create the shaders.
	perPermutationShadingProg.resize(numPerm);

	bool hadErrors = false;

	const bool isCacheLoadedAndValid = isCacheLoaded && programCompiledCache.verifyThatCacheIsUpDoDate();
	if (isCacheLoadedAndValid) {
		for (int iPerm = 0; iPerm < compileTimeOptionsPermutator.getAllPermunations().size(); ++iPerm) {
			const auto& permCache = programCompiledCache.perPermutBytecode[iPerm];


			GpuHandle<Shader> vs = sgedev->requestResource<Shader>();
			vs->createFromNativeBytecode(ShaderType::VertexShader, permCache.vsBytecode);

			GpuHandle<Shader> ps = sgedev->requestResource<Shader>();
			ps->createFromNativeBytecode(ShaderType::PixelShader, permCache.psBytecode);

			perPermutationShadingProg[iPerm].shadingProgram = sgedev->requestResource<ShadingProgram>();
			perPermutationShadingProg[iPerm].shadingProgram->create(vs, ps);
		}

		hadErrors = false;
	} else {
		// No cache compile for real.
		std::string shaderCodeFull;
		size_t shaderCodeSize = strlen(shaderCode);
		for (int iPerm = 0; iPerm < compileTimeOptionsPermutator.getAllPermunations().size(); ++iPerm) {
			const auto& perm = compileTimeOptionsPermutator.getAllPermunations()[iPerm];

			// Construct a string setting the values for this permutation.
			std::string macrosToPreapend;
			for (int iOpt = 0; iOpt < compileTimeOptions.size(); ++iOpt) {
				const OptionPermuataor::OptionDesc& desc = compileTimeOptions[iOpt];
				macrosToPreapend += "#define " + desc.name + " " + desc.possibleValues[perm[iOpt]] + "\n";
			}

			perPermutationShadingProg[iPerm].shadingProgram = sgedev->requestResource<ShadingProgram>();

			shaderCodeFull.clear();
			shaderCodeFull += macrosToPreapend;
			shaderCodeFull.append(shaderCode, shaderCodeSize);

			// Compile the program and if succeeded, grab a reflection and cache the requested uniform locations.
			const CreateShaderResult programCreateResult = perPermutationShadingProg[iPerm].shadingProgram->createFromCustomHLSL(
			    shaderCodeFull.c_str(), shaderCodeFull.c_str(), &dependantFiles);

			if (programCreateResult.succeeded == false) {
				sgeLogError("Shader Compilation Failed:\n%s", programCreateResult.errors.c_str());
				hadErrors = true;
			}
		}
	}

	// Cache the requested uniforms locations.
	for (int iPerm = 0; iPerm < compileTimeOptionsPermutator.getAllPermunations().size(); ++iPerm) {
		const ShadingProgramRefl& refl = perPermutationShadingProg[iPerm].shadingProgram->getReflection();
		perPermutationShadingProg[iPerm].uniformLUT.reserve(uniformsToCacheInLUT.size());
		for (int t = 0; t < uniformsToCacheInLUT.size(); ++t) {
			BindLocation bindLoc = refl.findUniform(uniformsToCacheInLUT[t].uniformName, uniformsToCacheInLUT[t].shaderStage);
			perPermutationShadingProg[iPerm].uniformLUT.push_back(bindLoc);
		}
	}

	if (outIncludedFiles) {
		outIncludedFiles->insert(dependantFiles.begin(), dependantFiles.end());
	}

	if (!hadErrors && !isCacheLoadedAndValid && precompiledCacheFile != nullptr) {
		generateShadingProgramsCompilationCache(precompiledCacheFile);
	}

	return !hadErrors;
}

void ShadingProgramPermuator::generateShadingProgramsCompilationCache(const char* const precompiledCacheFile) const {
	ShadingProgramsCacheFile cacheFile;

	std::vector<char> fileData;
	for (const std::string& filename : dependantFiles) {
		fileData.clear();
		FileReadStream::readFile(filename.c_str(), fileData);
		unsigned hash = hash_djb2(fileData.data(), fileData.size());

		cacheFile.sourceFileContentsHash[filename] = hash;
	}

	std::vector<char> vsBytecode;
	std::vector<char> psBytecode;
	for (const Permutation& perm : perPermutationShadingProg) {
		vsBytecode.clear();
		perm.shadingProgram->getVertexShader()->getCreationBytecode(vsBytecode);

		psBytecode.clear();
		perm.shadingProgram->getPixelShader()->getCreationBytecode(psBytecode);

		cacheFile.perPermutBytecode.push_back({std::move(vsBytecode), std::move(psBytecode)});
	}


	cacheFile.saveToFile(precompiledCacheFile);
}

} // namespace sge
