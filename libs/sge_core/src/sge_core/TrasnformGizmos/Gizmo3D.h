#pragma once

#include "sge_core/sgecore_api.h"
#include "sge_utils/math/Box3f.h"
#include "sge_utils/math/primitives.h"
#include "sge_utils/math/transform.h"
#include "sge_utils/sge_utils.h"

namespace sge {

struct InputState;

// TODO: Add a way to select if the gizmo should snap the added difference or the whole value.
struct SGE_CORE_API GizmoSnapSets {
	GizmoSnapSets() = default;
	GizmoSnapSets(const vec3f& translationSnapping, float const rotationSnapping, const vec3f& scaleSnapping)
	    : translationSnapping(translationSnapping)
	    , rotationSnapping(rotationSnapping)
	    , scaleSnapping(scaleSnapping)
	{
	}

	float applySnappingTranslation(const float v, int iAxis) const
	{
		iAxis = iAxis % 3; // Assuming the input is in SignedAxis enum convention, which has negatives as well.
		sgeAssert(iAxis >= 0 && iAxis < 3);
		if (translationSnapping[iAxis] == 0.f) {
			return v;
		}

		const float step = translationSnapping[iAxis];
		const float res = roundf(v / step) * step;
		return res;
	}

	vec3f applySnappingTranslation(vec3f v) const
	{
		return vec3f(applySnappingTranslation(v.x, 0), applySnappingTranslation(v.y, 1), applySnappingTranslation(v.z, 2));
	}

	// Snap at these values, 0 means no snapping.
	vec3f translationSnapping = vec3f(0.f);

	// Snap on these n*angle intervals, 0 means no snapping.
	float rotationSnapping = 0.f;

	// snap at these values
	vec3f scaleSnapping = vec3f(0.f);
};

struct SGE_CORE_API GizmoInteractArgs {
	GizmoInteractArgs() = default;
	GizmoInteractArgs(const InputState* const is, const GizmoSnapSets* snapSets, const Ray& rayWS, const float customScale)
	    : is(is)
	    , rayWS(rayWS)
	    , snapSets(snapSets)
	    , customScale(customScale)
	{
	}

	const InputState* is = nullptr;
	const GizmoSnapSets* snapSets = nullptr;
	Ray rayWS;
	float customScale; // Used to prevent the gizmo from scaling based on the rays origin. Useful for orthographic cameras. Ignored if 0.f
};


struct SGE_CORE_API GizmoInteractResult {
	GizmoInteractResult(bool isDone = false, bool userClickedAway = false)
	    : isDone(isDone)
	    , userClickedAway(userClickedAway)
	{
	}

	bool isDone;
	bool userClickedAway;
};


struct SGE_CORE_API GizmoActionMask {
	union {
		int mask;
		struct {
			unsigned x : 1;
			unsigned y : 1;
			unsigned z : 1;

			unsigned neg_x : 1; // Used only in volume scaling gizmo.
			unsigned neg_y : 1; // Used only in volume scaling gizmo.
			unsigned neg_z : 1; // Used only in volume scaling gizmo.
		};
	};

	GizmoActionMask()
	    : mask(0)
	{
	}

	bool only_x() const { return mask == 1; }
	bool only_y() const { return mask == 2; }
	bool only_z() const { return mask == 4; }

	bool only_neg_x() const { return mask == 8; }
	bool only_neg_y() const { return mask == 16; }
	bool only_neg_z() const { return mask == 32; }

	bool only_xy() const { return x == 1 && y == 1 && z == 0; }
	bool only_yz() const { return x == 0 && y == 1 && z == 1; }
	bool only_xz() const { return x == 1 && y == 0 && z == 1; }
	bool only_xyz() const { return x == 1 && y == 1 && z == 1; }

	void set(SignedAxis axis) { mask |= 1 << int(axis); }
	bool has(SignedAxis axis) const { return mask & (1 << int(axis)); }
	bool hasOnly(SignedAxis axis) const { return mask == (1 << int(axis)); }

