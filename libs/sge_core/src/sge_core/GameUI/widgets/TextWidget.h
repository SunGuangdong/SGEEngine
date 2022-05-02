#pragma once

#include "sge_core/GameUI/UIContext.h"
#include "sge_core/GameUI/IWidget.h"

namespace sge::gamegui {

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

} // namespace sge::gamegui
