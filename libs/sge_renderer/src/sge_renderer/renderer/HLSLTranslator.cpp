#include "HLSLTranslator.h"
#include "sge_codepreproc/sge_codepreproc.h"
#include "sge_log/Log.h"
#include "sge_utils/io/FileStream.h"
#include "sge_utils/text/format.h"
#include <Engine.h>
#include <GLSLGenerator.h>
#include <HLSLGenerator.h>
#include <HLSLParser.h>
#include <cstring>
#include <mutex>
#include <unordered_map>

namespace sge {

bool translateHLSL(
    const char* const pCode,
    const ShadingLanguage::Enum shadingLanguage,
    const ShaderType::Enum shaderType,
    std::string& result,
    std::string& compilationErrors,
    std::set<std::string>* outIncludedFiles)
{
	M4::g_hlslParserErrors.clear();

	const char* const mOpenGL = "OpenGL";

	std::vector<const char*> macros;

	if (shadingLanguage == ShadingLanguage::GLSL) {
		macros.push_back(mOpenGL);
	}

	if (shaderType == ShaderType::PixelShader) {
		macros.push_back("SGE_PIXEL_SHADER");
	}


	const std::string processsedCode =
	    preprocessCode(pCode, "<NO-FILE>", macros.data(), int(macros.size()), "core_shaders/", outIncludedFiles);

	M4::Allocator m4Alloc;
	M4::HLSLParser hlslParser(&m4Alloc, "<NO-FILE>", processsedCode.c_str(), processsedCode.size());
	M4::HLSLTree tree(&m4Alloc);

	if (hlslParser.Parse(&tree) == false) {
		compilationErrors = M4::g_hlslParserErrors;
		return false;
	}

	if (shadingLanguage == ShadingLanguage::GLSL) {
		auto target = shaderType == ShaderType::VertexShader ? M4::GLSLGenerator::Target_VertexShader
		                                                     : M4::GLSLGenerator::Target_FragmentShader;
		auto mainFnName = shaderType == ShaderType::VertexShader ? "vsMain" : "psMain";

		M4::GLSLGenerator gen;
#if defined(__EMSCRIPTEN__)
		if (gen.Generate(&tree, target, M4::GLSLGenerator::Version_300_ES, mainFnName) == false) {
#else
		if (gen.Generate(&tree, target, M4::GLSLGenerator::Version_150, mainFnName) == false) {
#endif
			compilationErrors = M4::g_hlslParserErrors;
			return false;
		}
		result = gen.GetResult();
	}
	else if (shadingLanguage == ShadingLanguage::HLSL) {
		auto target = shaderType == ShaderType::VertexShader ? M4::HLSLGenerator::Target_VertexShader
		                                                     : M4::HLSLGenerator::Target_PixelShader;
		auto mainFnName = shaderType == ShaderType::VertexShader ? "vsMain" : "psMain";

		M4::HLSLGenerator gen;
		if (gen.Generate(&tree, target, mainFnName, false) == false) {
			compilationErrors = M4::g_hlslParserErrors;
			return false;
		}
		result = gen.GetResult();
	}

	return true;
}

} // namespace sge
