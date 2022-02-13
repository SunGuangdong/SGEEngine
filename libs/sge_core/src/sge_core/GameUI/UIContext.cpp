#include <functional>

#include "Layout.h"
#include "UIContext.h"
#include "Widget.h"
#include "sge_core/ICore.h"
#include "sge_core/QuickDraw.h"
#include "sge_core/application/input.h"
#include "sge_utils/math/Box3f.h"

namespace sge::gamegui {

void UIContext::update(const InputState& is, const vec2i& canvasSize, const float dt)
{
	const float kGamepadDirInputCooldownDuration = 0.25f;
	m_gamepadDirInputCooldown -= dt;
	m_canvasSize = canvasSize;

	bool shouldUseGamepad = m_isUsingGamepad;
	if (is.xinputDevicesState[0].hooked && is.xinputDevicesState[0].hadInputThisPoll) {
		shouldUseGamepad = true;
	}
	if (is.m_hadkeyboardOrMouseInputThisPoll) {
		shouldUseGamepad = false;
	}

	m_isUsingGamepad = shouldUseGamepad && is.xinputDevicesState[0].hooked;

	std::function<void(const std::shared_ptr<IWidget>&)> updateWidget = [&](const std::shared_ptr<IWidget>& w) {
		if (w->isSuspended()) {
			w->update();
			return;
		}

		if (w->m_wasHoveredPrevFrame) {
			Box2f bboxss = w->getScissorBoxSS();

			if (m_isUsingGamepad) {
				w->m_wasHoveredPrevFrame = false;
				w->onHoverLeave();
			}
			else {
				if (!bboxss.isInside(is.GetCursorPos())) {
					w->m_wasHoveredPrevFrame = false;
					w->onHoverLeave();
				}
			}
		}

		w->update();

		if (w->getLayout()) {
			w->getLayout()->update();
		}

		for (const std::shared_ptr<IWidget>& child : w->getChildren()) {
			updateWidget(child);
		}
	};

	for (std::shared_ptr<IWidget>& widget : m_rootWidgets) {
		updateWidget(widget);
	}

	if (m_isUsingGamepad) {
		if (getGamepadTarget() == nullptr || getGamepadTarget()->isSuspended()) {
			std::function<bool(const std::shared_ptr<IWidget>&)> findGamepadTarget = [&](const std::shared_ptr<IWidget>& w) -> bool {
				if (w->isSuspended()) {
					return false;
				}

				if (w->isGamepadTargetable()) {
					setGamepadTarget(w);
					return true;
				}

				for (const std::shared_ptr<IWidget>& child : w->getChildren()) {
					if (findGamepadTarget(child)) {
						break;
					}
				}

				return false;
			};

			for (const std::shared_ptr<IWidget>& widget : m_rootWidgets) {
				if (findGamepadTarget(widget)) {
					break;
				}
			}
		}

		std::shared_ptr<IWidget> gamepadTarget = getGamepadTarget();
		if (gamepadTarget) {
			vec2f gamePadInputDir = is.xinputDevicesState[0].getInputDir(true).normalized0();

			// Flip Y, as screen cordinates along Y increate downwards.
			gamePadInputDir.y *= -1.f;

			if (m_gamepadDirInputCooldown <= 0.f && gamePadInputDir != vec2f(0.f)) {
				m_gamepadDirInputCooldown = kGamepadDirInputCooldownDuration;
				const vec2f oldTargetWidgetCenterSS = gamepadTarget->getScissorBoxSS().center();

				std::shared_ptr<IWidget> newTarget = nullptr;
				float currentK = FLT_MAX;
				std::function<void(const std::shared_ptr<IWidget>&)> findAllGamepadTargets =
				    [&](const std::shared_ptr<IWidget>& w) -> void {
					if (w == gamepadTarget || w->isSuspended()) {
						return;
					}

					if (w->isGamepadTargetable()) {
						vec2f currWidgetCenterSS = w->getScissorBoxSS().center();

						vec2f d = currWidgetCenterSS - oldTargetWidgetCenterSS;
						if (d != vec2f(0.f)) {
							float dLength = d.length();
							float angle = acosf(dot(d, gamePadInputDir) / dLength);
							float k = (d * dot(d, gamePadInputDir)).length();
							if (angle < 45.f && dot(d, gamePadInputDir) > 0.f && k < currentK) {
								currentK = k;
								newTarget = w;
							}
						}
					}
					else {
						for (const std::shared_ptr<IWidget>& child : w->getChildren()) {
							findAllGamepadTargets(child);
						}
					}
				};

				for (std::shared_ptr<IWidget>& widget : m_rootWidgets) {
					findAllGamepadTargets(widget);
				}

				if (newTarget) {
					setGamepadTarget(newTarget);
					gamepadTarget = newTarget;
				}
			}

			if (is.xinputDevicesState[0].isBtnReleased(GamepadState::btn_a)) {
				if (gamepadTarget->isSuspended() == false) {
					gamepadTarget->onRelease(true);
				}
			}
		}
	}
	else {
		std::vector<std::weak_ptr<IWidget>> hoveredWidgetsChain;
		std::function<void(const std::shared_ptr<IWidget>&)> handleCursor = [&](const std::shared_ptr<IWidget>& w) {
			if (w->isSuspended()) {
				return;
			}

			hoveredWidgetsChain.push_back(w);

			for (const std::shared_ptr<IWidget>& child : w->getChildren()) {
				Box2f bboxss = child->getScissorBoxSS();

				if (bboxss.isInside(is.GetCursorPos())) {
					handleCursor(child);
				}
			}
		};

		for (const std::shared_ptr<IWidget>& widget : m_rootWidgets) {
			Box2f bboxss = widget->getScissorBoxSS();

			if (bboxss.isInside(is.GetCursorPos()) && widget->isSuspended() == false) {
				handleCursor(widget);
				break;
			}
		}

		std::reverse(hoveredWidgetsChain.begin(), hoveredWidgetsChain.end());
		for (std::weak_ptr<IWidget> widgetWeak : hoveredWidgetsChain) {
			std::shared_ptr<IWidget> widget = widgetWeak.lock();
			if (widget) {
				if (widget->m_wasHoveredPrevFrame) {
					break;
				}

				if (widget->onHoverEnter()) {
					widget->m_wasHoveredPrevFrame = true;
					break;
				}
			}
		}

		if (is.IsKeyPressed(Key_MouseLeft)) {
			for (std::weak_ptr<IWidget> widgetWeak : hoveredWidgetsChain) {
				std::shared_ptr<IWidget> widget = widgetWeak.lock();
				if (widget->onPress()) {
					m_pressedWidgetWeak = widget;
					break;
				}
			}
		}

		if (is.m_wheelCount != 0) {
			for (std::weak_ptr<IWidget> widgetWeak : hoveredWidgetsChain) {
				std::shared_ptr<IWidget> widget = widgetWeak.lock();
				if (widget->onMouseWheel(is.m_wheelCount)) {
					break;
				}
			}
		}


		if (auto pressedWidget = m_pressedWidgetWeak.lock(); pressedWidget && is.IsKeyReleased(Key_MouseLeft)) {
			bool isCursorInside = pressedWidget->getScissorBoxSS().isInside(is.GetCursorPos());
			pressedWidget->onRelease(isCursorInside);
			m_pressedWidgetWeak.reset();
		}
	}
}

void UIContext::draw(const UIDrawSets& drawSets)
{
	std::function<void(const std::shared_ptr<IWidget>&)> drawWidget = [&](const std::shared_ptr<IWidget>& w) {
		if (w->isSuspended()) {
			return;
		}

		w->draw(drawSets);
		for (const std::shared_ptr<IWidget>& child : w->getChildren()) {
			drawWidget(child);
		}
	};

	for (const std::shared_ptr<IWidget>& widget : m_rootWidgets) {
		drawWidget(widget);
	}

	if (auto gamepadTarget = getGamepadTarget(); m_isUsingGamepad && gamepadTarget && !gamepadTarget->isSuspended()) {
		Box2f bb = gamepadTarget->getBBoxPixels();
		drawSets.quickDraw->drawRect(drawSets.rdest, bb, vec4f(1.f, 1.f, 0.f, 0.33f),
		                             getCore()->getGraphicsResources().BS_backToFrontAlpha);
	}
}

} // namespace sge::gamegui
