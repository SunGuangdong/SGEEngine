#pragma once

#include <map>
#include <string>
#include <vector>

namespace sge {

/// Used internally by @ShadingProgramPermuator, this structure provides
/// A way to cache the ShadingProgram compiled bytecode as (at least on D3D)
/// the shader compilation could be quite slow. Later between game/editor launches
/// if the shader code hasn't been changed the ShadingProgramPermuator can
/// reuse the compiled bytecode, bypassing the slow compilation.
/// If the source file of the shader has been changed, the @verifyThatCacheIsUpDoDate method
/// would return false, meaning that we should compile the shaders again.
struct ShadingProgramPermuatorCache {
	/// Saves the cache to the specified file.
	bool saveToFile(const char* cacheFilename) const;

	/// Loads the cache from the specified file.
	bool loadFileFile(const char* cacheFilename);

	/// After loading a cache with @loadFileFile, we can use
	/// this function to see if any the source files used by the shader
	/// has been changed.
	bool verifyThatCacheIsUpDoDate() const;

  public:
	struct ShadingProgramByteCode {
		std::vector<char> vsBytecode;
		std::vector<char> psBytecode;
	};

	std::map<std::string, unsigned> sourceFileContentsHash;
	std::vector<ShadingProgramByteCode> perPermutBytecode;
};

} // namespace sge
