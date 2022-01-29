#include "MultiCurve2DEditor.h"
#include "sge_core/SGEImGui.h"
#include "sge_utils/math/Box.h"
#include "sge_utils/text/format.h"

#include <unordered_set>
#include <vector>

namespace sge {

enum class State {
	Idle,
	CameraPan,
	CreatingSelectionRect,
	MovingPoints,
};

struct StateStorage {
	std::unordered_set<int> m_selectedIndices;
	State m_state = State::Idle;
	vec2f m_prevCursorCS = vec2f(0.f);
	vec2f m_selectionRectStartPointCS;
	vec2f m_selectionRectEndPointCS;

	// The index of the point that the movement is initiated.
	// This is used when we move selection as well, we do the movement relative to it.
	int m_movedPointIdx = -1;

	// The position of the camera in function space. The camera position translates to the center of the client space.
	vec2f m_camPosFS = vec2f(0.f, 0.f);
	vec2f m_camViewSizeFS = vec2f(2.1f, 2.1f); // The view area of the camera in function space.

	//
	bool expandedWindowIsOpened = false;

	void remapIndicesBecauseTheCurveChanged(const std::vector<int>& remap) {
		std::unordered_set<int> newSelection;
		for (const int sel : m_selectedIndices) {
			if_checked(remap.size() > sel) {
				if (remap[sel] >= 0) {
					newSelection.insert(remap[sel]);
				}
			}
		}

		m_selectedIndices = std::move(newSelection);

		if (m_movedPointIdx >= 0 && int(remap.size()) > m_movedPointIdx) {
			if (remap[m_movedPointIdx] >= 0) {
				m_movedPointIdx = remap[m_movedPointIdx];
			}
		}
	}
};

void MultiCurve2DEditor(const char* const widgetName, MultiCurve2D& m_fn, vec2f widgetSize, bool isThisExpandedOfAnother) {
	const char* const kContextPopupName = "MultiCurve2DEditorContext Menu";
	const float kPointRadius = 4.f;

	if (isThisExpandedOfAnother) {
		ImVec2 available = ImGui::GetContentRegionAvail();
		widgetSize.x = available.x;
		widgetSize.y = available.y - 50.f;
	} else {
		widgetSize.x = widgetSize.x < 0 ? ImGui::CalcItemWidth() + (ImGui::GetStyle().FramePadding.x * 2.f) : widgetSize.x;
		widgetSize.y = widgetSize.y < 0 ? floorf(widgetSize.x * 0.50f) : widgetSize.y;
	}

	widgetSize.x = std::max(16.f, widgetSize.x);
	widgetSize.y = std::max(16.f, widgetSize.y);


	if (!ImGui::BeginChild(ImGui::GetID(widgetName), toImGui(widgetSize), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove)) {
		ImGui::EndChild();
		return;
	}

	ImGui::BeginGroup();

	StateStorage& ss = getUIState<StateStorage>();

	vec2f& m_camPosFS = ss.m_camPosFS;
	vec2f& m_camViewSizeFS = ss.m_camViewSizeFS;


	if (ImGui::IsWindowHovered() && (ImGui::IsMouseDown(2) || ImGui::IsMouseDown(1))) {
		ImGui::SetWindowFocus();
	}


	const ImVec2 windowCanvasSize = ImGui::GetContentRegionAvail();
	const ImVec2 canvasPosSS = ImGui::GetCursorScreenPos();

	// Coordinate conversion functions:
	const auto function2client = [&widgetSize, &m_camPosFS, &m_camViewSizeFS](const vec2f& fs) -> vec2f {
		const vec2f camPosCS = widgetSize * 0.5f;
		vec2f pixelPerUnit = widgetSize / m_camViewSizeFS;
		pixelPerUnit.y *= -1.f;
		const vec2f diffFS = fs - m_camPosFS;
		const vec2f res = camPosCS + diffFS * pixelPerUnit;
		return res;
	};

	const auto client2function = [&widgetSize, &m_camPosFS, &m_camViewSizeFS](const vec2f& cs) -> vec2f {
		const vec2f camPosCS = widgetSize * 0.5f;
		vec2f unitsPerPixel = m_camViewSizeFS / widgetSize;
		unitsPerPixel.y *= -1.f;
		const vec2f diffCS = cs - camPosCS;
		const vec2f res = m_camPosFS + unitsPerPixel * diffCS;
		return res;
	};

	const auto screen2client = [windowCanvasSize, canvasPosSS](vec2f posSS) -> vec2f {
		vec2f result = posSS - fromImGui(canvasPosSS);
		return result;
	};
	const auto clientToScreen = [&canvasPosSS](vec2f posCS) -> vec2f { return fromImGui(canvasPosSS) + posCS; };
	const auto function2screen = [&clientToScreen, function2client](vec2f posFN) -> vec2f {
		return clientToScreen(function2client(posFN));
	};


	// Process the user input:
	const vec2f cursorCS = screen2client(fromImGui(ImGui::GetMousePos()));
	vec2f cursorFS = client2function(cursorCS);

	if (ss.m_state == State::Idle && ImGui::IsWindowFocused()) {
		if (ImGui::IsMouseDoubleClicked(0)) {
			m_fn.addSmoothPointSafe(cursorFS.x, cursorFS.y);
		} else if (ImGui::GetIO().MouseWheel) {
			// If the cursor is over some point zoom towards that point, otherwise zoom towards the curson in function space.
			vec2f zoomTargetCS = cursorCS;
			vec2f zoomTargetInitialFS = cursorFS;
			for (int iPt = 0; iPt < int(m_fn.getPoints().size()); ++iPt) {
				const MultiCurve2D::Point& pt = m_fn.getPoints()[iPt];
				const vec2f ptCS = function2client(pt.getPos());
				const float distance = (ptCS - cursorCS).length();
				if (distance <= kPointRadius) {
					zoomTargetCS = ptCS;
					zoomTargetInitialFS = pt.getPos();
					break;
				}
			}

			// Do the zooming
			const float zoomFactor = -ImGui::GetIO().MouseWheel / 120.f;
			m_camViewSizeFS *= 1.f + zoomFactor * 10.f;

			const vec2f newTargetPointPreCameraMoveCS = function2client(client2function(zoomTargetCS));
			const vec2f movementFS = client2function(newTargetPointPreCameraMoveCS) - zoomTargetInitialFS;
			m_camPosFS -= movementFS;

			// The cached function space position of the cursor has changed, update it.
			cursorFS = client2function(cursorCS);

		} else if (ImGui::IsMouseClicked(1) && ImGui::IsWindowHovered()) {
			ImGui::OpenPopup(kContextPopupName);
		} else if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered()) {
			// Check if any point is under the cursor, if so, select it.
			int ptIndexUnderCursor = -1;
			for (int iPt = 0; iPt < int(m_fn.getPoints().size()); ++iPt) {
				const MultiCurve2D::Point& pt = m_fn.getPoints()[iPt];
				const vec2f ptCS = function2client(vec2f(pt.x, pt.y));
				const float distance = (ptCS - cursorCS).length();
				if (distance <= kPointRadius) {
					ptIndexUnderCursor = iPt;
					break;
				}
			}

			if (ptIndexUnderCursor == -1) {
				ss.m_selectedIndices.clear();
				ss.m_state = State::CreatingSelectionRect;
				ss.m_selectionRectStartPointCS = cursorCS;
			} else {
				// If the hovered point is selected the user wants to move points
				// otherwise the user wants to select (and move) this point alone.
				if (ss.m_selectedIndices.count(ptIndexUnderCursor)) {
					ss.m_state = State::MovingPoints;
					ss.m_movedPointIdx = ptIndexUnderCursor;
				} else {
					ss.m_selectedIndices.clear();
					ss.m_selectedIndices.insert(ptIndexUnderCursor);
					ss.m_state = State::MovingPoints;
					ss.m_movedPointIdx = ptIndexUnderCursor;
				}
			}
		}

		if (ImGui::IsMouseDown(2)) {
			// The user wants to pan the camera.
			ss.m_state = State::CameraPan;
		}
	}

