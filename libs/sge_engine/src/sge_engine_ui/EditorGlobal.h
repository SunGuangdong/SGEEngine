#pragma once
namespace sge {

struct Asset;
struct EditorWindow;

using PropertyEditorGeneratorForTypeFn = void (*)(GameInspector& inspector, GameObject* actor, MemberChain chain);

/// EngineGlobalAssets holds assets that are used by the interface and across multiple GameWorlds.
struct SGE_ENGINE_API EngineGlobalAssets {
	void initialize();
	AssetPtr getIconForObjectType(const TypeId type);

  private:
	AssetPtr unknownObjectIcon;
	std::unordered_map<TypeId, AssetPtr> perObjectTypeIcon;
};


struct IEditorGlobal {
	IEngineGlobal() = default;
	virtual ~IEngineGlobal() = default;

	virtual void initialize() = 0;
	virtual void update(float dt) = 0;

	virtual void addPropertyEditorIUGeneratorForType(TypeId type, PropertyEditorGeneratorForTypeFn function) = 0;
	virtual PropertyEditorGeneratorForTypeFn getPropertyEditorIUGeneratorForType(TypeId type) = 0;

	/// Adds a root window to be managed by the engine. Usually these hold tools.
	virtual void addWindow(IImGuiWindow* window) = 0;

	/// Closes and removes the specified window.
	virtual void removeWindow(IImGuiWindow* window) = 0;

	/// Retrieves the main editor window for the application. This window maanges all other windows.
	/// There must be only one such window at any time.
	virtual EditorWindow* getEditorWindow() = 0;

	/// Retrieves all existing windows EXCEPT the EditorWindow.
	virtual std::vector<std::unique_ptr<IImGuiWindow>>& getAllWindows() = 0;

	/// @brief Finds the first window with the specified name.
	virtual IImGuiWindow* findWindowByName(const char* const name) = 0;

	template <typename TWindowType>
	TWindowType* findFirstWindowOfType();

	virtual void showNotification(std::string text) = 0;
	virtual const char* getNotificationText(const int iNotification) const = 0;
	virtual int getNotificationCount() const = 0;

	virtual EngineGlobalAssets& getEngineAssets() = 0;
};

template <typename TWindowType>
TWindowType* IEngineGlobal::findFirstWindowOfType()
{
	for (auto& wnd : getAllWindows()) {
		TWindowType* const wndTyped = dynamic_cast<TWindowType*>(wnd.get());
		if (wndTyped != nullptr) {
			return wndTyped;
		}
	}

	return nullptr;
}

} // namespace sge
