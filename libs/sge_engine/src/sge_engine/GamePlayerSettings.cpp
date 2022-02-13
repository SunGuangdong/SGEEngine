#include "GamePlayerSettings.h"
#include "sge_utils/io/FileStream.h"
#include "sge_utils/json/json.h"
#include "sge_utils/text/Path.h"

namespace sge {

bool GamePlayerSettings::loadFromJsonFile(const char* filename)
{
	*this = GamePlayerSettings();

	FileReadStream frs;
	if (frs.open(filename) == 0) {
		return false;
	}

	JsonParser jp;
	if (!jp.parse(&frs)) {
		return false;
	}

	const JsonValue* const jRoot = jp.getRoot();

	const JsonValue* const jWndWidth = jRoot->getMember("window_width");
	const JsonValue* const jWndHeight = jRoot->getMember("window_height");
	const JsonValue* const jWndIsResizable = jRoot->getMember("window_is_resizable");
	const JsonValue* const jInitalLevel = jRoot->getMember("initial_level");

	if (jInitalLevel == nullptr || !jInitalLevel->isString()) {
		return false;
	}

	initalLevel = jInitalLevel->GetString();

	if (jWndWidth) {
		windowWidth = jWndWidth->getNumberAs<int>();
	}

	if (jWndHeight) {
		windowHeight = jWndHeight->getNumberAs<int>();
	}

	if (jWndIsResizable) {
		windowIsResizable = jWndIsResizable->getAsBool();
	}

	return true;
}

bool GamePlayerSettings::saveToJsonFile(const char* filename) const
{
	if (filename == nullptr) {
		return false;
	}

	JsonValueBuffer jvb;

	JsonValue* const jRoot = jvb(JID_MAP);
	jRoot->setMember("window_width", jvb(windowWidth));
	jRoot->setMember("window_height", jvb(windowHeight));
	jRoot->setMember("window_is_resizable", jvb(windowIsResizable));
	jRoot->setMember("initial_level", jvb(initalLevel));

	JsonWriter jw;
	bool succeeded = jw.WriteInFile(filename, jRoot, true);
	return succeeded;
}

} // namespace sge