	ImGui::SetNextWindowPos(ImGui::GetMousePos(), ImGuiCond_Appearing);
	if (ImGui::BeginPopup(kContextPopupName)) {
		// Context menu.
		if (ImGui::MenuItem("Make Linear")) {
			for (int sel : ss.m_selectedIndices) {
				m_fn.chnagePointType(sel, MultiCurve2D::pointType_linear);
			}
		}
		if (ImGui::MenuItem("Make Constant")) {
			for (int sel : ss.m_selectedIndices) {
				m_fn.chnagePointType(sel, MultiCurve2D::pointType_constant);
			}
		}
		if (ImGui::MenuItem("Make Bezier")) {
			for (int sel : ss.m_selectedIndices) {
				m_fn.chnagePointType(sel, MultiCurve2D::pointType_bezierKey);
			}
		}

		ImGui::Separator();
		if (ImGui::MenuItem("Delete")) {
			std::vector<int> remap;
			while (ss.m_selectedIndices.empty() == false) {
				if (!m_fn.removePointSafe(*ss.m_selectedIndices.begin(), remap)) {
					sgeAssert(false);
					ss.m_selectedIndices.clear();
					break;
				}

				ss.remapIndicesBecauseTheCurveChanged(remap);
			}
		}

		ImGui::Separator();
		if (ImGui::MenuItem("Fit Stretchy")) {
			if (m_fn.getNumPoints() == 0) {
				m_camPosFS = vec2f(0.f);
				float w = std::max(widgetSize.x, widgetSize.y);
				m_camViewSizeFS = 2.f * widgetSize * (1.f / w);
			} else {
				AABox2f bboxFs;
				if (ss.m_selectedIndices.empty() || ss.m_selectedIndices.size() == 1) {
					for (auto& pt : m_fn.getPoints()) {
						bboxFs.expand(pt.getPos());
					}
				} else {
					for (const int iSel : ss.m_selectedIndices) {
						bboxFs.expand(m_fn.getPoints()[iSel].getPos());
					}
				}

				if (bboxFs.halfDiagonal().length() > 1e-5f) {
					bboxFs.expandEdgesByCoefficient(0.05f);

					m_camPosFS = bboxFs.center();

					m_camViewSizeFS = bboxFs.diagonal();

					// Stop zooming when number get too small.
					m_camViewSizeFS.x = std::max(m_camViewSizeFS.x, 1e-3f);
					m_camViewSizeFS.y = std::max(m_camViewSizeFS.y, 1e-3f);
				} else {
					m_camPosFS = bboxFs.center();
				}
			}
		}
		if (ImGui::MenuItem("Fit Regular")) {
			if (m_fn.getNumPoints() == 0) {
				m_camPosFS = vec2f(0.f);
				float w = std::max(widgetSize.x, widgetSize.y);
				m_camViewSizeFS = 2.f * widgetSize * (1.f / w);
			} else {
				AABox2f bboxFs;
				if (ss.m_selectedIndices.empty() || ss.m_selectedIndices.size() == 1) {
					for (auto& pt : m_fn.getPoints()) {
						bboxFs.expand(pt.getPos());
					}
				} else {
					for (const int iSel : ss.m_selectedIndices) {
						bboxFs.expand(m_fn.getPoints()[iSel].getPos());
					}
				}

				if (bboxFs.halfDiagonal().length() > 1e-5f) {
					bboxFs.expandEdgesByCoefficient(0.05f);

					m_camPosFS = bboxFs.center();

					// Fit that fits the curve in a region while making the size/function coverage equal for x and y.
					vec2f ratios = bboxFs.diagonal() / widgetSize;
					if (ratios.x < ratios.y) {
						m_camViewSizeFS.x = bboxFs.diagonal().y * widgetSize.x / widgetSize.y;
						m_camViewSizeFS.y = bboxFs.diagonal().y;
					} else {
						m_camViewSizeFS.x = bboxFs.diagonal().x;
						m_camViewSizeFS.y = bboxFs.diagonal().x * widgetSize.y / widgetSize.x;
					}


					// Stop zooming when number get too small.
					m_camViewSizeFS.x = std::max(m_camViewSizeFS.x, 1e-3f);
					m_camViewSizeFS.y = std::max(m_camViewSizeFS.y, 1e-3f);
				} else {
					m_camPosFS = bboxFs.center();
				}
			}
		}

		ImGui::EndPopup();
	}

