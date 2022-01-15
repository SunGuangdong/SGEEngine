#include "ShadingProgramPermuator.h"
#include "ShadingProgramPermuatorCache.h"
#include "sge_core/ICore.h"
#include "sge_log/Log.h"
#include "sge_utils/utils/FileStream.h"
#include "sge_utils/utils/hash_combine.h"
#include "sge_utils/utils/strings.h"

namespace sge {

bool ShadingProgramPermuator::createFromFile(SGEDevice* sgedev,
                                             const char* const filename,
                                             const char* const precompiledCacheFile,
                                             const std::vector<OptionPermuataor::OptionDesc>& compileTimeOptions,
                                             const std::vector<Unform>& uniformsToCacheInLUT,
                                             std::set<std::string>* outIncludedFiles) {
	*this = ShadingProgramPermuator();

	std::string fileContents;
	if (FileReadStream::readTextFile(filename, fileContents)) {
		if (outIncludedFiles != nullptr) {
			outIncludedFiles->insert(filename);
		}

		dependantFilesForShaderCachemaking.insert(filename);

		return createInternal(sgedev, fileContents.data(), filename, precompiledCacheFile, compileTimeOptions, uniformsToCacheInLUT,
		              outIncludedFiles);
	}

	return false;
}

bool ShadingProgramPermuator::create(SGEDevice* sgedev,
                                     const char* const shaderCode,
                                     const char* const shaderCodeFileName,
                                     const char* const precompiledCacheFile,
                                     const std::vector<OptionPermuataor::OptionDesc>& compileTimeOptions,
                                     const std::vector<Unform>& uniformsToCacheInLUT,
                                     std::set<std::string>* outIncludedFiles) {
	*this = ShadingProgramPermuator();

	return createInternal(sgedev, shaderCode, shaderCodeFileName, precompiledCacheFile, compileTimeOptions, uniformsToCacheInLUT,
	                      outIncludedFiles);
}

bool ShadingProgramPermuator::createInternal(SGEDevice* sgedev,
                                             const char* const shaderCode,
                                             const char* const shaderCodeFileName,
                                             const char* const precompiledCacheFile,
                                             const std::vector<OptionPermuataor::OptionDesc>& compileTimeOptions,
                                             const std::vector<Unform>& uniformsToCacheInLUT,
                                             std::set<std::string>* outIncludedFiles) {
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
	ShadingProgramPermuatorCache programCompiledCache;
	bool isCacheLoaded = false;
	if (!isStringEmpty(precompiledCacheFile)) {
		isCacheLoaded = programCompiledCache.loadFileFile(precompiledCacheFile);
	}

	// Create the shaders.
	perPermutationShadingProg.resize(numPerm);

	bool hadErrors = false;

	const bool isCacheLoadedAndValid = isCacheLoaded && programCompiledCache.verifyThatCacheIsUpDoDate();
	if (isCacheLoadedAndValid) {
		sgeLogCheck("For %s a vaild shader cache will be used.", shaderCodeFileName);

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
		sgeLogWarn("For %s no valid cache file is found. The shaders are going to be compiled.", shaderCodeFileName);

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
			    shaderCodeFull.c_str(), shaderCodeFull.c_str(), &dependantFilesForShaderCachemaking);

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
		outIncludedFiles->insert(dependantFilesForShaderCachemaking.begin(), dependantFilesForShaderCachemaking.end());
	}

	if (!hadErrors && !isCacheLoadedAndValid && precompiledCacheFile != nullptr) {
		generateShadingProgramsCompilationCache(precompiledCacheFile);
	}

	return !hadErrors;
}

void ShadingProgramPermuator::generateShadingProgramsCompilationCache(const char* const precompiledCacheFile) const {
	ShadingProgramPermuatorCache cacheFile;

	std::vector<char> fileData;
	for (const std::string& filename : dependantFilesForShaderCachemaking) {
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
