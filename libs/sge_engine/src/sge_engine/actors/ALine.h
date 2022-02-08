#pragma once
#include "sge_engine/Actor.h"
#include "sge_engine/traits/TraitPath.h"
#include "sge_engine/traits/TraitViewportIcon.h"
#include <string>

#include "sge_engine/InspectorCmds.h"

namespace sge {

//--------------------------------------------------------------------
// TraitPath3DForASpline
//--------------------------------------------------------------------
struct SGE_ENGINE_API TraitPath3DForASpline final : public TraitPath3D {
	SGE_TraitDecl_Final(TraitPath3DForASpline);

	bool isEmpty() const final;
	float getTotalLength() final;
	bool evaluateAtDistance(vec3f* outPosition, vec3f* outTanget, float const distance) final;
};

//--------------------------------------------------------------------
// ALine
//--------------------------------------------------------------------
struct SGE_ENGINE_API ALine : public Actor {
	std::vector<vec3f> points;
	bool isLooped = false;

	std::vector<float> segmentsLength;
	int segmentsBuiltDirtyIndex = 0;
	TraitPath3DForASpline traitPath;
	float totalLength;
	TraitViewportIcon m_traitViewportIcon;

	ALine() {}

	void create() final;
	Box3f getBBoxOS() const final;
	void onMemberChanged() final;

	void computeSegmentsLength();
	bool isEmpty() const { return int(points.empty()); }
	float getTotalLength() const { return totalLength; }
	bool evaluateAtDistance(vec3f* outPosition, vec3f* outTanget, float const distance);
	int getNumPoints() const { return int(points.size()); }

	int getNumSegments() const {
		const int pts = getNumPoints();
		if (pts <= 1) {
			return 0;
		}

		return (isLooped) ? pts : pts - 1;
	}

	bool getSegmentVerts(const int iSegment, int& i0, int& i1) const {
		const int numSegments = getNumSegments();

		if (iSegment >= numSegments || iSegment < 0) {
			sgeAssert(false);
			return false;
		}

		if (isLooped && iSegment == (numSegments - 1)) {
			i0 = getNumPoints() - 1;
			i1 = 0;
		} else {
			i0 = iSegment;
			i1 = iSegment + 1;
		}

		return true;
	}

	int getNumItemsInMode(EditMode const mode) const final {
		if (mode == editMode_points)
			return int(points.size());
		return Actor::getNumItemsInMode(mode);
	}

	bool getItemTransform(transf3d& result, EditMode const mode, int itemIndex) final {
		if (mode == editMode_points) {
			if (itemIndex > points.size())
				return false;

			result = transf3d::getIdentity();
			result.p = mat_mul_pos(getTransform().toMatrix(), points[itemIndex]);
			return true;
		}

		return Actor::getItemTransform(result, mode, itemIndex);
	}

	void setItemTransform(EditMode const mode, int itemIndex, const transf3d& tr) {
		if (mode == editMode_points) {
			if (itemIndex > points.size()) {
				sgeAssert(false);
				return;
			}
			points[itemIndex] = mat_mul_pos(getTransform().toMatrix().inverse(), tr.p);
			onMemberChanged();
		} else {
			Actor::setItemTransform(mode, itemIndex, tr);
		}
	}

	InspectorCmd* generateDeleteItemCmd(GameInspector* inspector,
	                                    const SelectedItem* items,
	                                    int numItems,
	                                    bool ifActorModeShouldDeleteActorsUnder) final;

	InspectorCmd* generateItemSetTransformCmd(
	    GameInspector* inspector, EditMode const mode, int itemIndex, const transf3d& initalTrasform, const transf3d& newTransform) final;
};

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
struct SGE_ENGINE_API ASplineMovePointCmd : public InspectorCmd {
	ObjectId m_actorid;
	int m_pointIndex;
	vec3f m_originalPosition;
	vec3f m_newPosition;

	void setup(ObjectId actorid, int pointIndex, const vec3f& originalPosition, const vec3f& newPosition);

	void apply(GameInspector* inspector) final;
	void redo(GameInspector* inspector) final;
	void undo(GameInspector* inspector) final;

	void getText(std::string& text) final { text = "Spline point moved"; }
};

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
struct SGE_ENGINE_API ASplineAddPoints : public InspectorCmd {
	ObjectId m_actorid;
	std::vector<int> m_pointsToTessalateBetween; // the edges between each consecutive points.
	std::vector<vec3f> m_originalSplinePoints;

	// Point indices as currently taken from the game inspector.
	void setup(ALine* const spline, std::vector<int> pointsToTessalateBetween);

	void apply(GameInspector* inspector) final;
	void redo(GameInspector* inspector) final;
	void undo(GameInspector* inspector) final;

	void getText(std::string& text) final { text = "Spline tessalated"; }
};

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
struct SGE_ENGINE_API ASplineDeletePoints : public InspectorCmd {
	ObjectId m_actorid;
	std::vector<int> m_indicesToDelete; // The indices of the points that are going to be deleted. MUST BE SORTED in GREATER TO SMALLER.
	std::vector<vec3f> m_originalSplinePoints;

	// Point indices as currently taken from the game inspector.
	void setup(ALine* const spline, std::vector<int> indicesToDelete);

	void apply(GameInspector* inspector) final;
	void redo(GameInspector* inspector) final;
	void undo(GameInspector* inspector) final;

	void getText(std::string& text) final { text = "Spline points deleted"; }
};

} // namespace sge