	if (ss.m_state == State::CameraPan) {
		const vec2f prevCursorFS = client2function(ss.m_prevCursorCS);
		const vec2f cursorMovementFs = cursorFS - prevCursorFS;

		m_camPosFS -= cursorMovementFs;
		if (ImGui::IsMouseReleased(2)) {
			ss.m_state = State::Idle;
		}
	}

	if (ss.m_state == State::CreatingSelectionRect) {
		ss.m_selectionRectEndPointCS = cursorCS;

		// The user is done selecting check if we've selected anything.
		if (ImGui::IsMouseReleased(0)) {
			ss.m_selectedIndices.clear();

			AABox2f selectionRectFS;
			selectionRectFS.expand(client2function(ss.m_selectionRectStartPointCS));
			selectionRectFS.expand(client2function(ss.m_selectionRectEndPointCS));

			// Check if any points are inside this rect.
			for (int iPt = 0; iPt < int(m_fn.getPoints().size()); ++iPt) {
				const MultiCurve2D::Point& pt = m_fn.getPoints()[iPt];
				if (selectionRectFS.isInside(vec2f(pt.x, pt.y))) {
					ss.m_selectedIndices.insert(iPt);
				}
			}

			ss.m_state = State::Idle;
		}
	}

	if (ss.m_state == State::MovingPoints) {
		if (ImGui::IsMouseReleased(0) || !m_fn.isIndexValid(ss.m_movedPointIdx)) {
			ss.m_movedPointIdx = -1;
			ss.m_state = State::Idle;
		} else {
			const vec2f originPointFs = m_fn.getPoints()[ss.m_movedPointIdx].getPos();
			const vec2f movementFs = cursorFS - originPointFs;

			MultiCurve2D original = m_fn;

			for (const int iSel : ss.m_selectedIndices) {
				if_checked(m_fn.isIndexValid(iSel)) {
					m_fn.getPointsMutable()[iSel].x += movementFs.x;
					m_fn.getPointsMutable()[iSel].y += movementFs.y;
				}
			}

			std::vector<int> indexRemap;
			const bool needsIndexRemap = m_fn.sortPointsByX(indexRemap);

			if (m_fn.isCurveValid() == false) {
				m_fn = std::move(original);
			} else {
				if (needsIndexRemap) {
					ss.remapIndicesBecauseTheCurveChanged(indexRemap);
				}
			}
		}
	}

