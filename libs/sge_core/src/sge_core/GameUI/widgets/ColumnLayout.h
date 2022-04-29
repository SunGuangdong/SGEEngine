#pragma once

#include "sge_core/GameUI/Widget.h"

namespace sge::gamegui {

/// A widget that aligns other widgets in a column.
/// The row could be left-to-right or right-to-left and top-to-bottom and bottom-to-top.
/// The aligned widgets are added by @addWidget and are also made a child of this layout.
struct SGE_CORE_API ColumnLayout final : public IWidget {
	ColumnLayout(UIContext& owningContext)
	    : IWidget(owningContext)
	{
	}

	/// Adds the widgert to be aligned by the class.
	/// The widget would also get added as a child to the layout.
	void addWidget(std::shared_ptr<IWidget> widget);

	// void removeWidget(std::shared_ptr<IWidget> widget);

	void update() override;
	void draw(const UIDrawSets& UNUSED(drawSets)) override {}

  public:
	Pos startPosition; ///< The starting position in parent space of the 1st widget.
	Unit spacingX;     ///< The horizontal spacing between widgets (if multiple columns are needed).
	Unit spacingY;     ///< The vertical spacing between widgets.

	/// True if the widgets are to be layed out left to right.
	/// A friendly reminded that if you alignment doesn't seem right to check the anchor of the
	/// widgets being aligned.
	bool leftToRight = true;

	/// True if the next line of widgets should be below the previous one.
	/// A friendly reminded that if you alignment doesn't seem right to check the anchor of the
	/// widgets being aligned.
	bool topToBottom = true;

	/// If true and if the widgets woudn't fit on a single line
	/// the layout would start a new line of widgets below it.
	/// otherwise all widgets would be in a single line even if the laoyut widget isn't big enough.
	bool breakInMultipleColumns = true;

  private:
	/// The list of all widgets to be alligned.
	/// Use @addWidget to add widgets.
	std::vector<std::shared_ptr<IWidget>> alignedWidgets;
};

} // namespace sge::gamegui
