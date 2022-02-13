#include "MultiCurve2D.h"
#include "sge_utils/math/Hermite.h"
#include "sge_utils/math/common.h"
#include "sge_utils/sge_utils.h"
#include <algorithm>
#include <cfloat>

namespace sge {

int MultiCurve2D::findBezierH0(const int iBezierKey) const
{
	if (isIndexValid(iBezierKey) == false) {
		return -1;
	}

	if (pointType_isBezierKey(m_pointsWs[iBezierKey].type) == false) {
		return -1;
	}

	const int loopEnd = std::min(int(m_pointsWs.size()), iBezierKey + 3);
	for (int t = iBezierKey + 1; t < loopEnd; ++t) {
		if (pointType_isBezierHandle(m_pointsWs[t].type)) {
			if (m_pointsWs[t].type == pointType_bezierHandle0) {
				return t;
			}
		}
		else {
			// Do not assert here as this is used in isCurveValid()
			// sgeAssert(false && "This point cannot be anything different but a bezier handle!");
			return -1;
		}
	}

	return -1;
}

int MultiCurve2D::findBezierH1(const int iBezierKey) const
{
	if (isIndexValid(iBezierKey) == false) {
		return -1;
	}

	if (pointType_isBezierKey(m_pointsWs[iBezierKey].type) == false) {
		return -1;
	}

	const int loopEnd = std::min(int(m_pointsWs.size()), iBezierKey + 3);
	for (int t = iBezierKey + 1; t < loopEnd; ++t) {
		if (pointType_isBezierHandle(m_pointsWs[t].type)) {
			if (m_pointsWs[t].type == pointType_bezierHandle1) {
				return t;
			}
		}
		else {
			// Do not assert here as this is used in isCurveValid()
			// sgeAssert(false && "This point cannot be anything different but a bezier handle!");
			return -1;
		}
	}

	return -1;
}

int MultiCurve2D::findHandleKeyPoint(const int iHandle) const
{
	if (isIndexValid(iHandle) == false) {
		return -1;
	}

	if (!pointType_isBezierHandle(m_pointsWs[iHandle].type)) {
		return -1;
	}

	for (int t = iHandle - 1; t >= 0; --t) {
		if (pointType_isBezierKey(m_pointsWs[t].type)) {
			return t;
		}
	}

	return -1;
}

int MultiCurve2D::findHandleAttachmentPoint(const int iHandle) const
{
	if (isIndexValid(iHandle) == false) {
		return -1;
	}

	if (m_pointsWs[iHandle].type == pointType_bezierHandle0) {
		return findHandleKeyPoint(iHandle);
	}

	if (m_pointsWs[iHandle].type == pointType_bezierHandle1) {
		for (int t = iHandle + 1; t < m_pointsWs.size(); ++t) {
			if (pointType_isControlPoint(m_pointsWs[t].type)) {
				return t;
			}
		}
	}

	return -1;
}

int MultiCurve2D::findNextKeyPoint(const int iStart) const
{
	if (isIndexValid(iStart) == false) {
		return -1;
	}

	for (int t = iStart + 1; t >= m_pointsWs.size(); ++t) {
		if (pointType_isControlPoint(m_pointsWs[t].type)) {
			return t;
		}
	}

	return -1;
}

int MultiCurve2D::findPrevKeyPoint(const int iStart) const
{
	if (isIndexValid(iStart) == false) {
		return -1;
	}

	for (int t = iStart - 1; t >= 0; --t) {
		if (pointType_isControlPoint(m_pointsWs[t].type)) {
			return t;
		}
	}

	return -1;
}


bool MultiCurve2D::isCurveValid() const
{
	float prevPointX = -FLT_MAX;
	for (int t = 0; t < m_pointsWs.size(); ++t) {
		const Point pt = m_pointsWs[t];

		// Points must be sorted by their x value.
		if (pt.x <= prevPointX) {
			return false;
		}

		if (pt.type == pointType_bezierKey) {
			const int h0 = findBezierH0(t);
			const int h1 = findBezierH1(t);

			if (!isIndexValid(h0) || !isIndexValid(h1)) {
				return false;
			}

			const bool isH0PossibleIdx = (h0 - t) == 1 || (h0 - t) == 2;
			const bool isH1PossibleIdx = (h1 - t) == 1 || (h1 - t) == 2;

			if (!isH0PossibleIdx || !isH1PossibleIdx || h0 == h1) {
				return false;
			}
		}
		else if (pointType_isBezierHandle(pt.type)) {
			const int key = findHandleKeyPoint(t);
			if (key < 0 || ((t - key) > 2)) {
				return false;
			}
		}

		prevPointX = pt.x;
	}

	return true;
}

void MultiCurve2D::sortPointsByX()
{
	for (int i = 0; i < m_pointsWs.size(); ++i) {
		for (int j = i + 1; j < m_pointsWs.size(); ++j) {
			if (m_pointsWs[j].x < m_pointsWs[i].x) {
				std::swap(m_pointsWs[i], m_pointsWs[j]);
			}
		}
	}
}

bool MultiCurve2D::sortPointsByX(std::vector<int>& outIndexRemap)
{
	std::vector<int> indicesLut;
	for (int i = 0; i < m_pointsWs.size(); ++i) {
		indicesLut.emplace_back(i);
	}

	bool hasOrderChanged = false;
	for (int i = 0; i < m_pointsWs.size(); ++i) {
		for (int j = i + 1; j < m_pointsWs.size(); ++j) {
			if (m_pointsWs[j].x < m_pointsWs[i].x) {
				hasOrderChanged = true;

				std::swap(indicesLut[i], indicesLut[j]);
				std::swap(m_pointsWs[i], m_pointsWs[j]);
			}
		}
	}

	outIndexRemap.resize(indicesLut.size());
	for (int i = 0; i < indicesLut.size(); ++i) {
		outIndexRemap[indicesLut[i]] = i;
	}

	return hasOrderChanged;
}

bool MultiCurve2D::addSmoothPointSafe(const float x, const float y)
{
	sgeAssert(isCurveValid());

	if (m_pointsWs.empty()) {
		addPointUnsafe(Point(pointType_smooth, x, y));
		return true;
	}

	// Find the apropriate index for the point.
	int placeIndex = -1;
	for (int t = 0; t < m_pointsWs.size(); ++t) {
		if (pointType_isBezierHandle(m_pointsWs[t].type)) {
			// The point cannot be placed between points
			continue;
		}

		if (x > m_pointsWs[t].x) {
			placeIndex = t + 1;
		}
	}

	std::vector<Point> originals = m_pointsWs;

	// Place the point.
	if (placeIndex >= 0 && placeIndex < m_pointsWs.size()) {
		m_pointsWs.insert(m_pointsWs.begin() + placeIndex, Point(pointType_smooth, x, y));
	}
	else if (placeIndex == -1) {
		// push the point to the front.
		m_pointsWs.insert(m_pointsWs.begin(), Point(pointType_smooth, x, y));
	}
	else if (placeIndex == m_pointsWs.size()) {
		// If the old last point is a bezier key, add its handles.
		if (pointType_isBezierKey(m_pointsWs.back().type)) {
			const float oldLastX = m_pointsWs.back().x;
			const float oldLasty = m_pointsWs.back().y;

			const float xDiff = x - oldLastX;
			const float h0x = oldLastX + xDiff * 0.1f;
			const float h1x = oldLastX + xDiff * 0.9f;
			m_pointsWs.emplace_back(Point(pointType_bezierHandle0, h0x, oldLasty));
			m_pointsWs.emplace_back(Point(pointType_bezierHandle1, h1x, y));
		}

		m_pointsWs.emplace_back(Point(pointType_smooth, x, y));
		return true;
	}
	else {
		sgeAssert(false && "Should never happen, above situations should handle everything!");
		return false;
	}

	if (isCurveValid()) {
		return true;
	}

	sgeAssert(false && "The curve is expected to be valid!");
	m_pointsWs = std::move(originals);
	return false;
}

bool MultiCurve2D::removePointSafe(const int idx, std::vector<int>& outIndexRemap)
{
	if (isIndexValid(idx) == false) {
		return false;
	}

	if (isCurveValid() == false) {
		return false;
	}

	outIndexRemap.clear();
	outIndexRemap.resize(getNumPoints());
	for (int t = 0; t < int(outIndexRemap.size()); ++t) {
		outIndexRemap[t] = t;
	}

	if (pointType_isBezierHandle(m_pointsWs[idx].type)) {
		// Bezier handle points cannot be deleted. Delete the keypoint itself.
		return false;
	}
	else if (pointType_isBezierKey(m_pointsWs[idx].type)) {
		const int h0 = findBezierH0(idx);
		const int h1 = findBezierH1(idx);

		if ((h0 < 0) != (h1 < 0)) {
			sgeAssert(
			    false &&
			    "Both keypoints must be valid or not valid at the same time. They are could be invalid if the key point is the last point");
			return false;
		}

		if (h0 > idx && h1 > h0) {
			const int hFar = std::max(h0, h1);
			const int hNear = std::min(h0, h1);

			removePointUnsafe(hFar);
			removePointUnsafe(hNear);
			removePointUnsafe(idx);

			outIndexRemap[hFar] = -1;
			outIndexRemap[hNear] = -1;
			outIndexRemap[idx] = -1;
		}
		else {
			sgeAssert(false);
			return false;
		}
	}
	else {
		// Key point without handles.
		removePointUnsafe(idx);
		outIndexRemap[idx] = -1;
	}

	int counter = 0;
	for (int& remap : outIndexRemap) {
		if (remap == -1) {
			continue;
		}
		remap = counter;
		counter++;
	}

	sgeAssert(isCurveValid() && "The curve is expected to be valid");
	return true;
}

bool MultiCurve2D::chnagePointType(const int idx, const PointType newType)
{
	if (isIndexValid(idx) == false) {
		return false;
	}

	if (m_pointsWs[idx].type == newType) {
		// The types already match.
		return true;
	}

	if (pointType_isControlPoint(m_pointsWs[idx].type) == false) {
		// Only control point can change type.
		return false;
	}

	std::vector<Point> originals = m_pointsWs;

	// Do the change.
	if (pointType_isBezierKey(newType)) {
		if (pointType_isBezierKey(m_pointsWs[idx].type)) {
			// Bezier Key -> Bezier Key
			sgeAssert(false && "Shouldn't happen!");
		}
		else {
			// Regular -> Bezier Key
			if (m_pointsWs.size() == idx + 1) {
				// Making the last point bezier, doesn't need handles.
				m_pointsWs[idx].type = newType;
			}
			else {
				const float x0 = m_pointsWs[idx].x;
				const float x1 = m_pointsWs[idx + 1].x;

				const float y0 = m_pointsWs[idx].y;
				const float y1 = m_pointsWs[idx + 1].y;

				const float xDiff = x1 - x0;

				const float h0x = x0 + xDiff * 0.1f;
				const float h1x = x0 + xDiff * 0.9f;

				m_pointsWs[idx].type = newType;
				m_pointsWs.insert(m_pointsWs.begin() + idx + 1, Point(pointType_bezierHandle0, h0x, y0));
				m_pointsWs.insert(m_pointsWs.begin() + idx + 2, Point(pointType_bezierHandle1, h1x, y1));
			}
		}
	}
	else {
		if (pointType_isBezierKey(m_pointsWs[idx].type)) {
			// Regular -> Regular
			m_pointsWs[idx].type = newType;
		}
		else {
			// Bezier -> Regular
			const int h0 = findBezierH0(idx);
			const int h1 = findBezierH1(idx);
			removePointUnsafe(h0);
			removePointUnsafe(h1);
			m_pointsWs[idx].type = newType;
		}
	}


	// Check if the modification was valid, if not revert back to the original.
	if (isCurveValid()) {
		return true;
	}

	sgeAssert(false && "The curve is expected to be valid!");
	m_pointsWs = std::move(originals);
	return false;
}

void MultiCurve2D::addPointUnsafe(const Point& pt)
{
	m_pointsWs.emplace_back(pt);
}

bool MultiCurve2D::removePointUnsafe(int idx)
{
	if (isIndexValid(idx) == false) {
		return false;
	}

	m_pointsWs.erase(m_pointsWs.begin() + idx);
	return true;
}

float MultiCurve2D::sample(const float x) const
{
	if (m_pointsWs.empty()) {
		return 0.f;
	}

	int iBasePt = -1;
	bool wasBasePtFound = false;

	for (int t = int(m_pointsWs.size()) - 1; t >= 0; --t) {
		if ((x > m_pointsWs[t].x) && !pointType_isBezierHandle(m_pointsWs[t].type)) {
			iBasePt = t;
			wasBasePtFound = true;
			break;
		}
	}

	if (!wasBasePtFound) {
		return m_pointsWs.front().y;
	}

	sgeAssert(isIndexValid(iBasePt));

	if (iBasePt + 1 == int(m_pointsWs.size())) {
		return m_pointsWs.back().y;
	}

	if (m_pointsWs[iBasePt].type == pointType_constant) {
		return m_pointsWs[iBasePt].y;
	}
	else if (m_pointsWs[iBasePt].type == pointType_linear) {
		const Point pt0 = m_pointsWs[iBasePt];
		const Point pt1 = m_pointsWs[iBasePt + 1];
		const float t = (x - pt0.x) / (pt1.x - pt0.x);
		const float res = (1.f - t) * pt0.y + t * pt1.y;
		return res;
	}
	else if (m_pointsWs[iBasePt].type == pointType_smooth) {
		float points[4];

		const int i0 = iBasePt - 1;
		const int i1 = iBasePt;
		const int i2 = iBasePt + 1;
		const int i3 = iBasePt + 2;

		// The control points.
		points[1] = m_pointsWs[i1].y;
		points[2] = m_pointsWs[i2].y;

		const int n = getNumPoints();

		points[0] = (i0 < 0) ? points[1] - (points[2] - points[1]) : m_pointsWs[i0].y;
		points[3] = (i3 >= n) ? points[2] + (points[2] - points[1]) : m_pointsWs[i3].y;
		points[0] = (i0 < 0) ? points[1] : m_pointsWs[i0].y;
		points[3] = (i3 >= n) ? points[2] : m_pointsWs[i3].y;

		const float t = (x - m_pointsWs[i1].x) / (m_pointsWs[i2].x - m_pointsWs[i1].x);
		return hermiteEval(t, points);
	}
	else if (m_pointsWs[iBasePt].type == pointType_bezierKey) {
		const Point h0 = m_pointsWs[findBezierH0(iBasePt)];
		const Point h1 = m_pointsWs[findBezierH1(iBasePt)];
		const Point cp0 = m_pointsWs[iBasePt];
		const Point cp1 = m_pointsWs[iBasePt + 3];

		// TODO: proper evaluation please!
		const float Ax = cp0.x;
		const float Bx = h0.x;
		const float Cx = h1.x;
		const float Dx = cp1.x;

#if 1
		float region[2] = {0.f, 1.f};
		float t = (x - cp0.x) / (cp1.x - cp0.x); // 0.5f;

		// while (fabsf(region[0] - region[1]) > 1e-6f) {
		for (int itr = 0; itr < 21; ++itr) {
			t = (region[0] + region[1]) * 0.5f;

			const float AB = lerp(Ax, Bx, t);
			const float BC = lerp(Bx, Cx, t);
			const float CD = lerp(Cx, Dx, t);

			const float ABC = lerp(AB, BC, t);
			const float BCD = lerp(BC, CD, t);

			const float ix = lerp(ABC, BCD, t);
			if (fabsf(ix - x) < 1e-6f) {
				break;
			}

			if (x > ix) {
				region[0] = t;
			}
			if (x < ix) {
				region[1] = t;
			}
		}
#else
		const float t = (x - cp0.x) / (cp1.x - cp0.x);
#endif

		const vec2f A(cp0.x, cp0.y);
		const vec2f B(h0.x, h0.y);
		const vec2f C(h1.x, h1.y);
		const vec2f D(cp1.x, cp1.y);

		vec2f AB = lerp(A, B, t);
		vec2f BC = lerp(B, C, t);
		vec2f CD = lerp(C, D, t);

		vec2f ABC = lerp(AB, BC, t);
		vec2f BCD = lerp(BC, CD, t);

		vec2f result = lerp(ABC, BCD, t);
		return result.y;
	}

	sgeAssert(false);
	return 0.f;
} // namespace sge

} // namespace sge
