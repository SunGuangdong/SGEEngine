#pragma once

#include <chrono>
#include <vector>

#include "IWidget.h"
#include "sge_core/sgecore_api.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/containers/Optional.h"
#include "sge_utils/math/Box3f.h"
#include "sge_utils/math/Random.h"
#include "sge_utils/math/color.h"
#include "sge_utils/math/vec4f.h"
#include "sge_utils/react/Event.h"

#include "Sizing.h"

#include <atomic>

namespace sge {

struct QuickDraw;
struct InputState;
struct QuickFont;

} // namespace sge

namespace sge::gamegui {

struct UIDrawSets;
struct UIContext;


/// @brief InvisibleWidget is just an invisible widget with size and position
/// useful for position and aligning objects or root widget for menus.
struct SGE_CORE_API InvisibleWidget final : public IWidget {
	InvisibleWidget(UIContext& owningContext, Pos position, Size size)
	    : IWidget(owningContext)
	{
		setPosition(position);
		setSize(size);
	}

	static std::shared_ptr<InvisibleWidget> create(UIContext& owningContext, Pos position, Size size)
	{
		std::shared_ptr<InvisibleWidget> w = std::make_shared<InvisibleWidget>(owningContext, position, size);
		return w;
	}

	virtual void draw(const UIDrawSets& UNUSED(drawSets)) override {}
};

/// @brief ColoredWidget is non-interactable widget with a constant color.
struct SGE_CORE_API ColoredWidget final : public IWidget {
	ColoredWidget(UIContext& owningContext, Pos position, Size size)
	    : IWidget(owningContext)
	{
		setPosition(position);
		setSize(size);
	}

	static std::shared_ptr<ColoredWidget> create(UIContext& owningContext, Pos position, Size size);

	virtual void draw(const UIDrawSets& drawSets) override;

	void setColor(const vec4f& c) { m_color = c; }

	bool onMouseWheel(int cnt) override
	{
		Pos co = getContentsOrigin().getAsFraction(getParentBBoxSS().size());
		co.posY.value -= float(cnt) * 0.2f;
		setContentsOrigin(co);
		return true;
	}

  public:
	vec4f m_color = vec4f(1.f);
};

//----------------------------------------------------
// TextWidget
//----------------------------------------------------
struct SGE_CORE_API TextWidget final : public IWidget {
	TextWidget(UIContext& owningContext, Pos position, Size size)
	    : IWidget(owningContext)
	{
		setPosition(position);
		setSize(size);
	}

	static std::shared_ptr<TextWidget> create(UIContext& owningContext, Pos position, Size size, const char* text)
	{
		auto w = std::make_shared<TextWidget>(owningContext, position, size);
		if (text) {
			w->setText(text);
		}
		return w;
	}

	virtual void draw(const UIDrawSets& drawSets) override;

	void setColor(const vec4f& c) { m_color = c; }
	void setText(std::string s) { m_text = std::move(s); }
	void setFont(QuickFont* font) { m_font = font; }
	void setFontSize(Unit fontSize) { m_fontSize = fontSize; }

	bool m_algnTextHCenter = true;
	bool m_algnTextVCenter = true;

	bool enableYScroll = false;
	float yScrollPixels = 0.f;
	bool isScrollingY = false;

  private:
	QuickFont* m_font = nullptr;
	vec4f m_color = vec4f(1.f);
	Unit m_fontSize = Unit::fromFrac(1.f);
	std::string m_text;
};

/// ImageWidget is a widget that display a texture.
struct SGE_CORE_API ImageWidget final : public IWidget {
	ImageWidget(UIContext& owningContext, Pos position, Size size)
	    : IWidget(owningContext)
	{
		setPosition(position);
		setSize(size);
	}

	static std::shared_ptr<ImageWidget>
	    create(UIContext& owningContext, Pos position, Unit width, GpuHandle<Texture> texture);
	static std::shared_ptr<ImageWidget>
	    createByHeight(UIContext& owningContext, Pos position, Unit height, GpuHandle<Texture> texture);
	virtual void draw(const UIDrawSets& drawSets) override;

  private:
	GpuHandle<Texture> m_texture;
	vec4f uvRegion = vec4f(0.f, 0.f, 1.f, 1.f);
};

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

//----------------------------------------------------
// HorizontalComboBox
//----------------------------------------------------
struct SGE_CORE_API HorizontalComboBox final : public IWidget {
	HorizontalComboBox(UIContext& owningContext, Pos position, Size size);

	static std::shared_ptr<HorizontalComboBox> create(UIContext& owningContext, Pos position, Size size);
	void draw(const UIDrawSets& drawSets) override;

	void addOption(std::string option)
	{
		m_options.emplace_back(std::move(option));
		if (m_currentOption < 0) {
			m_currentOption = 0;
		}
	}

  private:
	void leftPressed();
	void rightPressed();

	EventSubscription m_leftPressSub;
	EventSubscription m_rightPressSub;

	std::weak_ptr<ButtonWidget> m_leftBtn;
	std::weak_ptr<ButtonWidget> m_rightBtn;

	int m_currentOption = -1;
	std::vector<std::string> m_options;
};

//----------------------------------------------------
// Checkbox
//----------------------------------------------------
struct SGE_CORE_API Checkbox final : public IWidget {
	Checkbox(UIContext& owningContext, Pos position, Size size)
	    : IWidget(owningContext)
	{
		setPosition(position);
		setSize(size);
	}

	static std::shared_ptr<Checkbox>
	    create(UIContext& owningContext, Pos position, Size size, const char* const text, bool isOn);
	bool isGamepadTargetable() override { return true; }
	void draw(const UIDrawSets& drawSets) override;
	bool onPress() override;
	void onRelease(bool wasReleaseInside) override;

	void setText(std::string s) { m_text = std::move(s); }

	EventSubscription onRelease(std::function<void(bool)> fn) { return m_onReleaseListeners.subscribe(std::move(fn)); }

  private:
	bool m_isOn = false;
	bool m_isPressed = false;
	std::string m_text;

	EventEmitter<bool> m_onReleaseListeners;
};

} // namespace sge::gamegui
