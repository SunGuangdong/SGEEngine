#include "ColumnLayout.h"

namespace sge::gamegui {

void ColumnLayout::addWidget(std::shared_ptr<IWidget> widget)
{
	addChild(widget);
	alignedWidgets.emplace_back(std::move(widget));
}

// void ColumnLayout::removeWidget(std::shared_ptr<IWidget> widget)
//{
//	for (int t = 0; t < alignedWidgets.size(); ++t) {
//		if (alignedWidgets[t] == widget) {
//			alignedWidgets.erase(alignedWidgets.begin() + t);
//			// removeChild????
//			return;
//		}
//	}
//}

void ColumnLayout::update()
{
	const Box2f thisBBoxSSPixels = getBBoxPixelsSS();
	const vec2f thisSize = thisBBoxSSPixels.diagonal();
	const vec2f spacingPixels = {
	    spacingX.computeSizePixels(true, thisSize), spacingY.computeSizePixels(true, thisSize)};

	const vec2f fistWidgetPosition = startPosition.toPixels(thisSize);
	vec2f nextWidgetPosition = fistWidgetPosition;
	float nextLineStartX = leftToRight ? -FLT_MAX : FLT_MAX;
	int numWidgetsInColumn = 0;

	for (std::shared_ptr<IWidget>& widget : alignedWidgets) {
		// Check if the widget would fit in the line.
		widget->m_position.posX = Unit::fromPixels(nextWidgetPosition.x);
		widget->m_position.posY = Unit::fromPixels(nextWidgetPosition.y);
		Box2f widgetBboxPixelsPs = widget->getBBoxPixelsParentSpace();

		bool doesWidgetFit = false;
		if (topToBottom) {
			doesWidgetFit = widgetBboxPixelsPs.max.y <= thisSize.y + 1e-6f;
		}
		else {
			doesWidgetFit = widgetBboxPixelsPs.min.y >= 0.f - 1e-6f;
		}

		if ((!doesWidgetFit && breakInMultipleColumnsToFit) ||
		    (maxWidgetsPerColumn != 0 && (numWidgetsInColumn >= maxWidgetsPerColumn))) {
			// Widget wouldn't fit, move to the next column.
			nextWidgetPosition.y = fistWidgetPosition.y;
			numWidgetsInColumn = 0;
			if (leftToRight) {
				if (nextLineStartX != -FLT_MAX) {
					nextWidgetPosition.x = nextLineStartX + spacingPixels.x;
					nextLineStartX = -FLT_MAX;
				}
			}
			else {
				if (nextLineStartX != FLT_MAX) {
					nextWidgetPosition.x = nextLineStartX - spacingPixels.x;
					nextLineStartX = FLT_MAX;
				}
			}

			// Place the widget in a new column:
			widget->m_position.posX = Unit::fromPixels(nextWidgetPosition.x);
			widget->m_position.posY = Unit::fromPixels(nextWidgetPosition.y);
			widgetBboxPixelsPs = widget->getBBoxPixelsParentSpace();
		}

		if (topToBottom) {
			nextWidgetPosition.y = widgetBboxPixelsPs.max.y + spacingPixels.y;
		}
		else {
			nextWidgetPosition.y = widgetBboxPixelsPs.min.y - spacingPixels.y;
		}


		// Update the lowset point on the current line
		// so we know where the next line (if any) would be:
		if (leftToRight) {
			if (widgetBboxPixelsPs.max.x > nextLineStartX) {
				nextLineStartX = widgetBboxPixelsPs.max.x;
			}
		}
		else {
			if (widgetBboxPixelsPs.min.x < nextLineStartX) {
				nextLineStartX = widgetBboxPixelsPs.min.x;
			}
		}

		numWidgetsInColumn += 1;
	}
}

} // namespace sge::gamegui
