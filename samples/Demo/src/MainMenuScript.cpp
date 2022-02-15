#include "sge_core/GameUI/UIContext.h"
#include "sge_core/GameUI/Widget.h"
#include "sge_core/ICore.h"
#include "sge_core/QuickDraw.h"
#include "sge_core/QuickDraw/TextRender2D.h"
#include "sge_engine/GameDrawer/GameDrawer.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/IWorldScript.h"


namespace sge {

using namespace gamegui;

struct MainMenuScript final : public IWorldScript {
	gamegui::UIContext uiContext;
	DebugFont font;
	std::shared_ptr<gamegui::InvisibleWidget> rootMenuWidget;
	std::shared_ptr<gamegui::InvisibleWidget> mainMenuButtonsWidget;
	std::vector<EventSubscription> eventSubs;

	TextRenderer textRenderer;

	void create() override
	{
		uiContext.create(getWorld()->userProjectionSettings.canvasSize);

		using namespace literals;

		rootMenuWidget = std::make_shared<gamegui::InvisibleWidget>(uiContext, Pos(0_f, 0_f), Size(1_f, 1_f));

		mainMenuButtonsWidget =
		    std::make_shared<gamegui::InvisibleWidget>(uiContext, Pos(0.5_f, 0.75_f, vec2f(0.5f)), Size(0.30_hf, 0.20_hf));
		rootMenuWidget->addChild(mainMenuButtonsWidget);

		std::shared_ptr<ButtonWidget> btn = std::make_shared<ButtonWidget>(uiContext, Pos(0_f, 0_f), Size(1_f, 0.333_f));
		btn->setText("New Game");
		eventSubs.push_back(btn->subscribe_onRelease(
		    [&] { getWorld()->addPostSceneTask(new PostSceneUpdateTaskLoadWorldFormFile("assets/levels/level0.lvl", true)); }));

		mainMenuButtonsWidget->addChild(btn);

		std::shared_ptr<ButtonWidget> btn2 = std::make_shared<ButtonWidget>(uiContext, Pos(0_f, 0.333_f), Size(1_f, 0.333_f));
		btn2->setText("Settings");
		mainMenuButtonsWidget->addChild(btn2);

		uiContext.addRootWidget(rootMenuWidget);

		textRenderer.create(getCore()->getDevice());
	}

	void onPostUpdate(const GameUpdateSets& u) override { uiContext.update(u.is, getWorld()->userProjectionSettings.canvasSize, u.dt); }

	void onPostDraw(const GameDrawSets& drawSets) override
	{
		UIDrawSets uiDrawSets;
		uiDrawSets.setup(drawSets.rdest, drawSets.quickDraw);
		uiContext.draw(uiDrawSets);


		//drawSets.quickDraw->drawTextLazy(drawSets.rdest, *uiContext.getDefaultFont(), vec2f(0.f, 128.f), vec4f(1.f), "Qwop!\nQwok!", 128.f);

		textRenderer.drawText(drawSets.rdest, mat4f::getTranslation(0.f, 128.f, 0.f), vec4f(1.f), "Settings?", *uiContext.getDefaultFont(),
		                      64.f);
	}
};

ReflAddTypeId(MainMenuScript, 21'03'13'0001);
ReflBlock()
{
	ReflAddScript(MainMenuScript);
}

} // namespace sge