	// Draw the curve:
	ImDrawList* const wndDrawList = ImGui::GetWindowDrawList();

	// Draw the functiom pane:
	{
		std::string number;
		const vec2f bottomLeftFS = m_camPosFS - m_camViewSizeFS * 0.5f;
		const vec2f topRightFS = m_camPosFS + m_camViewSizeFS * 0.5f;
		{
			// const int expY = int(floorf(log10f(m_camViewSizeFS.x)));
			const int expYSmall = int(floorf(log10f(m_camViewSizeFS.y / 10.f)));
			const float yIncrementsSmall = powf(10.f, float(expYSmall));
			float yDrawSmall = yIncrementsSmall * roundf(bottomLeftFS.y / yIncrementsSmall);

			if (yIncrementsSmall > 1e-7f)
				while (yDrawSmall < topRightFS.y) {
					if (expYSmall < 0) {
						string_format(number, "%.*f", -expYSmall, yDrawSmall);
					} else {
						string_format(number, "%0.*f", abs(expYSmall), yDrawSmall);
					}


					bool showText = false;
					int color = 0;
					if (int(roundf(fabsf(yDrawSmall) * powf(10.f, float(-expYSmall)))) % 10 == 0) {
						float f = clamp((yIncrementsSmall * 10.f / m_camViewSizeFS.y), 0.f, 1.f);
						color = IM_COL32(0xFF, 0xFF, 0xFF, int(255.f * f));
						showText = true;
					} else {
						float f = clamp((yIncrementsSmall / m_camViewSizeFS.y), 0.f, 0.25f);
						color = IM_COL32(0xFF, 0xFF, 0xFF, int(255.f * f));
					}

					if (fabsf(yDrawSmall) < yIncrementsSmall * 0.1f) {
						number = "0";
						color = 0xFF0000FF;
					}


					vec2f leftPosSS = function2screen(vec2f(bottomLeftFS.x, yDrawSmall));
					vec2f rightPosSS = function2screen(vec2f(topRightFS.x, yDrawSmall));
					wndDrawList->AddLine(toImGui(leftPosSS), toImGui(rightPosSS), color);
					if (showText)
						wndDrawList->AddText(toImGui(leftPosSS), color, number.c_str());
					yDrawSmall += yIncrementsSmall;
				}
		}

		{
			const int expXSmall = int(floorf(log10f(m_camViewSizeFS.x / 10.f)));
			const float xIncrementsSmall = powf(10.f, float(expXSmall));

			float xDrawSmall = xIncrementsSmall * roundf(bottomLeftFS.x / xIncrementsSmall);
			if (xIncrementsSmall > 1e-7f)
				while (xDrawSmall < topRightFS.x) {
					if (expXSmall < 0) {
						string_format(number, "%.*f", -expXSmall, xDrawSmall);
					} else {
						string_format(number, "%0.f", xDrawSmall);
					}

					bool showText = false;
					int color = 0;
					if (int(roundf(fabsf(xDrawSmall) * powf(10.f, float(-expXSmall)))) % 10 == 0) {
						float f = clamp((xIncrementsSmall * 10.f / m_camViewSizeFS.x), 0.f, 1.f);
						color = IM_COL32(0xFF, 0xFF, 0xFF, int(255.f * f));
						showText = true;
					} else {
						float f = clamp((xIncrementsSmall / m_camViewSizeFS.x), 0.f, 0.25f);
						color = IM_COL32(0xFF, 0xFF, 0xFF, int(255.f * f));
					}
					if (fabsf(xDrawSmall) < xIncrementsSmall * 0.1f) {
						number = "0";
						color = 0xFF00FF00;
					}

					vec2f bottomPos = function2screen(vec2f(xDrawSmall, bottomLeftFS.y));
					vec2f topPos = function2screen(vec2f(xDrawSmall, topRightFS.y));
					wndDrawList->AddLine(toImGui(bottomPos), toImGui(topPos), color);
					if (showText)
						wndDrawList->AddText(toImGui(topPos), color, number.c_str());
					xDrawSmall += xIncrementsSmall;
				}
		}
	}

