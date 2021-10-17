#include "sge_core/GameUI/UIContext.h"
#include "sge_core/GameUI/Widget.h"
#include "sge_core/ICore.h"
#include "sge_core/QuickDraw.h"
#include "sge_engine/GameDrawer/GameDrawer.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/IWorldScript.h"
#include "sge_engine/ScriptObject.h"

namespace sge {

using namespace gamegui;
using namespace gamegui::literals;

struct MainMenuScript : public IWorldScript {
	gamegui::UIContext context;

	std::vector<EventSubscription> eventSubs;

	// std::shared_ptr<gamegui::IWidget> mainMenu;
	DebugFont font;

	void create() override {
		context.create(getWorld()->userProjectionSettings.canvasSize);

		font.Create(getCore()->getDevice(), "assets/editor/fonts/AutourOne-Regular.ttf", 128);

		context.setDefaultFont(&font);

		auto rootMenuWidget = std::make_shared<gamegui::InvisibleWidget>(context, Pos(0_f, 0_f), Size(1_f, 1_f));

		auto mainMenuWidget = std::make_shared<gamegui::InvisibleWidget>(context, Pos(0.5_f, 0.5_f, vec2f(0.5f)), Size(0.85_hf, 0.35_hf));

		std::shared_ptr<gamegui::ButtonWidget> newGameBtn = ButtonWidget::create(context, Pos(0_f, 0_f), Size(1_wf, 0.5_hf), "Play");
		eventSubs.push_back(newGameBtn->subscribe_onRelease(
		    [&] { getWorld()->addPostSceneTask(new PostSceneUpdateTaskLoadWorldFormFile("assets/levels/game.lvl", true)); }));
		mainMenuWidget->addChild(newGameBtn);

		std::shared_ptr<gamegui::ButtonWidget> quitBtn = ButtonWidget::create(context, Pos(0_f, 0.5_hf), Size(1_wf, 0.5_hf), "Quit");
		mainMenuWidget->addChild(quitBtn);

		rootMenuWidget->addChild(mainMenuWidget);


		context.addRootWidget(rootMenuWidget);
	}


	// virtual void onPreUpdate(const GameUpdateSets& u) {}

	virtual void onPostUpdate(const GameUpdateSets& u) {
		if (u.isGamePaused() == false) {
			context.update(u.is, getWorld()->userProjectionSettings.canvasSize, u.dt);
		}
	}

	virtual void onPostDraw(const GameDrawSets& drawSets) {
		if (dynamic_cast<EditorCamera*>(drawSets.gameCamera) == nullptr) {
			gamegui::UIDrawSets drawSetsUI;
			drawSetsUI.setup(drawSets.rdest, drawSets.quickDraw);

			context.draw(drawSetsUI);
		}
	}
};
RelfAddTypeId(MainMenuScript, 21'08'26'0001);
ReflBlock() {
	ReflAddScript(MainMenuScript);
}

} // namespace sge
