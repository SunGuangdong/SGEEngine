#include "sge_core/AssetLibrary.h"
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


float computeOpacity(float totalTime, float fadeInStartTime, float fadeInEndTime, float fadeOutStartTime, float fadeOutEndTime) {
	if (totalTime < fadeInStartTime || totalTime > fadeOutEndTime) {
		return 0.f;
	}

	if (totalTime >= fadeInStartTime && totalTime <= fadeOutStartTime) {
		float fadeTime = fadeInEndTime - fadeInStartTime;
		return clamp01((totalTime - fadeInStartTime) / fadeTime);
	}

	if (totalTime >= fadeInEndTime && totalTime <= fadeOutStartTime) {
		return 1.f;
	}
	
	if (totalTime >= fadeOutStartTime) {
		float fadeTime = fadeOutEndTime - fadeOutStartTime;
		return 1.f - clamp01((totalTime - fadeOutStartTime) / fadeTime);
	}

	return 0.f;
}

struct CinematicMenuScript : public IWorldScript {
	gamegui::UIContext uictx;

	// std::shared_ptr<gamegui::IWidget> mainMenu;
	DebugFont font;

	std::shared_ptr<Asset> statueAsset;
	std::shared_ptr<Asset> statueCrackedAsset;
	std::shared_ptr<Asset> cityAsset;

	std::shared_ptr<gamegui::InvisibleWidget> scene1Widget;
	std::shared_ptr<gamegui::ImageWidget> scene1StatueWidget;
	std::shared_ptr<gamegui::ImageWidget> scene1StatueCrackedWidget;

	void create() override {
		statueAsset = getCore()->getAssetLib()->getAsset("assets/cinematic/statue.png", true);
		statueCrackedAsset = getCore()->getAssetLib()->getAsset("assets/cinematic/statueCracked.png", true);
		cityAsset = getCore()->getAssetLib()->getAsset("assets/cinematic/city.png", true);

		uictx.create(getWorld()->userProjectionSettings.canvasSize);

		font.Create(getCore()->getDevice(), "assets/editor/fonts/UbuntuMono-Regular.ttf", 96);

		uictx.setDefaultFont(&font);

		auto rootWidget = std::make_shared<gamegui::InvisibleWidget>(uictx, Pos(0_f, 0_f), Size(1_f, 1_f));

		// Scene 1
		{
			scene1Widget = std::make_shared<gamegui::InvisibleWidget>(uictx, Pos(0_f, 0_f), Size(1_f, 1_f));
			rootWidget->addChild(scene1Widget);

			auto cityWidget = ImageWidget::create(uictx, Pos(0.5_f, 0.5_f, vec2f(0.5f)), 1.1_wf, cityAsset->asTextureView()->tex);
			scene1Widget->addChild(cityWidget);

			scene1StatueWidget = ImageWidget::createByHeight(uictx, Pos(0.5_wf, 0.5_hf, vec2f(0.5f)), 0.666_hf, statueAsset->asTextureView()->tex);
			scene1Widget->addChild(scene1StatueWidget);

			scene1StatueCrackedWidget = ImageWidget::createByHeight(uictx, Pos(0.5_wf, 0.5_hf, vec2f(0.5f)), 0.666_hf, statueCrackedAsset->asTextureView()->tex);
			scene1Widget->addChild(scene1StatueCrackedWidget);
		}


		uictx.addRootWidget(rootWidget);
	}


	// virtual void onPreUpdate(const GameUpdateSets& u) {}

	virtual void onPostUpdate(const GameUpdateSets& u) {
		uictx.update(u.is, getWorld()->userProjectionSettings.canvasSize, u.dt);

		{
			scene1Widget->setOpacity(clamp01(totalProgress / 0.5f));

			scene1StatueWidget->setOpacity(computeOpacity(totalProgress, 0.f, 1.f, 2.f, 2.5f));
			scene1StatueCrackedWidget->setOpacity(computeOpacity(totalProgress, 2.5f, 5.f, 7.f, 9.5f));
		}

		if (totalProgress > 10.f) {
			getWorld()->addPostSceneTask(new PostSceneUpdateTaskLoadWorldFormFile("assets/levels/MainMenu.lvl", true));;
		}

		totalProgress += u.dt;
	}

	virtual void onPostDraw(const GameDrawSets& drawSets) {
		gamegui::UIDrawSets drawSetsUI;
		drawSetsUI.setup(drawSets.rdest, drawSets.quickDraw);

		uictx.draw(drawSetsUI);
	}

	float totalProgress = 0.f;
};
DefineTypeId(CinematicMenuScript, 21'08'30'0001);
ReflBlock() {
	ReflAddScript(CinematicMenuScript) ReflMember(CinematicMenuScript, totalProgress);
}

} // namespace sge
