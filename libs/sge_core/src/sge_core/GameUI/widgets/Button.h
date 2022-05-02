#pragma once

#include "sge_core/GameUI/IWidget.h"
#include "sge_utils/react/Event.h"
#include "sge_utils/math/color.h"

namespace sge {

struct QuickFont;
}

namespace sge::gamegui {

//----------------------------------------------------
// ButtonWidget
//----------------------------------------------------
struct SGE_CORE_API ButtonWidget final : public IWidget {
	ButtonWidget(UIContext& owningContext, Pos position, Size size)
	    : IWidget(owningContext)
	{
		setPosition(position);
		setSize(size);
	}

	static std::shared_ptr<ButtonWidget>
	    create(UIContext& owningContext, Pos position, Size size, const char* const text = nullptr);
	static std::shared_ptr<ButtonWidget>
	    createImageWidth(UIContext& owningContext, Pos position, Size size, const char* const text = nullptr);

	bool isGamepadTargetable() override { return true; }

	void draw(const UIDrawSets& drawSets) override;

	void setColor(const vec4f& c) { m_textColor = c; }
	void setBgColor(const vec4f& up, const vec4f& hovered, const vec4f& pressed)
	{
		m_bgColorUp = up;
		m_bgColorHovered = hovered;
		m_bgColorPressed = pressed;
	}
	void setText(std::string s) { m_text = std::move(s); }
	void setArrow(SignedAxis axis) { m_triangleDir = axis; }
	void setFont(QuickFont* font) { m_font = font; }
	void setFontSize(Unit fontSize) { m_fontSize = fontSize; }

	bool onHoverEnter() override
	{
		m_isHovered = true;
		return true;
	}

	bool onHoverLeave() override
	{
		m_isHovered = false;
		return true;
	}

	bool onPress() override
	{
		m_isPressed = true;
		return true;
	}

	void onRelease(bool wasReleaseInside) override
	{
		m_isPressed = false;
		if (wasReleaseInside) {
			m_onReleaseListeners.invokeEvent();
		}
	}

	/// @brief Adds a function to get called when the button has been released (cursor over the widget) or when
	/// interacted with the gamepad.
	[[nodiscard]] EventSubscription subscribe_onRelease(std::function<void()> fn)
	{
		return m_onReleaseListeners.subscribe(std::move(fn));
	}

  private:
	EventEmitter<> m_onReleaseListeners;

	bool m_isHovered = false;
	bool m_isPressed = false;

	QuickFont* m_font = nullptr;
	vec4f m_textColor = vec4f(1.f);
	vec4f m_bgColorUp = colorBlack(0.66f);
	vec4f m_bgColorHovered = colorBlack(0.78f);
	vec4f m_bgColorPressed = colorBlack(0.22f);
	Unit m_fontSize = Unit::fromFrac(1.f);
	std::string m_text;
	SignedAxis m_triangleDir = SignedAxis::signedAxis_numElements;
};


} // namespace sge::gamegui
