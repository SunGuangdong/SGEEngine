#include "sge_core/GameUI/UIContext.h"
#include "sge_core/GameUI/Widget.h"
#include "sge_core/ICore.h"
#include "sge_core/QuickDraw.h"
#include "sge_engine/GameDrawer/GameDrawer.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/IWorldScript.h"
#include "sge_engine/ScriptObject.h"

#include "sge_utils/credits/sge_all_3rd_party_credits.h"

namespace sge {

using namespace gamegui;
using namespace gamegui::literals;

struct MainMenuScript : public IWorldScript {
	gamegui::UIContext context;

	std::vector<EventSubscription> eventSubs;

	std::shared_ptr<gamegui::InvisibleWidget> mainMenuWidget;
	std::shared_ptr<gamegui::InvisibleWidget> creditsMenuWidget;
	std::shared_ptr<gamegui::TextWidget> creditsText;

	// std::shared_ptr<gamegui::IWidget> mainMenu;
	DebugFont font;

	void create() override {
		context.create(getWorld()->userProjectionSettings.canvasSize);

		font.Create(getCore()->getDevice(), "assets/editor/fonts/AutourOne-Regular.ttf", 128);

		context.setDefaultFont(&font);

		auto rootMenuWidget = std::make_shared<gamegui::InvisibleWidget>(context, Pos(0_f, 0_f), Size(1_f, 1_f));

		{
			mainMenuWidget = std::make_shared<gamegui::InvisibleWidget>(context, Pos(0.50_f, 0.85_f, vec2f(0.5f)), Size(0.55_hf, 0.15_hf));

			std::shared_ptr<gamegui::ButtonWidget> newGameBtn = ButtonWidget::create(context, Pos(0_f, 0_f), Size(1_wf, 0.5_hf), "Play");
			eventSubs.push_back(newGameBtn->subscribe_onRelease(
			    [&] { getWorld()->addPostSceneTask(new PostSceneUpdateTaskLoadWorldFormFile("assets/levels/game.lvl", true)); }));
			mainMenuWidget->addChild(newGameBtn);

			newGameBtn->setColor(colorFromIntRgba(255, 158, 80));

			std::shared_ptr<gamegui::ButtonWidget> creditsBtn =
			    ButtonWidget::create(context, Pos(0_f, 0.5_hf), Size(1_wf, 0.5_hf), "Credits");
			mainMenuWidget->addChild(creditsBtn);

			creditsBtn->setColor(colorFromIntRgba(255, 158, 80));

			eventSubs.push_back(creditsBtn->subscribe_onRelease([&] {
				creditsMenuWidget->unsuspend();
				mainMenuWidget->suspend();
			}));

			rootMenuWidget->addChild(mainMenuWidget);
		}

		{
			creditsMenuWidget = std::make_shared<gamegui::InvisibleWidget>(context, Pos(0.0_f, 0.0_f, vec2f(0.0f)), Size(1_f, 1_f));

			creditsText = creditsMenuWidget->addChildT(
			    std::make_shared<gamegui::TextWidget>(context, Pos(0.0_f, 0.0_f, vec2f(0.0f)), Size(1_f, 0.8_f)));

			std::string creditsTextStr = R"txt(





Art - Christina Angelova @chrisiadraws				
Programming - Kostadin Petkov @ongamex

Background music pixelsphere.org / The Cynic Project.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

				)txt";
			creditsTextStr += get_sge_all_3rd_party_credits();
			creditsText->m_algnTextHCenter = false;
			creditsText->m_algnTextVCenter = false;
			creditsText->setText(creditsTextStr);
			creditsText->setFontSize(0.02_wf);
			creditsText->enableYScroll = true;

			auto creditsBack =
			    creditsMenuWidget->addChildT(ButtonWidget::create(context, Pos(0_f, 0.81_f, vec2f(0.f)), Size(1_wf, 0.19_hf), "Back"));
			eventSubs.push_back(creditsBack->subscribe_onRelease([&] {
				creditsMenuWidget->suspend();
				mainMenuWidget->unsuspend();
			}));
			creditsBack->setColor(colorFromIntRgba(255, 158, 80));

			creditsMenuWidget->suspend();

			rootMenuWidget->addChild(creditsMenuWidget);
		}


		context.addRootWidget(rootMenuWidget);
	}


	// virtual void onPreUpdate(const GameUpdateSets& u) {}

	virtual void onPostUpdate(const GameUpdateSets& u) {
		if (u.isGamePaused() == false) {
			context.update(u.is, getWorld()->userProjectionSettings.canvasSize, u.dt);

			if (creditsMenuWidget->isSuspended() == false) {
				creditsText->yScrollPixels -= 30.f * u.dt;
			}
			else {
				creditsText->yScrollPixels = 0.f;
			}

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
