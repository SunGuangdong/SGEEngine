#include "EditorWindowHotReloadData.h"
#include "sge_utils/utils/FileStream.h"
#include "sge_utils/utils/Path.h"
#include "sge_utils/utils/json.h"
#include "sge_utils/utils/strings.h"

namespace sge {

void SceneInstanceSerializedData::saveInDirectory(const std::string& dirPath) const {
	createDirectory(dirPath.c_str());

	JsonValueBuffer jvb;

	JsonValue* jRoot = jvb(JID_MAP);

	jRoot->setMember("activeInstanceIndex", jvb(iActiveInstance));
	JsonValue* jInstances = jRoot->setMember("instances", jvb(JID_ARRAY));

	for (int t = 0; t < (int)instances.size(); ++t) {
		const PerInstanceSerializedData& inst = instances[t];
		// Save the game world to a file.
		std::string filepathForLevel = std::string(dirPath) + string_format("/%d.lvl", t);
		FileWriteStream fws;
		fws.open(filepathForLevel.c_str());
		fws.write(inst.levelJson.data(), inst.levelJson.size());

		JsonValue* jInst = jInstances->arrPush(jvb(JID_MAP));

		jInst->setMember("filename", jvb(inst.filename));
		jInst->setMember("displayName", jvb(inst.displayName));
		jInst->setMember("levelJsonFilename", jvb(filepathForLevel));
	}

	JsonWriter jw;
	jw.WriteInFile((dirPath + "/editor_hot_realod_data.json").c_str(), jRoot, true);
}

bool SceneInstanceSerializedData::loadFromDirectory(const std::string& dirPath) {
	try {
		*this = SceneInstanceSerializedData();

		FileReadStream frs;
		if (!frs.open((dirPath + "/editor_hot_realod_data.json").c_str())) {
			return false;
		}

		JsonParser jp;
		if (!jp.parse(frs)) {
			return false;
		}

		const JsonValue* jRoot = jp.getRoot();

		iActiveInstance = jRoot->getMemberOrThrow("activeInstanceIndex").getNumberAsOrThrow<int>();

		const JsonValue& jInstances = jRoot->getMemberOrThrow("instances", JID_ARRAY);
		for (int t = 0; t < (int)jInstances.arrSize(); ++t) {
			const JsonValue* jInstance = jInstances.arrAt(t);

			PerInstanceSerializedData instance;

			instance.filename = jInstance->getMember("filename")->GetStringOrThrow();
			instance.displayName = jInstance->getMember("displayName")->GetStringOrThrow();

			std::string filepathForLevel = std::string(dirPath) + string_format("/%d.lvl", t);
			FileReadStream::readTextFile(filepathForLevel.c_str(), instance.levelJson);

			instances.emplace_back(std::move(instance));
		}
		return true;
	} catch (...) {
		return false;
	}
}

} // namespace sge
