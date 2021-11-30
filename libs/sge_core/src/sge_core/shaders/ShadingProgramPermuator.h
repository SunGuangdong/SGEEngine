#pragma once

#include "sge_core/sgecore_api.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/utils/OptionPermutator.h"
#include "sge_utils/utils/StaticArray.h"

namespace sge {


/// ShadingProgramPermuator provides a way to compile the specified file (and all of its includes)
/// to a ShadingProgram, while providing a way to specify multiple compile time options.
/// The permutatior will generate all possible combinations of these compile time options as you can request
/// any combination of compile time option and their values and get a ShadingProgram for it.
/// For example if you have shader that supports both skinned and non-skinned geometry. When the geometry
/// is skinned each vertex usually has a an attributes for bones ids and weights, when there is no skinning
/// these attributes should not be defined as the vertex buffer does not have values for them and the input assembled would fail.
/// Or another example could be a shading program that does less computations on mobile or other weaker devices.
struct SGE_CORE_API ShadingProgramPermuator {
	struct Unform {
		int safetyIndex; ///< The user specified to the ShadingProgramPermuator an array of uniforms to be found. In order to decrease
		                 ///< debugging we want this index to be the same as the index in the array specified by the user, as they are going
		                 ///< to access them by that index.
		const char* uniformName;
		ShaderType::Enum shaderStage;
	};

	struct Permutation {
		GpuHandle<ShadingProgram> shadingProgram;
		std::vector<BindLocation> uniformLUT;

		/// Using the cached bind locations for each uniforms, binds the input data to the specified uniform
		/// in in the @uniforms.
		template <size_t N>
		void bind(StaticArray<BoundUniform, N>& uniforms, const int uniformEnumId, void* const dataPointer) const {
			if (uniformLUT[uniformEnumId].isNull() == false) {
				[[maybe_unused]] bool bindSucceeded = uniforms.push_back(BoundUniform(uniformLUT[uniformEnumId], (dataPointer)));
				sgeAssert(bindSucceeded);
				sgeAssert(uniforms.back().bindLocation.isNull() == false && uniforms.back().bindLocation.uniformType != 0);
			};
		}
	};

  public:
	bool createFromFile(SGEDevice* sgedev,
	                    const char* const fileName,
	                    const std::vector<OptionPermuataor::OptionDesc>& compileTimeOptions,
	                    const std::vector<Unform>& uniformsToCacheInLUT,
	                    std::set<std::string>* outIncludedFiles = nullptr);

	bool create(SGEDevice* sgedev,
	            const char* const shaderCode,
	            const std::vector<OptionPermuataor::OptionDesc>& compileTimeOptions,
	            const std::vector<Unform>& uniformsToCacheInLUT,
	            std::set<std::string>* outIncludedFiles = nullptr);


	const OptionPermuataor getCompileTimeOptionsPerm() const {
		return compileTimeOptionsPermutator;
	}
	const std::vector<Permutation>& getShadersPerPerm() const {
		return perPermutationShadingProg;
	}

  private:
	OptionPermuataor compileTimeOptionsPermutator;
	std::vector<Permutation> perPermutationShadingProg;
};

} // namespace sge
