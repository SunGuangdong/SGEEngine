#include "InGameMenu.h"

#include "sge_core/GameUI/Widget.h"
#include "sge_core/ICore.h"
#include "sge_core/QuickDraw.h"
#include "sge_engine/GameDrawer/GameDrawer.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/IWorldScript.h"
#include "sge_engine/ScriptObject.h"
#include "sge_utils/utils/strings.h"

#include "../GlobalRandom.h"

#include "../AWitch.h"

namespace sge {
using namespace gamegui;
using namespace gamegui::literals;

RelfAddTypeId(InGameMenuScript, 21'08'26'0002);
ReflBlock() {
	ReflAddScript(InGameMenuScript);
}


const char* g_deadMessages[] = {
    "With that much candy you cannot even make a cavity!",
    "Doing nothing is better than wasting time!",
};

void InGameMenuScript::create() {
	context.create(getWorld()->userProjectionSettings.canvasSize);
	font.Create(getCore()->getDevice(), "assets/editor/fonts/Festive-Regular.ttf", 128);
	fontMenus.Create(getCore()->getDevice(), "assets/editor/fonts/AutourOne-Regular.ttf", 128);
	context.setDefaultFont(&fontMenus);

	auto rootMenuWidget = std::make_shared<gamegui::InvisibleWidget>(context, Pos(0_f, 0_f), Size(1_f, 1_f));
	context.addRootWidget(rootMenuWidget);

	{
		hud = rootMenuWidget->addChildT(std::make_shared<gamegui::InvisibleWidget>(context, Pos(0_f, 0_f), Size(1_f, 1_f)));

		auto imgWitchProfile = getCore()->getAssetLib()->getAsset("assets/witchProfile.png", true);
		auto imgCandies = getCore()->getAssetLib()->getAsset("assets/candy icon.png", true);

		auto imgWidget = hud->addChildT(ImageWidget::create(context, Pos(0_f, 0_f), 0.15_wf, imgWitchProfile->asTextureView()->tex));

		livesTextWdg =
		    hud->addChildT(std::make_shared<gamegui::TextWidget>(context, Pos(0.15_wf, 0_f, vec2f(0.f, 0.f)), Size(0.5_wf, 0.15_wf)));

		livesTextWdg->setFont(&font);
		livesTextWdg->setFontSize(1_f);
		livesTextWdg->setColor(colorFromIntRgba(211, 255, 252));



		livesTextWdg->m_algnTextHCenter = false;
		livesTextWdg->m_algnTextVCenter = true;


		auto imgCandiesWidget =
		    hud->addChildT(ImageWidget::create(context, Pos(1_f, 0_hf, vec2f(1.f, 0.f)), 0.15_wf, imgCandies->asTextureView()->tex));


		cnadiesTextWdg =
		    hud->addChildT(std::make_shared<gamegui::TextWidget>(context, Pos(0.85_wf, 0.0_f, vec2f(1.f, 0.f)), Size(0.15_wf, 0.15_wf)));
		cnadiesTextWdg->setText("0");
		cnadiesTextWdg->setFont(&font);

		cnadiesTextWdg->setColor(colorFromIntRgba(211, 255, 252));
	}

	{
		deadScreen = rootMenuWidget->addChildT(std::make_shared<gamegui::InvisibleWidget>(context, Pos(0_f, 0_f), Size(1_f, 1_f)));


		auto deadScreenBlack = deadScreen->addChildT(std::make_shared<gamegui::ColoredWidget>(context, Pos(0_f, 0_f), Size(1_f, 1_f)));
		deadScreenBlack->setColor(vec4f(0.f, 0.f, 0.f, 1.f));

		auto diedTextMsg =
		    deadScreen->addChildT(std::make_shared<gamegui::TextWidget>(context, Pos(0.5_f, 0.25_f, vec2f(0.5f)), Size(0.8_wf, 0.07_wf)));
		diedTextMsg->setText("You fell off the broom!");
		diedTextMsg->setColor(colorFromIntRgba(255, 158, 80));

		auto btnRestart = deadScreen->addChildT(
		    std::make_shared<gamegui::ButtonWidget>(context, Pos(0.5_f, 0.40_f, vec2f(0.5f, 0.f)), Size(0.45_wf, 0.10_wf)));
		btnRestart->setText("Fly Again!");
		btnRestart->setBgColor(colorWhite(0.33f), colorBlack(0.44f), colorBlack(0.22f));
		btnRestart->setColor(colorFromIntRgba(255, 158, 80));

		deadScreenCandiesCount =
		    deadScreen->addChildT(std::make_shared<gamegui::TextWidget>(context, Pos(0.5_f, 0.75_f, vec2f(0.5f)), Size(0.7_wf, 0.05_wf)));
		deadScreenCandiesCount->setColor(colorFromIntRgba(255, 158, 80));

		eventSubs.push_back(btnRestart->subscribe_onRelease(
		    [&] { getWorld()->addPostSceneTask(new PostSceneUpdateTaskLoadWorldFormFile("assets/levels/game.lvl", true)); }));

		deadScreen->suspend();
	}
	
	{
		blackScreen = rootMenuWidget->addChildT(std::make_shared<gamegui::ColoredWidget>(context, Pos(0_f, 0_f), Size(1_f, 1_f)));
		blackScreen->setColor(colorFromIntRgba(16, 8, 1));
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
		} else {
			cnadiesTextWdg->setText(string_format("%d", int(w->numCandiesCollected)));
			deadScreenCandiesCount->setText(string_format("You've collected %d candies!", int(w->numCandiesCollected)));
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
