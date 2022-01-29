#pragma once

#include "sge_core/sgecore_api.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/containers/StaticArray.h"
#include "sge_utils/other/OptionPermutator.h"

namespace sge {

/// ShadingProgramPermuator provides a way to compile the specified file (and all of its includes)
/// to a ShadingProgram, while providing a way to specify multiple compile time options(C-style via macros).
/// The permutatior will generate all possible combinations of these compile time options as you can request
/// any combination of compile time option and their values and get a ShadingProgram for it.
///
/// For example if you have shader that supports both skinned and non-skinned geometry. When the geometry
/// is skinned each vertex usually has an attributes for bone ids and weights.
/// When there is no skinning these attributes should not be defined as the vertex buffer
/// doesn't have values for them and the input would fail to get passed to the shader.
/// Or another example could be a shading program that has a macro does makes the code do less computations
/// on mobile or other weaker devices.
struct SGE_CORE_API ShadingProgramPermuator {
	struct Unform {
		/// The user specified to the ShadingProgramPermuator an array of uniforms to be found. In order to decrease
		/// debugging we want this index to be the same as the index in the array specified by the user, as they are going
		/// to access them by that index.
		int safetyIndex;
		const char* uniformName;

		/// On witch shader stage is this uniform needed.
		// If a uniform is needed on multiple stages, just add it multiple times for every different stage.
		ShaderType::Enum shaderStage;
	};

	struct Permutation {
		GpuHandle<ShadingProgram> shadingProgram;
		std::vector<BindLocation> uniformLUT;

		/// Using the cached bind locations for each uniforms, binds the input data to the specified uniform
		/// in in the @uniforms.
		template <size_t N>
		void bind(StaticArray<BoundUniform, N>& uniforms, const int uniformEnumId, void* const dataPointer) const
		{
			if (uniformLUT[uniformEnumId].isNull() == false) {
				[[maybe_unused]] bool bindSucceeded = uniforms.push_back(BoundUniform(uniformLUT[uniformEnumId], (dataPointer)));
				sgeAssert(bindSucceeded);
				sgeAssert(uniforms.back().bindLocation.isNull() == false && uniforms.back().bindLocation.uniformType != 0);
			};
		}
	};

  public:
	/// Create a shader by using the specified filename as the root code.
	/// Additional code may be #include-ed (currently force to core_shaders/ directory).
	/// if a valid @precompiledCacheFile exists the shader bytecode is going to get loaded from there
	/// avoiding the need to call the slow D3DCompile every time. If the cachefile doesn't exists a new one will be created.
	/// if you don't wanna use caching just pass nullptr for @precompiledCacheFile.
	bool createFromFile(SGEDevice* sgedev,
	                    const char* const fileName,
	                    const char* const precompiledCacheFile,
	                    const std::vector<OptionPermuataor::OptionDesc>& compileTimeOptions,
	                    const std::vector<Unform>& uniformsToCacheInLUT,
	                    std::set<std::string>* outIncludedFiles = nullptr);

	/// See the comment for @createFromFile.
	bool create(SGEDevice* sgedev,
	            const char* const shaderCode,
	            const char* const shaderCodeFileName,
	            const char* const precompiledCacheFile,
	            const std::vector<OptionPermuataor::OptionDesc>& compileTimeOptions,
	            const std::vector<Unform>& uniformsToCacheInLUT,
	            std::set<std::string>* outIncludedFiles = nullptr);


	const OptionPermuataor getCompileTimeOptionsPerm() const
	{
		return compileTimeOptionsPermutator;
	}
	const std::vector<Permutation>& getShadersPerPerm() const
	{
		return perPermutationShadingProg;
	}

  private:
	bool createInternal(SGEDevice* sgedev,
	                    const char* const shaderCode,
	                    const char* const shaderCodeFileName,
	                    const char* const precompiledCacheFile,
	                    const std::vector<OptionPermuataor::OptionDesc>& compileTimeOptions,
	                    const std::vector<Unform>& uniformsToCacheInLUT,
	                    std::set<std::string>* outIncludedFiles = nullptr);


	/// Generates the cache file for the shader compilation cache.
	void generateShadingProgramsCompilationCache(const char* const precompiledCacheFile) const;

  private:
	OptionPermuataor compileTimeOptionsPermutator;
	std::vector<Permutation> perPermutationShadingProg;

	/// The value hold valid values only we we need to actually preprocess and compile the shader code.
	/// This happends when there is no valid compilation cache file, as we need to compile the code
	/// to find these files and the whole point of the cache is to not compile it,
	/// As D3DCompile is quite slow. If it is ever needed when we have cache, we can just pre-process the code
	/// and obtain the filenames, but this currently not needed, this is why it is not done.
	std::set<std::string> dependantFilesForShaderCachemaking;
};

} // namespace sge
