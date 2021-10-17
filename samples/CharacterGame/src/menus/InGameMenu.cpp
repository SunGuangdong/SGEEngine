#include "InGameMenu.h"

#include "sge_core/GameUI/Widget.h"
#include "sge_core/ICore.h"
#include "sge_core/QuickDraw.h"
#include "sge_engine/GameDrawer/GameDrawer.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/IWorldScript.h"
#include "sge_engine/ScriptObject.h"
#include "sge_utils/utils/strings.h"

#include "../AWitch.h"

namespace sge {
using namespace gamegui;
using namespace gamegui::literals;

RelfAddTypeId(InGameMenuScript, 21'08'26'0002);
ReflBlock() {
	ReflAddScript(InGameMenuScript);
}

void InGameMenuScript::create() {
	context.create(getWorld()->userProjectionSettings.canvasSize);
	font.Create(getCore()->getDevice(), "assets/editor/fonts/AutourOne-Regular.ttf", 128);
	context.setDefaultFont(&font);

	auto rootMenuWidget = std::make_shared<gamegui::InvisibleWidget>(context, Pos(0_f, 0_f), Size(1_f, 1_f));
	context.addRootWidget(rootMenuWidget);

	{
		hud = rootMenuWidget->addChildT(std::make_shared<gamegui::InvisibleWidget>(context, Pos(0_f, 0_f), Size(1_f, 1_f)));

		auto livesWdg = hud->addChildT(std::make_shared<gamegui::InvisibleWidget>(context, Pos(0_f, 0_f), Size(1_f, 0.15_hf)));

		auto imgWitchProfile = getCore()->getAssetLib()->getAsset("assets/witchProfile.png", true);
		auto imgWidget =
		    livesWdg->addChildT(ImageWidget::createByHeight(context, Pos(0_f, 0_f), 1_hf, imgWitchProfile->asTextureView()->tex));
		livesTextWdg = livesWdg->addChildT(
		    std::make_shared<gamegui::TextWidget>(context, Pos(imgWidget->getSize().sizeX, 0_f, vec2f(0.f, 0.f)), Size(1_wf, 1_hf)));

		livesTextWdg->setText("3");
		livesTextWdg->m_algnTextHCenter = false;
		livesTextWdg->m_algnTextVCenter = true;
	}

	{
		deadScreen = rootMenuWidget->addChildT(std::make_shared<gamegui::InvisibleWidget>(context, Pos(0_f, 0_f), Size(1_f, 1_f)));
		

		auto deadScreenBlack = deadScreen->addChildT(std::make_shared<gamegui::ColoredWidget>(context, Pos(0_f, 0_f), Size(1_f, 1_f)));
		deadScreenBlack->setColor(vec4f(0.f, 0.f, 0.f, 1.f));

		auto diedTextMsg = deadScreen->addChildT(std::make_shared<gamegui::TextWidget>(context, Pos(0_f, 0.25_f), Size(1_f, 0.07_f)));
		diedTextMsg->setText("You've falled off the broom!");

		auto btnRestart = deadScreen->addChildT(
		    std::make_shared<gamegui::ButtonWidget>(context, Pos(0.5_f, 0.40_f, vec2f(0.5f, 0.f)), Size(0.35_wf, 0.10_wf)));
		btnRestart->setText("Fly Again!");

		eventSubs.push_back(btnRestart->subscribe_onRelease(
		    [&] { getWorld()->addPostSceneTask(new PostSceneUpdateTaskLoadWorldFormFile("assets/levels/game.lvl", true)); }));

		deadScreen->suspend();
	}

	{
		blackScreen = rootMenuWidget->addChildT(std::make_shared<gamegui::ColoredWidget>(context, Pos(0_f, 0_f), Size(1_f, 1_f)));
		blackScreen->setColor(vec4f(0.f, 0.f, 0.f, 1.f));
	}
}

void InGameMenuScript::onPostUpdate(const GameUpdateSets& u) {
	blackScreen->m_color.w = 1.f - getWorld()->getGameTime() / 3.f;
	blackScreen->m_color.w = maxOf(blackScreen->m_color.w, 0.f);

	context.update(u.is, getWorld()->userProjectionSettings.canvasSize, u.dt);

	if (const auto& objs = getWorld()->getObjects(sgeTypeId(AWitch)); objs && !objs->empty()) {
		AWitch* w = (AWitch*)objs->at(0);
		livesTextWdg->setText(string_format("%d", w->getHealth()));

		if (w->isDead()) {
			hud->suspend();
			blackScreen->suspend();
			deadScreen->unsuspend();
		}
	}
}

void InGameMenuScript::onPostDraw(const GameDrawSets& drawSets) {
	if (dynamic_cast<EditorCamera*>(drawSets.gameCamera) == nullptr) {
		gamegui::UIDrawSets drawSetsUI;
		drawSetsUI.setup(drawSets.rdest, drawSets.quickDraw);

		context.draw(drawSetsUI);
	}
}

} // namespace sge
