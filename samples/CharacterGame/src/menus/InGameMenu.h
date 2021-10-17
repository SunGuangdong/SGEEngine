#pragma once

#include "sge_core/GameUI/UIContext.h"
#include "sge_core/GameUI/Widget.h"
#include "sge_core/QuickDraw.h"
#include "sge_engine/IWorldScript.h"
#include "sge_utils/utils/Event.h"

namespace sge {

struct InGameMenuScript : public IWorldScript {
	void create();
	void onPostUpdate(const GameUpdateSets& u) override;
	void onPostDraw(const GameDrawSets& drawSets) override;

	gamegui::UIContext context;
	std::vector<EventSubscription> eventSubs;
	DebugFont font;

	std::shared_ptr<gamegui::TextWidget> livesTextWdg;

	std::shared_ptr<gamegui::InvisibleWidget> hud;
	std::shared_ptr<gamegui::ColoredWidget> blackScreen;
	std::shared_ptr<gamegui::InvisibleWidget> deadScreen;
};
} // namespace sge
