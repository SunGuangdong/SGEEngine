#include "Gizmo3D.h"

#include "sge_core/application/input.h"
#include "sge_utils/math/common.h"

namespace sge {

//-------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------
// The offset of the planar translation quad form the origin of the gizmo.
const float Gizmo3DTranslation::kQuadOffset = 0.44f;

// The size of the planar translation quad.
const float Gizmo3DTranslation::kQuatLength = 0.22f;

void Gizmo3DTranslation::reset(const vec3f& initalTranslation, const quatf& initialOrientation) {
	resetCommon();
	m_initialTranslation = initalTranslation;

	mat4f orientationMtx = mat4f::getRotationQuat(initialOrientation);

	for (int t = 0; t < 3; ++t) {
		m_axes[t] = normalized(orientationMtx.data[t].xyz());
	}

	m_editedTranslation = initalTranslation;
}

GizmoInteractResult Gizmo3DTranslation::interact(const GizmoInteractArgs& args) {
	// Recompute the interaction mask only if we are not currently interacting with the gizmo.
	if (m_interaction.mask == 0) {
		float const distance2camera = distance(m_initialTranslation, args.rayWS.pos);
		m_displayScale = args.customScale != 0.f ? args.customScale * m_sizeMultiplier : distance2camera * m_sizeMultiplier;
	}

	// The quads on each plane [0] -> origin, [1] e1, [2], e2
	const vec3f xy_quad[3] = {m_initialTranslation + m_displayScale * kQuadOffset * (m_axes[0] + m_axes[1]),
	                          m_axes[0] * kQuatLength * m_displayScale, m_axes[1] * kQuatLength * m_displayScale};

	const vec3f yz_quad[3] = {m_initialTranslation + m_displayScale * kQuadOffset * (m_axes[1] + m_axes[2]),
	                          m_axes[1] * kQuatLength * m_displayScale, m_axes[2] * kQuatLength * m_displayScale};

	const vec3f xz_quad[3] = {m_initialTranslation + m_displayScale * kQuadOffset * (m_axes[0] + m_axes[2]),
	                          m_axes[0] * kQuatLength * m_displayScale, m_axes[2] * kQuatLength * m_displayScale};

	const float axisHitRadius = 0.03f * m_displayScale;

	m_hover.mask = 0;

	float t = FLT_MAX;

	if (t == FLT_MAX) {
		t = IntersectRayQuad(args.rayWS, xy_quad[0], xy_quad[1], xy_quad[2]);
		if (t != FLT_MAX) {
			m_hover.x = 1;
			m_hover.y = 1;
		}
	}

	if (t == FLT_MAX) {
		t = IntersectRayQuad(args.rayWS, yz_quad[0], yz_quad[1], yz_quad[2]);
		if (t != FLT_MAX) {
			m_hover.y = 1;
			m_hover.z = 1;
		}
	}

	if (t == FLT_MAX) {
		t = IntersectRayQuad(args.rayWS, xz_quad[0], xz_quad[1], xz_quad[2]);
		if (t != FLT_MAX) {
			m_hover.x = 1;
			m_hover.z = 1;
		}
	}

	float lerp;
	const float ditanceX = getRaySegmentDistance(args.rayWS.pos, args.rayWS.dir, vec3f(m_initialTranslation),
	                                             m_initialTranslation + m_axes[0] * m_displayScale, &lerp);
	const float ditanceY = getRaySegmentDistance(args.rayWS.pos, args.rayWS.dir, vec3f(m_initialTranslation),
	                                             m_initialTranslation + m_axes[1] * m_displayScale, &lerp);
	const float ditanceZ = getRaySegmentDistance(args.rayWS.pos, args.rayWS.dir, vec3f(m_initialTranslation),
	                                             m_initialTranslation + m_axes[2] * m_displayScale, &lerp);
	if (t == FLT_MAX && ditanceX <= axisHitRadius) {
		m_hover.x = 1;
		t = lerp;
	}
	if (t == FLT_MAX && ditanceY <= axisHitRadius) {
		m_hover.y = 1;
		t = lerp;
	}
	if (t == FLT_MAX && ditanceZ <= axisHitRadius) {
		m_hover.z = 1;
		t = lerp;
	}

	// Check if the ray intersects with out gizmo model.
	if (args.is->IsKeyPressed(Key_MouseLeft) && args.is->wasActiveWhilePolling()) {
		if (m_hover.mask == 0) {
			// The user clicked away for the gizmo.
			return GizmoInteractResult(true, true);
		}

		m_interaction = m_hover;
		m_interactionStartRayWS = args.rayWS;

		return GizmoInteractResult(false, false);
	}

	if (args.is->IsKeyDown(Key_MouseLeft) && args.is->wasActiveWhilePolling() && (m_interaction.mask != 0)) {
		// Pick an intersection plane and compute the intersection.
		Plane plane;

		if (m_interaction.only_xy())
			plane.setNormal(m_axes[2]);
		else if (m_interaction.only_yz())
			plane.setNormal(m_axes[0]);
		else if (m_interaction.only_xz())
			plane.setNormal(m_axes[1]);
		else if (m_interaction.only_x()) {
			plane.setNormal(fabsf(dot(m_axes[1], args.rayWS.dir)) > fabsf(dot(m_axes[2], args.rayWS.dir)) ? m_axes[1] : m_axes[2]);
		} else if (m_interaction.only_y()) {
			plane.setNormal(fabsf(dot(m_axes[0], args.rayWS.dir)) > fabsf(dot(m_axes[2], args.rayWS.dir)) ? m_axes[0] : m_axes[2]);
		} else if (m_interaction.only_z()) {
			plane.setNormal(fabsf(dot(m_axes[0], args.rayWS.dir)) > fabsf(dot(m_axes[1], args.rayWS.dir)) ? m_axes[0] : m_axes[1]);
		}

		plane.setDistance(-dot(plane.norm(), m_initialTranslation));

		// Intersect the rays with the plane.
		float const tStart = -(plane.d() + dot(m_interactionStartRayWS.pos, plane.norm())) / dot(plane.norm(), m_interactionStartRayWS.dir);
		float const tNow = -(plane.d() + dot(args.rayWS.pos, plane.norm())) / dot(plane.norm(), args.rayWS.dir);

		vec3f const ptIntersectStart = m_interactionStartRayWS.Sample(tStart);
		vec3f const ptIntersectNow = args.rayWS.Sample(tNow);
		vec3f const diff = ptIntersectNow - ptIntersectStart;

		float const xDiff = dot(m_axes[0], diff);
		float const yDiff = dot(m_axes[1], diff);
		float const zDiff = dot(m_axes[2], diff);

		auto const snapValue = [](float value, float snap) -> float {
			if (snap == 0.f)
				return value;

			float fSnapCount = value / snap;
			return roundf(fSnapCount) * snap;
		};

		m_editedTranslation = m_initialTranslation;

		if (m_interaction.x) {
			m_editedTranslation += m_axes[0] * xDiff;
			m_editedTranslation.x =
			    args.snapSets ? args.snapSets->applySnappingTranslation(m_editedTranslation.x, 0) : m_editedTranslation.x;
		}
		if (m_interaction.y) {
			m_editedTranslation += m_axes[1] * yDiff;
			m_editedTranslation.y =
			    args.snapSets ? args.snapSets->applySnappingTranslation(m_editedTranslation.y, 1) : m_editedTranslation.y;
		}
		if (m_interaction.z) {
			m_editedTranslation += m_axes[2] * zDiff;
			m_editedTranslation.z =
			    args.snapSets ? args.snapSets->applySnappingTranslation(m_editedTranslation.z, 2) : m_editedTranslation.z;
		}

		return GizmoInteractResult(false, false);
	}

	if (args.is->IsKeyReleased(Key_MouseLeft) && args.is->wasActiveWhilePolling() && (m_interaction.mask != 0)) {
		// Done
		return GizmoInteractResult(true, false);
	}

	return GizmoInteractResult(false, false);
}


//-------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------
void Gizmo3DRotation::reset(const vec3f& postion, const quatf& rotation) {
	resetCommon();
	m_initialTranslation = postion;
	m_initalRotation = rotation;
	m_editedRotation = rotation;
}

GizmoInteractResult Gizmo3DRotation::interact(const GizmoInteractArgs& args) {
	m_lastInteractionEyePosition = args.rayWS.pos;

	// Recompute the interaction mask only if we are not currently m_interaction with the gizmo.
	// If we do for example the translation wont feel natural.
	if (m_interaction.is_none()) {
		float const distance2camera = distance(m_initialTranslation, args.rayWS.pos);
		m_displayScale = args.customScale != 0.f ? args.customScale * m_sizeMultiplier : distance2camera * m_sizeMultiplier;
	}

	// NOTE: we assume that rayWS.pos is the camera position.
	// Scale down the ray, because out gizmo is scaled by overscale.
	const mat4f gizmoTransf = mat4f::getTRS(m_initialTranslation, m_editedRotation, vec3f(m_displayScale));
	const mat4f gizmoTransfInv = inverse(gizmoTransf);

	Ray ray;
	ray.dir = (gizmoTransfInv * vec4f(args.rayWS.dir, 0.f)).xyz().normalized();
	ray.pos = (gizmoTransfInv * vec4f(args.rayWS.pos, 1.f)).xyz();

	m_hover.mask = 0;

	// Find the intersection point with the "gizmo's sphere".
	// We actually intersect the picking ray with X, Y, Z planes, and see which intersection
	// is closest to the circle (with r = 1) in the same plane.
	{
		// The min distance to the arc in order to concider it a click on the arc.
		const float infl = 0.1f;

		vec3f const px = ray.Sample(intersectRayPlane(ray.pos, ray.dir, vec3f::getAxis(0), 0.f));
		vec3f const py = ray.Sample(intersectRayPlane(ray.pos, ray.dir, vec3f::getAxis(1), 0.f));
		vec3f const pz = ray.Sample(intersectRayPlane(ray.pos, ray.dir, vec3f::getAxis(2), 0.f));

		// Calculated the vector between the intersection point and the circle (with r = 1) in the same plane.
		vec3f const nx = (px - px.normalized0());
		vec3f const ny = (py - py.normalized0());
		vec3f const nz = (pz - pz.normalized0());

		// Filter only to use the front arcs.
		float dx = dot(ray.pos, px) > 0.f ? nx.length() : FLT_MAX;
		float dy = dot(ray.pos, py) > 0.f ? ny.length() : FLT_MAX;
		float dz = dot(ray.pos, pz) > 0.f ? nz.length() : FLT_MAX;


		if (dx > infl && dy > infl && dz > infl) {
			m_hover.mask = 0;
		} else {
			if (dx < dy && dx < dz)
				m_hover.x = 1;
			else if (dy < dz)
				m_hover.y = 1;
			else
				m_hover.z = 1;
		}
	}

	// Check if the ray intersects with the gizmo.
	if (args.is->IsKeyPressed(Key_MouseLeft) && args.is->wasActiveWhilePolling()) {
		if (m_hover.is_none()) {
			// The use clicked away for the gizmo, there for interaction is canceled.
			return GizmoInteractResult(true, true);
		}

		m_interaction = m_hover;

		const int interactionAxisIdx = [&]() -> int {
			if (m_interaction.only_x())
				return 0;
			if (m_interaction.only_y())
				return 1;
			if (m_interaction.only_z())
				return 2;

			// Something went wrong, currently ony axial rotations are supported by the gizmo.
			sgeAssert(false);
			return -1;
		}();

		if (interactionAxisIdx == -1) {
			return GizmoInteractResult(true, true);
		}

		const vec3f planeAxis = normalized((gizmoTransf * vec4f::getAxis(interactionAxisIdx)).xyz());
		m_intersectionPlane = Plane::FromPosAndDir(m_initialTranslation, planeAxis);

		const float t =
		    -(m_intersectionPlane.d() + dot(args.rayWS.pos, m_intersectionPlane.norm())) / dot(m_intersectionPlane.norm(), args.rayWS.dir);
		m_interactionStartHitWS = args.rayWS.pos + t * args.rayWS.dir;

		return GizmoInteractResult(false, false);
	}

	if (args.is->IsKeyDown(Key_MouseLeft) && args.is->wasActiveWhilePolling() && !m_interaction.is_none()) {
		// Pick an intersection plane and compute the intersection.
		const float t =
		    -(m_intersectionPlane.d() + dot(args.rayWS.pos, m_intersectionPlane.norm())) / dot(m_intersectionPlane.norm(), args.rayWS.dir);
		const vec3f intersectionPoint = args.rayWS.pos + args.rayWS.dir * t;

		const vec3f baseDir = normalized(m_interactionStartHitWS - m_initialTranslation);
		const vec3f tarDir = normalized(intersectionPoint - m_initialTranslation);

		const float d = dot(baseDir, tarDir);
		const float c = cross(baseDir, tarDir).lengthSqr();
		const float sign = dot(m_intersectionPlane.norm(), cross(baseDir, tarDir)) > 0.f ? 1.f : -1.f;

		float angle = atan2(c, d) * sign;

		// Snapping.
		if (args.snapSets && args.snapSets->rotationSnapping != 0.f) {
			// Normalize the angle because if the we are going to make a jump between positive to negavtive angle (or vice versa)
			// we will skip the 0 inbetween because of float->int conversion.
			angle = normalizeAngle(angle);
			float const n = roundf(angle / args.snapSets->rotationSnapping);
			angle = n * args.snapSets->rotationSnapping;
		}

		m_editedRotation = quatf::getAxisAngle(m_intersectionPlane.norm(), angle) * m_initalRotation;

		return GizmoInteractResult(false, false);
	}

	if (args.is->IsKeyReleased(Key_MouseLeft) && args.is->wasActiveWhilePolling() && !m_interaction.is_none()) {
		// Done
		return GizmoInteractResult(true, false);
	}

	return GizmoInteractResult(false, false);
}

//-------------------------------------------------------------------------------
// Gizmo3DScale
//-------------------------------------------------------------------------------
const float Gizmo3DScale::kTrapezoidStart = 0.33f;
const float Gizmo3DScale::kTrapezoidEnd = 0.44f;

void Gizmo3DScale::reset(const vec3f& position, const quatf& rotation, const vec3f& scale) {
	resetCommon();
	m_initialTranslation = position;
	m_initalRotation = rotation;
	m_initialScaling = scale;
	m_editedScaling = scale;
}

GizmoInteractResult Gizmo3DScale::interact(const GizmoInteractArgs& args) {
	// Recompute the interaction mask only if we are not currently m_interaction with the gizmo.
	// If we do for example the translation wont feel natural.
	if (m_interaction.mask == 0) {
		float const distance2camera = distance(m_initialTranslation, args.rayWS.pos);
		m_displayScale = args.customScale != 0.f ? args.customScale * m_sizeMultiplier : distance2camera * m_sizeMultiplier;
	}

	// NOTE: we assume that rayWS.pos is the camera position.
	// Scale down the ray, because out gizmo is scaled by overscale.
	const mat4f gizmoTransf = mat4f::getTRS(m_initialTranslation, m_initalRotation, vec3f(m_displayScale));
	const mat4f gizmoTransfInv = inverse(gizmoTransf);

	Ray ray;
	ray.dir = (gizmoTransfInv * vec4f(args.rayWS.dir, 0.f)).xyz().normalized();
	ray.pos = (gizmoTransfInv * vec4f(args.rayWS.pos, 1.f)).xyz();

	auto intersectTrapezod = [&](const vec3f& ax0, const vec3f& ax1) -> bool {
		const vec3f tr0[3] = {ax0 * kTrapezoidStart, ax0 * kTrapezoidEnd, ax1 * kTrapezoidEnd};
		const vec3f tr1[3] = {ax1 * kTrapezoidStart, ax1 * kTrapezoidEnd, ax0 * kTrapezoidStart};

		return IntersectRayTriangle(ray, tr0) != FLT_MAX || IntersectRayTriangle(ray, tr1) != FLT_MAX;
	};

	const vec3f triScaleAll0[3] = {vec3f(0.f), kTrapezoidStart * vec3f::getAxis(1), kTrapezoidStart * vec3f::getAxis(2)};
	const vec3f triScaleAll1[3] = {vec3f(0.f), kTrapezoidStart * vec3f::getAxis(0), kTrapezoidStart * vec3f::getAxis(2)};
	const vec3f triScaleAll2[3] = {vec3f(0.f), kTrapezoidStart * vec3f::getAxis(0), kTrapezoidStart * vec3f::getAxis(1)};

	bool bCollisionAllScale = IntersectRayTriangle(ray, triScaleAll0) != FLT_MAX || IntersectRayTriangle(ray, triScaleAll1) != FLT_MAX ||
	                          IntersectRayTriangle(ray, triScaleAll2) != FLT_MAX;

	m_hover.mask = 0;

	if (bCollisionAllScale) {
		m_hover.x = 1;
		m_hover.y = 1;
		m_hover.z = 1;
	}

	// Check the trapezods.
	if (m_hover.mask == 0 && intersectTrapezod(vec3f::getAxis(1), vec3f::getAxis(2))) {
		m_hover.y = 1;
		m_hover.z = 1;
	}
	if (m_hover.mask == 0 && intersectTrapezod(vec3f::getAxis(0), vec3f::getAxis(2))) {
		m_hover.x = 1;
		m_hover.z = 1;
	}
	if (m_hover.mask == 0 && intersectTrapezod(vec3f::getAxis(0), vec3f::getAxis(1))) {
		m_hover.x = 1;
		m_hover.y = 1;
	}

	// Check the lines.
	if (m_hover.mask == 0 && getRaySegmentDistance(ray.pos, ray.dir, vec3f(0), vec3f::getAxis(0)) < 0.03)
		m_hover.x = 1;
	if (m_hover.mask == 0 && getRaySegmentDistance(ray.pos, ray.dir, vec3f(0), vec3f::getAxis(1)) < 0.03)
		m_hover.y = 1;
	if (m_hover.mask == 0 && getRaySegmentDistance(ray.pos, ray.dir, vec3f(0), vec3f::getAxis(2)) < 0.03)
		m_hover.z = 1;

	if (args.is->IsKeyPressed(Key_MouseLeft) && args.is->wasActiveWhilePolling()) {
		if (m_hover.mask == 0) {
			return GizmoInteractResult(true, true);
		}

		m_interaction.mask = m_hover.mask;
		const mat4f gizmoOrientation = mat4f::getRotationQuat(m_initalRotation);

		// Pick an intersection plane and compute the intersection.
		if (m_interaction.x && m_interaction.y && m_interaction.z)
			m_interactionPlane.setNormal(-args.rayWS.dir);
		else if (m_interaction.x && m_interaction.y)
			m_interactionPlane.setNormal(vec3f::getAxis(2));
		else if (m_interaction.y && m_interaction.z)
			m_interactionPlane.setNormal(vec3f::getAxis(0));
		else if (m_interaction.x && m_interaction.z)
			m_interactionPlane.setNormal(vec3f::getAxis(1));
		else if (m_interaction.x) {
			m_interactionPlane.setNormal(vec3f::getAxis(1));
		} else if (m_interaction.y) {
			// Pick the most "perpendicular" axis for better projection.
			m_interactionPlane.setNormal(vec3f::getAxis((ray.dir.z < ray.dir.x) ? 2 : 0));
		} else if (m_interaction.z) {
			m_interactionPlane.setNormal(vec3f::getAxis(1));
		}

		m_interactionPlane.setDistance(-dot(m_interactionPlane.norm(), m_initialTranslation));

		// Intersec the ray with the plane.
		float const t =
		    -(m_interactionPlane.d() + dot(args.rayWS.pos, m_interactionPlane.norm())) / dot(m_interactionPlane.norm(), args.rayWS.dir);
		vec3f const ptIntersect = args.rayWS.pos + t * args.rayWS.dir;

		m_interactionStartHitWS = ptIntersect;
		m_interactionStartHitPS = args.is->GetCursorPos();
	} else if (args.is->IsKeyDown(Key_MouseLeft) && args.is->wasActiveWhilePolling()) {
		int numInteractingAxes = 0;

		if (m_interaction.x)
			numInteractingAxes += 1;
		if (m_interaction.y)
			numInteractingAxes += 1;
		if (m_interaction.z)
			numInteractingAxes += 1;


		float const t =
		    -(m_interactionPlane.d() + dot(args.rayWS.pos, m_interactionPlane.norm())) / dot(m_interactionPlane.norm(), args.rayWS.dir);
		vec3f const ptIntersect = args.rayWS.pos + t * args.rayWS.dir;
		vec3f n = ptIntersect - m_initialTranslation;
		vec3f i = m_interactionStartHitWS - m_initialTranslation;

		n = m_initalRotation.transformDir(n);
		i = m_initalRotation.transformDir(i);

		if (numInteractingAxes == 1) {
			float kDiff = dot(n, i.normalized0()) / i.length();

			if (m_interaction.x) {
				m_editedScaling.x = kDiff * m_initialScaling.x;
			}
			if (m_interaction.y) {
				m_editedScaling.y = kDiff * m_initialScaling.y;
			}
			if (m_interaction.z) {
				m_editedScaling.z = kDiff * m_initialScaling.z;
			}
		} else if (numInteractingAxes == 2) {
			const vec3f currDir = ptIntersect - m_initialTranslation;
			const vec3f initDir = m_interactionStartHitWS - m_initialTranslation;
			const vec3f prjDir = normalized(initDir);

			const float prj0 = dot(prjDir, initDir);
			const float prj1 = dot(prjDir, currDir);

			const float scaling = prj1 - prj0;

			if (m_interaction.x)
				m_editedScaling.x = m_initialScaling.x + scaling * m_initialScaling.x;
			if (m_interaction.y)
				m_editedScaling.y = m_initialScaling.y + scaling * m_initialScaling.y;
			if (m_interaction.z)
				m_editedScaling.z = m_initialScaling.z + scaling * m_initialScaling.z;

		} else if (numInteractingAxes == 3) {
			const vec2f diff = args.is->GetCursorPos() - m_interactionStartHitPS;
			const float overallScaling = (-diff.y) / 100.f;

			m_editedScaling = m_initialScaling + overallScaling * m_initialScaling;
		}

		// Apply snapping.
		if (args.snapSets != nullptr) {
			auto const snapValue = [](float value, float snap) -> float {
				if (snap == 0.f)
					return value;
				float fSnapCount = value / snap;

				// Avoid snapping to 0 as this will make the object flat or nonexistant.
				if (fSnapCount == 0.f)
					return value;
				return roundf(fSnapCount) * snap;
			};

			m_editedScaling.x = snapValue(m_editedScaling.x, args.snapSets->scaleSnapping.x);
			m_editedScaling.y = snapValue(m_editedScaling.y, args.snapSets->scaleSnapping.y);
			m_editedScaling.z = snapValue(m_editedScaling.z, args.snapSets->scaleSnapping.z);
		}
	}

	if (args.is->IsKeyReleased(Key_MouseLeft) && args.is->wasActiveWhilePolling() && m_interaction.mask != 0) {
		// Done
		return GizmoInteractResult(true, false);
	}

	return GizmoInteractResult(false, false);
}

//-------------------------------------------------------------------------------
// Gizmo3DScaleVolume
//-------------------------------------------------------------------------------
void Gizmo3DScaleVolume::reset(const transf3d& transform, const Box3f& bboxOs) {
	resetCommon();
	m_initalTransform = transform;
	m_editedTransform = transform;
	m_initalBboxOs = bboxOs;
}

GizmoInteractResult Gizmo3DScaleVolume::interact(const GizmoInteractArgs& args) {
	handleRadiusWs = getInitialBBoxOS().getTransformed(getEditedTrasform().getSelfNoRotation().toMatrix()).size().componentMinAbs() * 0.2f;

	// The display scale is not used for this gizmo as it is tied to the size of the bounding box.
	m_displayScale = 1.f;

	vec3f facesCentersOS[signedAxis_numElements] = {vec3f(0.f)};
	m_initalBboxOs.getFacesCenters(facesCentersOS);

	vec3f facesCentersWS[signedAxis_numElements] = {vec3f(0.f)};
	for (int iAxis = 0; iAxis < signedAxis_numElements; iAxis++) {
		facesCentersWS[iAxis] = mat_mul_pos(m_initalTransform.toMatrix(), facesCentersOS[iAxis]);
	}

	m_hover.mask = 0;

	// Find the hovered axis.
	float intersection_t = FLT_MAX;
	SignedAxis interactionAxis = signedAxis_numElements;
	for (int iAxis = 0; iAxis < signedAxis_numElements; iAxis++) {
		const Sphere s(facesCentersWS[iAxis], handleRadiusWs);
		float t0, t1;

		// TODO: intesect with a box or a quad instead of a sphere
		if (intersectRaySphere(args.rayWS, s, t0, t1)) {
			float t = minOf(t0, t1);
			if (t < intersection_t) {
				intersection_t = t;
				interactionAxis = SignedAxis(iAxis);
			}
		}
	}

	if (intersection_t != FLT_MAX) {
		m_hover.set(interactionAxis);
	}

	if (args.is->IsKeyPressed(Key_MouseLeft) && args.is->wasActiveWhilePolling()) {
		if (m_hover.mask == 0) {
			// The user clicked away for the gizmo.
			return GizmoInteractResult(true, true);
		}

		m_interaction = m_hover;
	} else if (args.is->IsKeyDown(Key_MouseLeft) && args.is->wasActiveWhilePolling()) {
		vec3f bboxFacesNormalsOs[signedAxis_numElements];
		m_initalBboxOs.getFacesNormals(bboxFacesNormalsOs);
		const vec3f faceNormalOS = bboxFacesNormalsOs[m_interaction.getSingleInteractionAxis()];
		const vec3f faceNormalWs = mat_mul_dir(m_initalTransform.toMatrix(), faceNormalOS);
		const vec3f planePosWs = facesCentersWS[m_interaction.getSingleInteractionAxis()];

		// Using the interaction handle, which is defined by a position and facing direction, compute the nearest point
		// from the ray casted by the mouse the the handle ray.
		// the resulting point will be used as the new position of the modified face.
		float nearestT = 0.f;
		getRaySegmentDistance(args.rayWS.pos, args.rayWS.dir, planePosWs, planePosWs + faceNormalWs * 1000.f, &nearestT);
		const vec3f hitPointOnPlaneWs = args.rayWS.Sample(nearestT);

		const vec3f pointOnFaceNormalWs = planePosWs + faceNormalWs * projectPointOnLine(planePosWs, faceNormalWs, hitPointOnPlaneWs);
		const vec3f pointOnFaceNormalOs = mat_mul_pos(m_initalTransform.toMatrix().inverse(), pointOnFaceNormalWs);

		// Compute the new scaling needed. Note that we ignore the rotation of the object.
		// this is simply done because it is easier to compute the scaling (no axes as swapped or in akward angles that we would need to
		// compansate to) and Box3f represents (as the name suggests) axis-aligned bounding.
		const transf3d initalTrNoRot = m_initalTransform.getSelfNoRotation();
		const Box3f initBoxWSNoRot = m_initalBboxOs.getTransformed(initalTrNoRot.toMatrix());

		const vec3f pointOnFaceNormalWsNoRot = mat_mul_pos(initalTrNoRot.toMatrix(), pointOnFaceNormalOs);

		// Compute the new location (along the picked axis in world-space-with-no-rotation).
		float faceNewPosWsNoRot = dot(pointOnFaceNormalWsNoRot, vec3f(fabsf(faceNormalOS.x), fabsf(faceNormalOS.y), fabsf(faceNormalOS.z)));
		if (args.snapSets) {
			faceNewPosWsNoRot = args.snapSets->applySnappingTranslation(faceNewPosWsNoRot, m_interaction.getSingleInteractionAxis());
		}

		// Move the face of the box.
		Box3f editedBoxWsNoRot = initBoxWSNoRot;
		editedBoxWsNoRot = editedBoxWsNoRot.movedFaceTo(m_interaction.getSingleInteractionAxis(), faceNewPosWsNoRot);

		// Compute the new scaling.
		const vec3f newScaling = editedBoxWsNoRot.size() / m_initalBboxOs.size();

		// Compute the new translation, because we want to move only the selected face of the bounding box while,
		// maintaining the position of the opposite face of the box. We need to also introduce translation.
		// The translation is basically "where should the origin of the bbox in object space (which is (0,0,0)) should be after scaling.
		// We take where, as a fraction, that origin lies in the box, and the using this fraction compute it relative to the newly scaled
		// box.
		const vec3f originFrac = (vec3f(0.f) - m_initalBboxOs.min) / m_initalBboxOs.size();
		const vec3f newOriginWsNoRot = editedBoxWsNoRot.size() * originFrac + editedBoxWsNoRot.min;
		const vec3f newOriginWs = quat_mul_pos(m_initalTransform.r, newOriginWsNoRot - m_initalTransform.p) + m_initalTransform.p;

		m_editedTransform = m_initalTransform;
		m_editedTransform.p = newOriginWs;
		m_editedTransform.s = newScaling;
	}

	if (args.is->IsKeyReleased(Key_MouseLeft) && args.is->wasActiveWhilePolling() && (m_interaction.mask != 0)) {
		// Done
		return GizmoInteractResult(true, false);
	}

	return GizmoInteractResult();
}

//-------------------------------------------------------------------------------
// Gizmo3D
//-------------------------------------------------------------------------------
void Gizmo3D::setSizeMultipler(const float scalingCoefficient) {
	m_gizmoTranslation.setSizeMultiplier(scalingCoefficient);
	m_gizmoRotation.setSizeMultiplier(scalingCoefficient);
	m_gizmoScaling.setSizeMultiplier(scalingCoefficient);
}

void Gizmo3D::reset(const Mode newMode, const transf3d& tr, const Box3f& bbox) {
	m_initalTransform = tr;

	m_mode = newMode;
	m_gizmoTranslation.reset(tr.p, quatf::getIdentity());
	m_gizmoRotation.reset(tr.p, tr.r);
	m_gizmoScaling.reset(tr.p, tr.r, tr.s);
	m_gizmoScaleVolume.reset(tr, bbox);
}

GizmoInteractResult Gizmo3D::interact(const GizmoInteractArgs& args) {
	switch (m_mode) {
		case Mode_Translation: {
			return m_gizmoTranslation.interact(args);
		} break;
		case Mode_Rotation: {
			return m_gizmoRotation.interact(args);
		} break;
		case Mode_Scaling: {
			return m_gizmoScaling.interact(args);
		};
		case Mode_ScaleVolume: {
			return m_gizmoScaleVolume.interact(args);
		};
		default:
			sgeAssert(false); // Not implemented.
	}

	return true;
}

transf3d Gizmo3D::getEditedTransform() const {
	transf3d result = m_initalTransform;

	switch (m_mode) {
		case Mode_Translation: {
			result.p = m_gizmoTranslation.getEditedTranslation();
		} break;
		case Mode_Rotation: {
			result.r = m_gizmoRotation.getEditedRotation();
		} break;
		case Mode_Scaling: {
			result.s = m_gizmoScaling.getEditedScaling();
		} break;
		case Mode_ScaleVolume: {
			result = m_gizmoScaleVolume.getEditedTrasform();
		} break;
		default:
			sgeAssert(false); // Not implemented.
	}

	return result;
}

transf3d Gizmo3D::getTransformDiff() const {
	transf3d diff = transf3d::getIdentity();

	switch (m_mode) {
		case Mode_Translation: {
			diff.p = m_gizmoTranslation.getEditedTranslation() - m_initalTransform.p;
		} break;
		case Mode_Rotation: {
			diff.r = normalized(m_gizmoRotation.getEditedRotation() * conjugate(m_initalTransform.r));
		} break;
		case Mode_Scaling: {
			diff.s = m_gizmoScaling.getEditedScaling() / m_initalTransform.s;
		} break;
		case Mode_ScaleVolume: {
			diff.p = m_gizmoScaleVolume.getEditedTrasform().p - m_initalTransform.p;
			diff.s = m_gizmoScaleVolume.getEditedTrasform().s / m_initalTransform.s;
		} break;
		default:
			sgeAssert(false); // Not implemented
	}

	return diff;
}

bool Gizmo3D::isInteracting() const {
	switch (m_mode) {
		case Mode_Translation: {
			return m_gizmoTranslation.isInteracting();
		} break;
		case Mode_Rotation: {
			return m_gizmoRotation.isInteracting();
		} break;
		case Mode_Scaling: {
			return m_gizmoScaling.isInteracting();
		} break;
		case Mode_ScaleVolume: {
			return m_gizmoScaleVolume.isInteracting();
		} break;
		default:
			sgeAssert(false); // Not implemented.
	}

	return false;
}

} // namespace sge
