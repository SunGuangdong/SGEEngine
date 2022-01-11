#pragma once
#include "sge_engine/sge_engine_api.h"
#include <string>
#include <vector>

namespace sge {

/// SceneInstanceSerializedData holds information about each opened scene in the editor window.
/// When we do hot reaload of the game plugin, we need to destroy all the levels
/// (as they contain pointer to the DLL of the game witch is going to get unloaded)
/// save them, and once the plugin has been relaoaded load them once again.
struct SGE_ENGINE_API SceneInstanceSerializedData {
	// The directory that will contain the files that describe the state of the editor.
	// These files are also used as a backup.
	// If the speicified directory path does not exists, it will get created.
	void saveInDirectory(const std::string& dirPath) const;

	bool loadFromDirectory(const std::string& dirPath);

  public:
	struct PerInstanceSerializedData {
		std::string filename;
		std::string displayName;
		std::string levelJson;
	};

	std::vector<PerInstanceSerializedData> instances;
	int iActiveInstance = -1;
};

} // namespace sge