	// Draw the function:
	vec2f prevFnPointCS = vec2f(0.f, 0.f);
	for (float xCS = 0.f; xCS < widgetSize.x; ++xCS) {
		const float xFn = client2function(vec2f(xCS, 0.f)).x;
		const float yFn = m_fn.sample(xFn);

		const vec2f pointCS = function2client(vec2f(xFn, yFn));

		if (xCS != 0.f) {
			wndDrawList->AddLine(toImGui(clientToScreen(prevFnPointCS)), toImGui(clientToScreen(pointCS)), 0xFFFF0000);
		}

		prevFnPointCS = pointCS;
	}


	// Draw Lines for the handle points.
	{
		for (int iPt = 0; iPt < int(m_fn.getPoints().size()); ++iPt) {
			const MultiCurve2D::Point& pt = m_fn.getPoints()[iPt];

			if (!MultiCurve2D::pointType_isBezierHandle(pt.type)) {
				continue;
			}

			int iAttacment = m_fn.findHandleAttachmentPoint(iPt);
			if (m_fn.isIndexValid(iAttacment)) {
				wndDrawList->AddLine(toImGui(function2screen(m_fn.getPoints()[iPt].getPos())),
				                     toImGui(function2screen(m_fn.getPoints()[iAttacment].getPos())), 0x66000000);
			}
		}
	}

	// Draw the key points.
	{
		int ptIndexUnderCursor = -1;
		for (int iPt = 0; iPt < int(m_fn.getPoints().size()); ++iPt) {
			const MultiCurve2D::Point& pt = m_fn.getPoints()[iPt];
			const vec2f ptCS = function2client(vec2f(pt.x, pt.y));
			const float distance = (ptCS - cursorCS).length();
			if (distance <= kPointRadius) {
				ptIndexUnderCursor = iPt;
				break;
			}
		}

		for (int iPt = 0; iPt < int(m_fn.getPoints().size()); ++iPt) {
			const MultiCurve2D::Point& pt = m_fn.getPoints()[iPt];
			const vec2f ptSS = clientToScreen(function2client(vec2f(pt.x, pt.y)));

			if (iPt == ptIndexUnderCursor) {
				wndDrawList->AddCircleFilled(toImGui(ptSS), kPointRadius, 0xAA339099);
			} else {
				if (ss.m_selectedIndices.count(iPt)) {
					wndDrawList->AddCircleFilled(toImGui(ptSS), kPointRadius, 0xAA000599);
				} else {
					wndDrawList->AddCircleFilled(toImGui(ptSS), kPointRadius, 0xAA000522);
					wndDrawList->AddCircleFilled(toImGui(ptSS), kPointRadius, 0xAA000522);
				}
			}
		}
	}

	// Draw the selection rectangle.
	if (ss.m_state == State::CreatingSelectionRect) {
		AABox2f selectionRectCS;
		selectionRectCS.expand(ss.m_selectionRectStartPointCS);
		selectionRectCS.expand(ss.m_selectionRectEndPointCS);
		const vec2f selRectMinSS = clientToScreen(selectionRectCS.min);
		const vec2f selRectMaxSS = clientToScreen(selectionRectCS.max);
		wndDrawList->AddRectFilled(toImGui(selRectMinSS), toImGui(selRectMaxSS), 0x22000000, 0.f);
	}

	ss.m_prevCursorCS = cursorCS;

	static float dummy[2];

	ImGui::EndGroup();

	ImGui::EndChild();
	ImGui::PushItemWidth(100.f);
	ImGui::DragFloat2("##Coords3", dummy);
	ImGui::SameLine();
	if (isThisExpandedOfAnother == false) {
		if (ImGui::Button("Expand")) {
			ss.expandedWindowIsOpened = true;
		}

		if (ss.expandedWindowIsOpened == true) {
			if (ImGui::Begin(widgetName, &ss.expandedWindowIsOpened, ImGuiWindowFlags_NoDocking)) {
				MultiCurve2DEditor(widgetName, m_fn, vec2f(-1.f, -1.f), true);
			}
			ImGui::End();
		}
	}
}
} // namespace sge
