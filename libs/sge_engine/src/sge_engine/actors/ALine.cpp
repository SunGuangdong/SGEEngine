#include "ALine.h"

#include "sge_engine/GameInspector.h"

namespace sge {


// clang-format off
ReflAddTypeId(ALine, 20'03'02'0022);

ReflBlock() {
	ReflAddActor(ALine)
		ReflMember(ALine, points)
		ReflMember(ALine, isLooped)
	;

}
// clang-format on

//--------------------------------------------------------------------
// TraitPath3DForASpline
//--------------------------------------------------------------------
bool TraitPath3DForASpline::isEmpty() const {
	const Actor* a = getActor();
	if (a && a->getType() == sgeTypeId(ALine)) {
		const ALine* const spline = static_cast<const ALine*>(a);
		return spline->getNumPoints() == 0;
	}

	sgeAssert(false);
	return true;
}

float TraitPath3DForASpline::getTotalLength() {
	ALine* const spline = static_cast<ALine*>(getActor());
	if (spline) {
		return spline->getTotalLength();
	}

	return 0.f;
}

bool TraitPath3DForASpline::evaluateAtDistance(vec3f* outPosition, vec3f* outTanget, float const distance) {
	ALine* const spline = static_cast<ALine*>(getActor());
	if (spline) {
		return spline->evaluateAtDistance(outPosition, outTanget, distance);
	}

	return false;
}

//--------------------------------------------------------------------
// ALine
//--------------------------------------------------------------------
void ALine::create() {
	registerTrait(traitPath);
	registerTrait(m_traitViewportIcon);
	m_traitViewportIcon.setTexture("assets/editor/textures/icons/obj/ALine.png", true);

	points.clear();
	points.push_back(vec3f(0.f));
	points.push_back(vec3f(10.f, 0.f, 0.f));
}

Box3f ALine::getBBoxOS() const {
	Box3f bbox;

	for (const vec3f& pt : points)
		bbox.expand(pt);

	// Include the position of the object as well.
	bbox.expand(vec3f(0.f, 0.f, 0.f));

	return bbox;
}

void ALine::onMemberChanged() {
	makeDirty();
}

void ALine::computeSegmentsLength() {
	totalLength = 0.f;

	if (isEmpty()) {
		return;
	}

	const int numSegments = getNumSegments();
	segmentsLength.resize(numSegments, 0.f);
	for (int iSegment = 0; iSegment < numSegments; ++iSegment) {
		int i0, i1;
		if (getSegmentVerts(iSegment, i0, i1) == false) {
			sgeAssert(false);
			break;
		}

		const float len = (points[i1] - points[i0]).length();
		segmentsLength[iSegment] = len;
		totalLength += len;
	}
}

bool ALine::evaluateAtDistance(vec3f* outPosition, vec3f* outTanget, float distance) {
	if (isEmpty()) {
		return false;
	}

	if (segmentsLength.size() == 0) {
		computeSegmentsLength(); // hack.
		// sgeAssert(false); // Should never happen, but it does...
		return false;
	}

	if (segmentsBuiltDirtyIndex != getDirtyIndex()) {
		computeSegmentsLength();
		segmentsBuiltDirtyIndex = getDirtyIndex();
	}

	distance = clamp(distance, 0.f, getTotalLength());

	bool segmentFound = false;
	vec3f segmentVerts[2] = {vec3f(0.f)};
	float interpolationCoeffInSegment = 0.f;
	float distanceAccum = 0.f;
	const int numSegments = getNumSegments();
	for (int iSegment = 0; iSegment < numSegments; ++iSegment) {
		const float evalDistanceInLastSegment = distance - distanceAccum;
		const float lastSegmentDistance = segmentsLength[iSegment];

		if (evalDistanceInLastSegment >= 0.f && evalDistanceInLastSegment <= lastSegmentDistance) {
			int i0, i1;
			if (getSegmentVerts(iSegment, i0, i1) == false) {
				sgeAssert(false);
				break;
			}

			segmentVerts[0] = points[i0];
			segmentVerts[1] = points[i1];
			interpolationCoeffInSegment = lastSegmentDistance > 0.f ? evalDistanceInLastSegment / lastSegmentDistance : 0.f;
			segmentFound = true;
			break;
		}

		distanceAccum += lastSegmentDistance;
	}

	sgeAssert(segmentFound);

	if (outPosition != nullptr) {
		*outPosition = lerp(segmentVerts[0], segmentVerts[1], interpolationCoeffInSegment);
	}

	if (outTanget != nullptr) {
		*outTanget = segmentVerts[1] - segmentVerts[0];
	}

	return true;
}

InspectorCmd* ALine::generateDeleteItemCmd(GameInspector* inspector,
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

	ASplineDeletePoints* const cmd = new ASplineDeletePoints;
	cmd->setup(this, std::move(pointIndicesToDelete));
	return cmd;
}

InspectorCmd* ALine::generateItemSetTransformCmd(
    GameInspector* inspector, EditMode const mode, int itemIndex, const transf3d& initalTrasform, const transf3d& newTransform) {
	if (mode == editMode_points) {
		ASplineMovePointCmd* cmd = new ASplineMovePointCmd;
		cmd->setup(getId(), itemIndex, initalTrasform.p, newTransform.p);
		return cmd;
	}

	return Actor::generateItemSetTransformCmd(inspector, mode, itemIndex, initalTrasform, newTransform);
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
void ASplineMovePointCmd::setup(ObjectId actorid, int pointIndex, const vec3f& originalPosition, const vec3f& newPosition) {
	m_actorid = actorid;
	m_pointIndex = pointIndex;
	m_originalPosition = originalPosition;
	m_newPosition = newPosition;
}

void ASplineMovePointCmd::apply(GameInspector* inspector) {
	Actor* const actor = inspector->m_world->getActorById(m_actorid);

	if (actor && actor->getType() == sgeTypeId(ALine)) {
		ALine* spline = (ALine*)actor;

		if (m_pointIndex < spline->points.size()) {
			spline->points[m_pointIndex] = m_newPosition;
			spline->makeDirtyExternal();
		} else {
			sgeAssert(false); // Should never happen.
		}
	} else {
		sgeAssert(false); // Should never happen.
	}
}

void ASplineMovePointCmd::redo(GameInspector* inspector) {
	apply(inspector);
}

void ASplineMovePointCmd::undo(GameInspector* inspector) {
	ALine* const spline = inspector->m_world->getActor<ALine>(m_actorid);

	if (spline) {
		if (m_pointIndex < spline->points.size()) {
			spline->points[m_pointIndex] = m_originalPosition;
			spline->makeDirtyExternal();
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
void ASplineAddPoints::setup(ALine* const spline, std::vector<int> pointsToTessalateBetween) {
	sgeAssert(spline);

	m_actorid = spline->getId();
	m_pointsToTessalateBetween = std::move(pointsToTessalateBetween);
	m_originalSplinePoints = spline->points;

	std::sort(m_pointsToTessalateBetween.begin(), m_pointsToTessalateBetween.end());
}

void ASplineAddPoints::apply(GameInspector* inspector) {
	ALine* const spline = inspector->m_world->getActor<ALine>(m_actorid);
	if (spline == nullptr) {
		return;
	} else {
		false;
	}

	int offs = 0; // "How many times have we tessalated?", when we add a new point the even newer points have to be shifted.
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

void ASplineAddPoints::redo(GameInspector* inspector) {
	apply(inspector);
}

void ASplineAddPoints::undo(GameInspector* inspector) {
	ALine* const spline = inspector->m_world->getActor<ALine>(m_actorid);
	if (spline) {
		spline->points = m_originalSplinePoints;
	}
	spline->onMemberChanged();
}

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
void ASplineDeletePoints::setup(ALine* const spline, std::vector<int> indicesToDelete) {
	if_checked(spline) {
		m_indicesToDelete = std::move(indicesToDelete);
		m_actorid = spline->getId();
		std::sort(m_indicesToDelete.begin(), m_indicesToDelete.end(), [](int a, int b) { return a > b; });
		m_originalSplinePoints = spline->points;
	}
}

void ASplineDeletePoints::apply(GameInspector* inspector) {
	ALine* const spline = inspector->m_world->getActor<ALine>(m_actorid);
	if_checked(spline) {
		// CAUTION: Assumes that indices are sorted big to small
		for (int const idxToDel : m_indicesToDelete) {
			if_checked(idxToDel < spline->points.size()) { spline->points.erase(spline->points.begin() + idxToDel); }
		}
	}

	spline->onMemberChanged();
}

#define if_asserted(expr) if (const bool v = (expr), sgeAssert(v), v)

void ASplineDeletePoints::redo(GameInspector* inspector) {
	apply(inspector);
}

void ASplineDeletePoints::undo(GameInspector* inspector) {
	ALine* const spline = inspector->m_world->getActor<ALine>(m_actorid);
	if_checked(spline) { spline->points = m_originalSplinePoints; }

	spline->onMemberChanged();
}

} // namespace sge