	SignedAxis getSingleInteractionAxis() const
	{
		for (int iAxis = 0; iAxis < signedAxis_numElements; ++iAxis) {
			if (hasOnly(SignedAxis(iAxis))) {
				return SignedAxis(iAxis);
			}
		}

		sgeAssert(false);
		return axis_x_pos;
	}

	bool is_none() const { return mask == 0; }
};

// A set of common "things" that is shared between the gizmos.
struct SGE_CORE_API Gizmo3DCommon {
	// Changes how big the gizmo should appear on the screen. Size of 1 means "as big as the screen".
	void setSizeMultiplier(float const sizeMultiplier) { m_sizeMultiplier = sizeMultiplier; }

	// Resturns the userspecified scaling of the gizmo.
	float getSizeMultiplier() const { return m_sizeMultiplier; }

	// Returns the scale of the object (takeing into accunt the distance to the camera). Used for rendering.
	float getDisplayScale() const { return m_displayScale; }

	// Retrieves the mask that specifies what is concidered "hot" based on the user input.
	GizmoActionMask getActionMask() const { return m_interaction.mask ? m_interaction : m_hover; }

	// Returns true if the gizmo is pending for or processing user input.
	bool isInteracting() const { return m_interaction.mask != 0; }

  protected:
	void resetCommon()
	{
		m_interaction.mask = 0;
		m_hover.mask = 0;
		m_displayScale = m_sizeMultiplier; // TODO: Not sure if this is necessary.
	}

	// How big the gizmo shold appear on the screen. Size of 1 means "as big as the screen". 1 is not default as it is just too big.
	float m_sizeMultiplier = 0.15f;

	// The total scale that takes into accout the distance to the camera.
	// CAUTION: This is computed by the gizmo itself usually in the interact method.
	float m_displayScale = 1.f;

	GizmoActionMask m_interaction;
	GizmoActionMask m_hover;
};

//--------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------
struct SGE_CORE_API Gizmo3DTranslation : public Gizmo3DCommon {
	// The offset of the planar translation quad form the origin of the gizmo.
	static const float kQuadOffset;

	// The size of the planar translation quad.
	static const float kQuatLength;

  public:
	// Changes the gizmo state back to non-interacting, and places it at the specified inital position.
	void reset(const vec3f& initalTranslation, const quatf& initialOrientation);

	// The update function of the gizmo, returns true when the "user interaction" is complete.
	GizmoInteractResult interact(const GizmoInteractArgs& args);

	// Retrieves the currently edited value by the user.
	vec3f getEditedTranslation() const { return m_editedTranslation; }

	const vec3f* getAxes() const { return m_axes; }

  private:
	vec3f m_axes[3];
	vec3f m_initialTranslation; // The inital translation.
	vec3f m_editedTranslation;  // The current translation what was modified by the user.
	Ray m_interactionStartRayWS;
};

//-------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------
struct SGE_CORE_API Gizmo3DRotation : public Gizmo3DCommon {
	void reset(const vec3f& postion, const quatf& rotation);

	// The update function of the gizmo, returns true when the "user interaction" is complete.
	GizmoInteractResult interact(const GizmoInteractArgs& args);

	vec3f getInitialTranslation() const { return m_initialTranslation; }
	quatf getInitialRotation() const { return m_initalRotation; }

	// Retrieves the currently edited value by the user.
	quatf getEditedRotation() const { return m_editedRotation; }

	vec3f getLastInteractionEyePosition() const { return m_lastInteractionEyePosition; }

	vec3f getInteractionStartPointWS() const { return m_interactionStartHitWS; }

  private:
	vec3f m_initialTranslation = vec3f(0);
	quatf m_initalRotation = quatf::getIdentity();
	quatf m_editedRotation = quatf::getIdentity();

	vec3f m_interactionStartHitWS = vec3f(0.f);
	Plane m_intersectionPlane; // The plane the user input is measured.

	vec3f m_lastInteractionEyePosition = vec3f(0.f);
};

//-------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------
struct SGE_CORE_API Gizmo3DScale : public Gizmo3DCommon {
	// The distance where the "plane" scaling trapezoids should start;
	static const float kTrapezoidStart;

	// The distance where the "plane" scaling trapezoids should end;
	static const float kTrapezoidEnd;

