#pragma once

#include "sge_utils/math/vec2f.h"
#include <unordered_map>
#include <vector>

namespace sge {

struct MultiCurve2D {
  public:
	enum PointType : int {
		pointType_linear,
		pointType_constant,
		pointType_smooth,
		pointType_bezierKey,
		pointType_bezierHandle0,
		pointType_bezierHandle1,
	};

	static bool pointType_isBezierHandle(const PointType pt) { return pt == pointType_bezierHandle0 || pt == pointType_bezierHandle1; }

	static bool pointType_isBezierKey(const PointType pt) { return pt == pointType_bezierKey; }

	static bool pointType_isControlPoint(const PointType pt) { return !pointType_isBezierHandle(pt); }

	struct Point {
		Point() = default;
		Point(PointType type, float x, float y)
		    : type(type)
		    , x(x)
		    , y(y) {}

		vec2f getPos() const { return vec2f(x, y); }

		PointType type = pointType_linear;
		float x = 0.f;
		float y = 0.f;
	};

  public:
	bool isIndexValid(int idx) const { return idx >= 0 && idx < m_pointsWs.size(); }

	int findBezierH0(const int iBezierKey) const;
	int findBezierH1(const int iBezierKey) const;

	int findHandleKeyPoint(const int iHandle) const;
	int findHandleAttachmentPoint(const int iHandle) const;

	int findNextKeyPoint(const int iStart) const;
	int findPrevKeyPoint(const int iStart) const;

	bool isCurveValid() const;
	void sortPointsByX();
	bool sortPointsByX(std::vector<int>& outIndexRemap);

	bool addSmoothPointSafe(const float x, const float y);
	bool removePointSafe(const int idx, std::vector<int>& outIndexRemap);
	bool chnagePointType(const int idx, const PointType newType);

	void addPointUnsafe(const Point& pt);
	bool removePointUnsafe(const int idx);

	float sample(const float x) const;
	float sampleDerivative(const float x) const;

	const std::vector<Point>& getPoints() const { return m_pointsWs; }

	const int getNumPoints() const { return int(m_pointsWs.size()); }

	std::vector<Point>& getPointsMutable() { return m_pointsWs; }

  public:
	std::vector<Point> m_pointsWs;
};


} // namespace sge
