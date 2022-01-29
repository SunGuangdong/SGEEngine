#include "GameExport.h"
#include "sge_utils/text/Path.h"
#include <filesystem>

namespace sge {
void exportGame(const std::string& exportDir) {
	if (exportDir.empty()) {
		return;
	}

#define SGE_TRY_CATCH(code) \
	try {                   \
		code;               \
	} catch (...) {         \
	};

	const auto copyDirRecOverwrite = std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive;
	const auto copyOverwrite = std::filesystem::copy_options::overwrite_existing;

#ifdef WIN32
#define SGE_EXE_SUFFIX ".exe"
#define SGE_DLL_SUFFIX ".dll"
#define SGE_DLL_PREFIX
#else
#define SGE_EXE_SUFFIX
#define SGE_DLL_SUFFIX ".so"
#define SGE_DLL_PREFIX "lib"
#endif

	SGE_TRY_CATCH(std::filesystem::copy("appdata", exportDir + "/appData", copyDirRecOverwrite));
	SGE_TRY_CATCH(std::filesystem::copy("assets", exportDir + "/assets", copyDirRecOverwrite));
	SGE_TRY_CATCH(std::filesystem::copy(SGE_DLL_PREFIX "core_shaders", exportDir + "/core_shaders", copyOverwrite));
	SGE_TRY_CATCH(
	    std::filesystem::copy(SGE_DLL_PREFIX "SDL2d" SGE_DLL_SUFFIX, exportDir + "/" SGE_DLL_PREFIX "SDL2d" SGE_DLL_SUFFIX, copyOverwrite));
	SGE_TRY_CATCH(
	    std::filesystem::copy(SGE_DLL_PREFIX "SDL2" SGE_DLL_SUFFIX, exportDir + "/" SGE_DLL_PREFIX "SDL2" SGE_DLL_SUFFIX, copyOverwrite));
	SGE_TRY_CATCH(std::filesystem::copy(SGE_DLL_PREFIX "sge_core" SGE_DLL_SUFFIX, exportDir + "/" SGE_DLL_PREFIX "sge_core" SGE_DLL_SUFFIX,
	                                    copyOverwrite));
	SGE_TRY_CATCH(std::filesystem::copy(SGE_DLL_PREFIX "sge_engine" SGE_DLL_SUFFIX,
	                                    exportDir + "/" SGE_DLL_PREFIX "sge_engine" SGE_DLL_SUFFIX, copyOverwrite));
	SGE_TRY_CATCH(std::filesystem::copy(SGE_DLL_PREFIX "sge_player" SGE_EXE_SUFFIX,
	                                    exportDir + "/" SGE_DLL_PREFIX "sge_player" SGE_EXE_SUFFIX, copyOverwrite));

	std::string pluginName;
	for (auto const& entry : std::filesystem::directory_iterator("./")) {
		if (std::filesystem::is_regular_file(entry) && entry.path().extension() == ".gll") {
			pluginName = entry.path().string();
		}
	}

	if (pluginName.empty() == false) {
		SGE_TRY_CATCH(std::filesystem::copy(pluginName, exportDir + pluginName));
	}
}

} // namespace sge
