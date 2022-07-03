#pragma once

#include <unordered_map>
#include <vector>
#include <string>

#include "sge_core/shaders/ShadingProgramPermuator.h"
#include "sge_renderer/renderer/renderer.h"

namespace sge {

struct GenericGeomLitShader {
	bool createWithCodeFromFile(const char* codeFilePath);

  private:
	enum : int {
		OPT_HasVertexColor,
		OPT_HasUV,
		OPT_HasTangentSpace,
		OPT_HasVertexSkinning,
		kNumOptions,
	};

	enum : int {
		uTexSkinningBones, // TODO: bind the sampler.
		uParamsCbFWDDefaultShading_vertex,
		uParamsCbFWDDefaultShading_pixel,
	};

	struct MtlCBufferExtraNumericUniforms {
		UniformType::Enum uniformType = UniformType::Unknown;
		int byteOffset = -1;
	};

  public:
	// The values inside "MtlCBuffer" constant buffer.
	// Assumes that all permutations have the same meory layout.
	std::unordered_map<std::string, MtlCBufferExtraNumericUniforms> extraNumericUniforms;
	std::unordered_map<std::string, std::vector<BindLocation>> extraTextureUniformLocationPerPermatutation;

	ShadingProgramPermuator shaderPermutator;
};

} // namespace sge
