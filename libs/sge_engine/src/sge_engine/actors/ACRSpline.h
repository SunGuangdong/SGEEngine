#pragma once

#include <string>

#include "sge_engine/Actor.h"
#include "sge_engine/traits/TraitPath.h"
#include "sge_engine/traits/TraitViewportIcon.h"

#include "sge_engine/InspectorCmds.h"

namespace sge {

struct SGE_ENGINE_API TraitPath3DForACRSpline final : public TraitPath3D {
	SGE_TraitDecl_Final(TraitPath3DForACRSpline);

	bool isEmpty() const final;
	bool evaluateAtDistance(vec3f* outPosition, vec3f* outTanget, float distance) override;
	float getTotalLength() final;
};


//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
struct SGE_ENGINE_API ACRSpline : public Actor {
	std::vector<vec3f> points;
	float totalLength;
	std::vector<float> distanceSamples;
	TraitPath3DForACRSpline traitPath;
	TraitViewportIcon m_traitViewportIcon;

	ACRSpline() = default;

	void create() final;
	Box3f getBBoxOS() const final;
	void onMemberChanged() final;

	void getPointsForSegment(vec3f result[4], const int iSegment) const;
	void computeSegmentsLength();

	int getNumPoints() const { return int(points.size()); }

	bool isEmpty() const;
	bool evaluateAtDistance(vec3f* outPosition, vec3f* outTanget, float distance);
	bool evalute(vec3f* outPosition, vec3f* outTanget, float t);
	float getTotalLength() { return totalLength; }

	int getNumItemsInMode(EditMode const mode) const final
	{
		if (mode == editMode_points)
			return int(points.size());
		return Actor::getNumItemsInMode(mode);
	}

	bool getItemTransform(transf3d& result, EditMode const mode, int itemIndex) final
	{
		if (mode == editMode_points) {
			if (itemIndex > points.size()) {
				return false;
			}

			result = transf3d::getIdentity();
			result.p = mat_mul_pos(getTransform().toMatrix(), points[itemIndex]);
			return true;
		}

		return Actor::getItemTransform(result, mode, itemIndex);
	}

	void setItemTransform(EditMode const mode, int itemIndex, const transf3d& tr)
	{
		if (mode == editMode_points) {
			if (itemIndex > points.size()) {
				sgeAssert(false);
				return;
			}

			points[itemIndex] = mat_mul_pos(getTransform().toMatrix().inverse(), tr.p);
		}
		else {
			Actor::setItemTransform(mode, itemIndex, tr);
		}

		onMemberChanged();
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
struct SGE_ENGINE_API ACRSplineMovePointCmd : public InspectorCmd {
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
struct SGE_ENGINE_API ACRSplineAddPoints : public InspectorCmd {
	ObjectId m_actorid;
	std::vector<int> m_pointsToTessalateBetween; // the edges between each consecutive points.
	std::vector<vec3f> m_originalSplinePoints;

	// Point indices as currently taken from the game inspector.
	void setup(ACRSpline* const spline, std::vector<int> pointsToTessalateBetween);

	void apply(GameInspector* inspector) final;
	void redo(GameInspector* inspector) final;
	void undo(GameInspector* inspector) final;

	void getText(std::string& text) final { text = "Spline tessalated"; }
};

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
struct SGE_ENGINE_API ACRSplineDeletePoints : public InspectorCmd {
	ObjectId m_actorid;
	std::vector<int> m_indicesToDelete; // The indices of the points that are going to be deleted. MUST BE SORTED in GREATER TO SMALLER.
	std::vector<vec3f> m_originalSplinePoints;

	// Point indices as currently taken from the game inspector.
	void setup(ACRSpline* const spline, std::vector<int> indicesToDelete);

	void apply(GameInspector* inspector) final;
	void redo(GameInspector* inspector) final;
	void undo(GameInspector* inspector) final;

	void getText(std::string& text) final { text = "Spline points deleted"; }
};


} // namespace sge
