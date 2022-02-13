#pragma once

#include "IImGuiWindow.h"
#include <string>

namespace sge {

struct InputState;
struct GameInspector;
struct GameObject;
struct MemberChain;

enum AssetIfaceType : int;

/// A set of functions that generate the User Interface for the specified member of game object.
/// The functions could be called anywhere.
namespace ProperyEditorUIGen {
	/// Does the user interface specified game object.
	SGE_ENGINE_API void doGameObjectUI(GameInspector& inspector, GameObject* const gameObject);

	enum DoMemberUIFlags : int {
		DoMemberUIFlags_none = 0,
		DoMemberUIFlags_structInNoHeader = 1 << 0,

	};

	/// Does the user interface for the speicfied member of the @gameObject.
	/// Useful if we want to have custom UI for some type,
	/// but we still want to use the auto generated UI for its members.
	/// @param [in, out] gameObject is the owner of the member that we are doing UI for.
	/// @param [in] chain points to the member (starting form @gameObject) that we are going to generate  UI for.
	/// @param [in] flags a bitmask made from @DoMemberUIFlags
	SGE_ENGINE_API void doMemberUI(GameInspector& inspector, GameObject* const gameObject, MemberChain chain, int flags = 0);

	SGE_ENGINE_API void editFloat(GameInspector& inspector, const char* label, GameObject* gameObject, MemberChain chain);
	SGE_ENGINE_API void editInt(GameInspector& inspector, const char* label, GameObject* gameObject, MemberChain chain);
	SGE_ENGINE_API void editString(GameInspector& inspector, const char* label, GameObject* gameObject, MemberChain chain);
	SGE_ENGINE_API void editStringAsAssetPath(GameInspector& inspector,
	                                          const char* label,
	                                          GameObject* gameObject,
	                                          MemberChain chain,
	                                          const AssetIfaceType possibleAssetIfaceTypes[],
	                                          const int numPossibleAssetIfaceTypes);

	SGE_ENGINE_API void editDynamicProperties(GameInspector& inspector, GameObject* gameObject, MemberChain chain);
} // namespace ProperyEditorUIGen


/// PropertyEditorWindow enables the user to edit values on game objects.
/// These edits generate undo/redo commands (command history).
struct SGE_ENGINE_API PropertyEditorWindow : public IImGuiWindow {
	PropertyEditorWindow(std::string windowName)
	    : m_windowName(std::move(windowName))
	{
	}

	void close() override { m_isOpened = false; }

	bool isClosed() override { return !m_isOpened; }

	const char* getWindowName() const override { return m_windowName.c_str(); }

	void update(SGEContext* const sgecon, struct GameInspector* inspector, const InputState& is) override;

  private:
	bool m_isOpened = true;
	GameInspector* m_inspector;
	std::string m_windowName;

	/// The filter string for the search box.
	char m_outlinerFilter[512] = {'*', '\0'};

	bool m_showLogicTransformInLocalSpace = false;
};


} // namespace sge
