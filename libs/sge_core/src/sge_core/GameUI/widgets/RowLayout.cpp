#include "RowLayout.h"

namespace sge::gamegui {

void RowLayout::addWidget(std::shared_ptr<IWidget> widget)
{
	addChild(widget);
	alignedWidgets.emplace_back(std::move(widget));
}

// void HorizontalLayout::removeWidget(std::shared_ptr<IWidget> widget)
//{
//	for (int t = 0; t < alignedWidgets.size(); ++t) {
//		if (alignedWidgets[t] == widget) {
//			alignedWidgets.erase(alignedWidgets.begin() + t);
//			// removeChild????
//			return;
//		}
//	}
//}

void RowLayout::update()
{
	for (int t = 0; t < alignedWidgets.size(); ++t) {
		if (alignedWidgets[t] == nullptr) {
			alignedWidgets.erase(alignedWidgets.begin() + t);
			t--;
		}
	}

	const Box2f thisBBoxSSPixels = getBBoxPixelsSS();
	const vec2f thisSize = thisBBoxSSPixels.diagonal();
	const vec2f spacingPixels = {
	    spacingX.computeSizePixels(true, thisSize), spacingY.computeSizePixels(true, thisSize)};
	const vec2f fistWidgetPosition = startPosition.toPixels(thisSize);

	vec2f nextWidgetPosition = fistWidgetPosition;
	float nextLineStartY = topToBottom ? -FLT_MAX : FLT_MAX;
	int numWidgetsInCurrentRow = 0;

	for (std::shared_ptr<IWidget>& widget : alignedWidgets) {
		// Check if the widget would fit in the line.
		widget->m_position.posX = Unit::fromPixels(nextWidgetPosition.x);
		widget->m_position.posY = Unit::fromPixels(nextWidgetPosition.y);
		Box2f widgetBboxPixelsPs = widget->getBBoxPixelsParentSpace();

		bool doesWidgetFit = false;
		if (leftToRight) {
			doesWidgetFit = widgetBboxPixelsPs.max.x <= thisSize.x + 1e-6f;
		}
		else {
			doesWidgetFit = widgetBboxPixelsPs.min.x >= 0.f - 1e-6f;
		}

		if ((!doesWidgetFit && breakInMultipleRowsToFit) || (maxWidgetsPerRow > 0 && (numWidgetsInCurrentRow >= maxWidgetsPerRow))) {
			numWidgetsInCurrentRow = 0;
			// Widget wouldn't fit, move to the next row.
			nextWidgetPosition.x = fistWidgetPosition.x;
			if (topToBottom) {
				if (nextLineStartY != -FLT_MAX) {
					nextWidgetPosition.y = nextLineStartY + spacingPixels.y;
					nextLineStartY = -FLT_MAX;
				}
			}
			else {
				if (nextLineStartY != FLT_MAX) {
					nextWidgetPosition.y = nextLineStartY - spacingPixels.y;
					nextLineStartY = FLT_MAX;
				}
			}

			// Place the widget on a new row:
			widget->m_position.posX = Unit::fromPixels(nextWidgetPosition.x);
			widget->m_position.posY = Unit::fromPixels(nextWidgetPosition.y);
			widgetBboxPixelsPs = widget->getBBoxPixelsParentSpace();
		}

		if (leftToRight) {
			nextWidgetPosition.x = widgetBboxPixelsPs.max.x + spacingPixels.x;
		}
		else {
			nextWidgetPosition.x = widgetBboxPixelsPs.min.x - spacingPixels.x;
		}


		// Update the lowset point on the current line
		// so we know where the next line (if any) would be:
		if (topToBottom) {
			if (widgetBboxPixelsPs.max.y > nextLineStartY) {
				nextLineStartY = widgetBboxPixelsPs.max.y;
			}
		}
		else {
			if (widgetBboxPixelsPs.min.y < nextLineStartY) {
				nextLineStartY = widgetBboxPixelsPs.min.y;
			}
		}

		numWidgetsInCurrentRow += 1;
	}
}

} // namespace sge::gamegui
