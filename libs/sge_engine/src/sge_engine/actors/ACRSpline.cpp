#include "ACRSpline.h"
#include "sge_engine/GameInspector.h"
#include "sge_utils/math/Hermite.h"

namespace sge {

RelfAddTypeId(ACRSpline, 20'03'02'0023);
// clang-format off

ReflBlock() {
	ReflAddActor(ACRSpline)
		ReflMember(ACRSpline, points)
	;
}
// clang-format on

bool TraitPath3DForACRSpline::isEmpty() const {
	const Actor* a = getActor();
	if (a && a->getType() == sgeTypeId(ACRSpline)) {
		const ACRSpline* const spline = static_cast<const ACRSpline*>(a);
		return spline->getNumPoints() == 0;
	}

	sgeAssert(false);
	return true;
}

bool TraitPath3DForACRSpline::evaluateAtDistance(vec3f* outPosition, vec3f* outTanget, float distance) {
	Actor* a = getActor();
	if (a && a->getType() == sgeTypeId(ACRSpline)) {
		ACRSpline* const spline = (ACRSpline*)a;
		return spline->evaluateAtDistance(outPosition, outTanget, distance);
	}

	return false;
}

float TraitPath3DForACRSpline::getTotalLength() {
	Actor* a = getActor();
	if (a && a->getType() == sgeTypeId(ACRSpline)) {
		ACRSpline* const spline = (ACRSpline*)a;
		return spline->getTotalLength();
	}

	return 0.0f;
}

void ACRSpline::create() {
	registerTrait(traitPath);
	registerTrait(m_traitViewportIcon);
	m_traitViewportIcon.setTexture("assets/editor/textures/icons/obj/ACRSpline.png", true);

	points.clear();
	points.push_back(vec3f(0.f, 0.f, 0.f));
	points.push_back(vec3f(1.f, 0.f, 0.f));
	computeSegmentsLength();
}

AABox3f ACRSpline::getBBoxOS() const {
	AABox3f bbox;

	for (const vec3f& pt : points)
		bbox.expand(pt);

	bbox.expand(getTransform().p);


	return bbox;
}

void ACRSpline::onMemberChanged() {
	makeDirty();
	computeSegmentsLength();
}

bool ACRSpline::evalute(vec3f* outPosition, vec3f* outTanget, float t) {
	int iSegment = clamp((int)t, 0, getNumPoints() - 2);
	vec3f verts[4];
	getPointsForSegment(verts, iSegment);

	const float k = clamp(t - iSegment, 0.f, 1.f);

	if (outPosition) {
		*outPosition = hermiteEval(k, verts);
	}

	if (outTanget) {
		*outTanget = hermiteEvalTanget(k, verts);
	}

	return true;
}

bool ACRSpline::evaluateAtDistance(vec3f* outPosition, vec3f* outTanget, float distance) {
	if (getNumPoints() == 0)
		return false;

	int iBest = 0;
	for (int t = 0; t < distanceSamples.size(); ++t) {
		iBest = t;
		if (distanceSamples[t] >= distance) {
			break;
		}
	}

	const float i0 = (float)(iBest) / (float)distanceSamples.size() * (points.size() - 1);
	const float i1 = (float)(iBest + 1) / (float)distanceSamples.size() * (points.size() - 1);

	const float a = iBest > 0 ? distanceSamples[iBest - 1] : 0.f;
	const float b = distanceSamples[iBest];

	const float k = (distance - a) / (b - a);
	const float t = lerp(i0, i1, k);

	return this->evalute(outPosition, outTanget, t);
}

void ACRSpline::getPointsForSegment(vec3f result[4], const int iSegment) const {
	const int i0 = iSegment - 1;
	const int i1 = iSegment;
	const int i2 = iSegment + 1;
	const int i3 = iSegment + 2;

	// The control points.
	result[1] = points[i1];
	result[2] = points[i2];

	const int n = getNumPoints();

	result[0] = (i0 < 0) ? result[1] - (result[2] - result[1]) : points[i0];
	result[3] = (i3 >= n) ? result[2] + (result[2] - result[1]) : points[i3];
	result[0] = (i0 < 0) ? result[1] : points[i0];
	result[3] = (i3 >= n) ? result[2] : points[i3];
}

void ACRSpline::computeSegmentsLength() {
	totalLength = 0.f;
	if (getNumPoints() <= 1) {
		return;
	}

	distanceSamples.resize(points.size() * 10);

	vec3f oldPt;
	this->evalute(&oldPt, nullptr, 0.f);

	for (int t = 0; t < distanceSamples.size(); ++t) {
		float k = (float)(t + 1) / (float)(distanceSamples.size());
		vec3f pt;
		this->evalute(&pt, nullptr, k * (points.size() - 1));
		totalLength += distance(pt, oldPt);
		distanceSamples[t] = totalLength;
		oldPt = pt;
	}
}

InspectorCmd* ACRSpline::generateDeleteItemCmd(GameInspector* inspector,
                                               const SelectedItem* items,
                                               int numItems,
                                               bool ifActorModeShouldDeleteActorsUnder) {
	if (numItems == 1 && items[0].editMode == editMode_actors) {
		return Actor::generateDeleteItemCmd(inspector, items, numItems, ifActorModeShouldDeleteActorsUnder);
	}

	std::vector<int> pointIndicesToDelete;
	for (int t = 0; t < numItems; ++t) {
		if_checked(items[t].editMode == editMode_points) { pointIndicesToDelete.push_back(items[t].index); }
	}

	ACRSplineDeletePoints* const cmd = new ACRSplineDeletePoints;
	cmd->setup(this, std::move(pointIndicesToDelete));
	return cmd;
}

InspectorCmd* ACRSpline::generateItemSetTransformCmd(
    GameInspector* inspector, EditMode const mode, int itemIndex, const transf3d& initalTrasform, const transf3d& newTransform) {
	if (mode == editMode_points) {
		ACRSplineMovePointCmd* cmd = new ACRSplineMovePointCmd;
		cmd->setup(getId(), itemIndex, initalTrasform.p, newTransform.p);
		return cmd;
	}

	return Actor::generateItemSetTransformCmd(inspector, mode, itemIndex, initalTrasform, newTransform);
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
void ACRSplineMovePointCmd::setup(ObjectId actorid, int pointIndex, const vec3f& originalPosition, const vec3f& newPosition) {
	m_actorid = actorid;
	m_pointIndex = pointIndex;
	m_originalPosition = originalPosition;
	m_newPosition = newPosition;
}

void ACRSplineMovePointCmd::apply(GameInspector* inspector) {
	Actor* const actor = inspector->m_world->getActorById(m_actorid);

	if (actor && actor->getType() == sgeTypeId(ACRSpline)) {
		ACRSpline* spline = (ACRSpline*)actor;

		if (m_pointIndex < spline->points.size()) {
			spline->points[m_pointIndex] = m_newPosition;
		} else {
			sgeAssert(false); // Should never happen.
		}
	} else {
		sgeAssert(false); // Should never happen.
	}
}

void ACRSplineMovePointCmd::redo(GameInspector* inspector) {
	apply(inspector);
}

void ACRSplineMovePointCmd::undo(GameInspector* inspector) {
	ACRSpline* const spline = inspector->m_world->getActor<ACRSpline>(m_actorid);

	if (spline) {
		if (m_pointIndex < spline->points.size()) {
			spline->points[m_pointIndex] = m_originalPosition;
		} else {
			sgeAssert(false); // Should never happen.
		}
	} else {
		sgeAssert(false); // Should never happen.
	}
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
void ACRSplineAddPoints::setup(ACRSpline* const spline, std::vector<int> pointsToTessalateBetween) {
	sgeAssert(spline);

	m_actorid = spline->getId();
	m_pointsToTessalateBetween = std::move(pointsToTessalateBetween);
	m_originalSplinePoints = spline->points;

	std::sort(m_pointsToTessalateBetween.begin(), m_pointsToTessalateBetween.end());
}

void ACRSplineAddPoints::apply(GameInspector* inspector) {
	ACRSpline* const spline = inspector->m_world->getActor<ACRSpline>(m_actorid);

	int offs = 0;

	if (spline) {
		for (int t = 0; t < m_pointsToTessalateBetween.size() - 1; ++t) {
			int const idx0 = m_pointsToTessalateBetween[t] + offs;
			int const idx1 = m_pointsToTessalateBetween[t + 1] + offs;

			// Tessalate if these are two consecutive points.
			if (idx0 + 1 == idx1) {
				vec3f const newPointPos = (spline->points[idx0] + spline->points[idx1]) * 0.5f;
				spline->points.insert(spline->points.begin() + idx1, newPointPos);
				++offs;
			}
		}
		spline->onMemberChanged();
	}
}

void ACRSplineAddPoints::redo(GameInspector* inspector) {
	apply(inspector);
}

void ACRSplineAddPoints::undo(GameInspector* inspector) {
	ACRSpline* const spline = inspector->m_world->getActor<ACRSpline>(m_actorid);
	if (spline) {
		spline->points = m_originalSplinePoints;
		spline->onMemberChanged();
	}
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
void ACRSplineDeletePoints::setup(ACRSpline* const spline, std::vector<int> indicesToDelete) {
	if_checked(spline) {
		m_indicesToDelete = std::move(indicesToDelete);
		m_actorid = spline->getId();
		std::sort(m_indicesToDelete.begin(), m_indicesToDelete.end(), [](int a, int b) { return a > b; });
		m_originalSplinePoints = spline->points;
	}
}

void ACRSplineDeletePoints::apply(GameInspector* inspector) {
	ACRSpline* const spline = inspector->m_world->getActor<ACRSpline>(m_actorid);
	if_checked(spline) {
		// CAUTION: Assumes that indices are sorted big to small
		for (int const idxToDel : m_indicesToDelete) {
			if_checked(idxToDel < spline->points.size()) { spline->points.erase(spline->points.begin() + idxToDel); }
		}
	}
}

#define if_asserted(expr) if (const bool v = (expr), sgeAssert(v), v)

void ACRSplineDeletePoints::redo(GameInspector* inspector) {
	apply(inspector);
}

void ACRSplineDeletePoints::undo(GameInspector* inspector) {
	ACRSpline* const spline = inspector->m_world->getActor<ACRSpline>(m_actorid);
	if_checked(spline) { spline->points = m_originalSplinePoints; }
}

} // namespace sge