  public:
	// Changes the gizmo state back to non-m_interaction, and places it at newTargetPosition.
	void reset(const vec3f& position, const quatf& rotation, const vec3f& scale);

	// The update function of the gizmo, returns true when the "user interaction" is complete.
	GizmoInteractResult interact(const GizmoInteractArgs& args);

	vec3f getInitialTranslation() const { return m_initialTranslation; }
	quatf getInitalRotation() const { return m_initalRotation; }

	// Retrieves the currently edited value by the user.
	vec3f getEditedScaling() const { return m_editedScaling; }

  private:
	vec3f m_initialTranslation;
	quatf m_initalRotation;
	vec3f m_initialScaling;

	vec3f m_editedScaling;

	vec3f m_interactionStartHitWS; // The position of the mouse cursor(in world space) when the gizmo interaction started(the mouse presso
	                               // on the gizmo).
	vec2f m_interactionStartHitPS; // The position of the mouse cursor when the interaction started in pixels.
	Plane m_interactionPlane;
};

//-------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------
struct SGE_CORE_API Gizmo3DScaleVolume : public Gizmo3DCommon {
	// Changes the gizmo state back to non-m_interaction, and places it at newTargetPosition.
	void reset(const transf3d& transform, const Box3f& bboxOs);

	// The update function of the gizmo, returns true when the "user interaction" is complete.
	GizmoInteractResult interact(const GizmoInteractArgs& args);

	transf3d getInitialTransform() const { return m_initalTransform; }
	Box3f getInitialBBoxOS() const { return m_initalBboxOs; }

	// Retrieves the currently edited value by the user.
	transf3d getEditedTrasform() const { return m_editedTransform; }

	float getHandleRadiusWS() const { return handleRadiusWs; }

  private:
	float handleRadiusWs = 0.f;

	transf3d m_initalTransform;
	Box3f m_initalBboxOs;

	transf3d m_editedTransform;
};

//-------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------
struct SGE_CORE_API Gizmo3D {
	enum Mode {
		Mode_ScaleVolume,
		Mode_Scaling,
		Mode_Rotation,
		Mode_Translation,
	};

	Gizmo3D()
	    : m_mode(Mode_Translation)
	    , m_initalTransform(transf3d::getIdentity())
	{
	}

	void setSizeMultipler(const float scalingCoefficient);

	// TODO: Implement local space transformations.
	// Resets the gizmo to it's inital state with the specified transformation.
	void reset(const Mode mode, const transf3d& tr, const Box3f& bbox);

	// The update function of the gizmo, returns true when the "user interaction" is complete.
	GizmoInteractResult interact(const GizmoInteractArgs& args);

	// Retieves the currently edited values.
	vec3f editScaling() const { return m_gizmoScaling.getEditedScaling(); }
	quatf editRotation() const { return m_gizmoRotation.getEditedRotation(); }
	vec3f editTranslation() const { return m_gizmoTranslation.getEditedTranslation(); }

	transf3d editTransform() const { return transf3d(editTranslation(), editRotation(), editScaling()); }
	const transf3d& getInitalTransform() const { return m_initalTransform; }

	Mode getMode() const { return m_mode; }

	const Gizmo3DTranslation& getGizmoTranslation() const { return m_gizmoTranslation; }
	const Gizmo3DRotation& getGizmoRotation() const { return m_gizmoRotation; }
	const Gizmo3DScale& getGizmoScale() const { return m_gizmoScaling; }
	const Gizmo3DScaleVolume& getGizmoScaleVolume() const { return m_gizmoScaleVolume; }

	transf3d getEditedTransform() const;

	// Returns the difference transform, form the inital moment to the currently edited.
	transf3d getTransformDiff() const;

	bool isInteracting() const;

  private:
	Mode m_mode; // The current mode of the gizmo.

	transf3d m_initalTransform;
	Gizmo3DTranslation m_gizmoTranslation;
	Gizmo3DRotation m_gizmoRotation;
	Gizmo3DScale m_gizmoScaling;
	Gizmo3DScaleVolume m_gizmoScaleVolume;
};

} // namespace sge
