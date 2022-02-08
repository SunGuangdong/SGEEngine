#include "Gizmo3DDraw.h"
#include "Gizmo3D.h"
#include "sge_core/ICore.h"
#include "sge_core/QuickDraw.h"

namespace sge {

// A set of colors used to specify how the gizmo is drawn.
enum GizmoColors : unsigned { Gizmo_ActiveColor = 0xC000FFFF, Gizmo_ActiveTransp = 0xC0000000, Gizmo_BaseTransp = 0xB0000000 };


void drawGizmo(const RenderDestination& rdest, const Gizmo3D& gizmo, const mat4f& projView) {
	if (gizmo.getMode() == Gizmo3D::Mode_Rotation)
		drawRotationGizmo(rdest, gizmo.getGizmoRotation(), projView);

	if (gizmo.getMode() == Gizmo3D::Mode_Translation)
		drawTranslationGizmo(rdest, gizmo.getGizmoTranslation(), projView);

	if (gizmo.getMode() == Gizmo3D::Mode_Scaling)
		drawScaleGizmo(rdest, gizmo.getGizmoScale(), projView);

	if (gizmo.getMode() == Gizmo3D::Mode_ScaleVolume)
		drawScaleVolumeGizmo(rdest, gizmo.getGizmoScaleVolume(), projView);
}


void drawTranslationGizmo(const RenderDestination& rdest, const Gizmo3DTranslation& gizmo, const mat4f& projView) {
	const mat4f world = mat4f::getTranslation(gizmo.getEditedTranslation()) * mat4f::getScaling(gizmo.getDisplayScale());
	const mat4f pvw = projView * world;

	const vec3f* const axes = gizmo.getAxes();

	const auto addArrow = [&](const vec3f& dir, const vec3f& up, const int color) {
		const float bladeLen = 0.85f;
		const float bladeHeight = bladeLen * 0.05f;

		getCore()->getQuickDraw().drawWiredAdd_Line(vec3f(0.f), dir, color);

		// The things that make it look like an arrow.
		const int numBlades = 32;
		const mat4f rot = mat4f::getRotationQuat(quatf::getAxisAngle(dir, 2 * pi<float>() / (float)numBlades));

		vec3f new_up = up;
		for (int t = 0; t < numBlades; ++t) {
			getCore()->getQuickDraw().drawWiredAdd_Line(dir, dir * bladeLen + new_up * bladeHeight, color);
			getCore()->getQuickDraw().drawWiredAdd_Line(dir * bladeLen + new_up * bladeHeight, dir * bladeLen, color);

			new_up = mat_mul_dir(rot, new_up);
		}
	};

	const GizmoActionMask actionMask = gizmo.getActionMask();

	// Draw the axial translation arrows.
	addArrow(axes[0], axes[1], actionMask.only_x() ? Gizmo_ActiveColor : Gizmo_BaseTransp + 0x0000ff);
	addArrow(axes[1], axes[2], actionMask.only_y() ? Gizmo_ActiveColor : Gizmo_BaseTransp + 0x00ff00);
	addArrow(axes[2], axes[1], actionMask.only_z() ? Gizmo_ActiveColor : Gizmo_BaseTransp + 0xff0000);

	getCore()->getQuickDraw().drawWired_Execute(rdest, pvw, getCore()->getGraphicsResources().BS_backToFrontAlpha);

	// Draw the planar translation triangles.
	const auto addQuad = [&](const vec3f o, const vec3f e1, const vec3f e2, const int color) {
		getCore()->getQuickDraw().drawSolidAdd_Triangle(o, o + e1, o + e2, color);
		getCore()->getQuickDraw().drawSolidAdd_Triangle(o + e1, o + e1 + e2, o + e2, color);
	};

	const float quadOffset = Gizmo3DTranslation::kQuadOffset;
	const float quadLength = Gizmo3DTranslation::kQuatLength;

	addQuad(axes[1] * quadOffset + axes[2] * quadOffset, axes[1] * quadLength, axes[2] * quadLength,
	        actionMask.only_yz() ? Gizmo_ActiveColor : Gizmo_BaseTransp + 0x0000ff);
	addQuad(axes[0] * quadOffset + axes[2] * quadOffset, axes[0] * quadLength, axes[2] * quadLength,
	        actionMask.only_xz() ? Gizmo_ActiveColor : Gizmo_BaseTransp + 0x00ff00);
	addQuad(axes[0] * quadOffset + axes[1] * quadOffset, axes[0] * quadLength, axes[1] * quadLength,
	        actionMask.only_xy() ? Gizmo_ActiveColor : Gizmo_BaseTransp + 0xff0000);

	getCore()->getQuickDraw().drawSolid_Execute(rdest, pvw, false, getCore()->getGraphicsResources().BS_backToFrontAlpha);

	return;
}


void drawRotationGizmo(const RenderDestination& rdest, const Gizmo3DRotation& gizmo, const mat4f& projView) {
	mat4f const rotation = mat4f::getRotationQuat(gizmo.getEditedRotation());
	mat4f const world = mat4f::getTranslation(gizmo.getInitialTranslation()) * rotation * mat4f::getScaling(gizmo.getDisplayScale());
	mat4f const pvw = projView * world;

	vec3f const lookDirWS = (gizmo.getInitialTranslation() - gizmo.getLastInteractionEyePosition()).normalized0();

	// Transform the look dir by the inverse of the rotation of the gizmo (or just reverse the multiplication order)
	// in order to cancel out the rotation that is going to happen when we draw the vertices.
	vec3f const lookDir = (vec4f(lookDirWS, 0.f) * rotation).xyz().normalized0();

	const GizmoActionMask actionMask = gizmo.getActionMask();

	// Draw a circle that faces the camera
	{
		int const numSegments = 60;

		// Create a matrix that rotates around the look vector.
		mat4f const mtxRotation = mat4f::getRotationQuat(quatf::getAxisAngle(lookDir, sgePi * 2.f / numSegments));

		// Start form a random vertex that is perpendicular to the view direction.
		vec3f circleVert = cross(lookDir, vec3f(lookDir.x, lookDir.z, -lookDir.y)).normalized() * 1.05f;
		vec3f prevVert = circleVert;

		for (int t = 0; t < numSegments; ++t) {
			circleVert = mat_mul_dir(mtxRotation, circleVert);
			getCore()->getQuickDraw().drawSolidAdd_Triangle(vec3f(0.f), circleVert, prevVert, 0x33000000);
			prevVert = circleVert;
		}

		getCore()->getQuickDraw().drawSolid_Execute(rdest, pvw, false, getCore()->getGraphicsResources().BS_backToFrontAlpha);
	}

	// Draw the wires.
	{
		// TODO: It wolud be better if arcs were used instead of circles.
		const auto addCircle = [&](const vec3f& around, const int color) {
			vec3f dir = normalized0(cross(around, lookDir));

			// Check if we are parallel
			if (dir.lengthSqr() < 1e-6f)
				return;

			const int numSegments = 30;

			mat4f const rot = mat4f::getRotationQuat(quatf::getAxisAngle(around, sgePi / (float)numSegments));

			vec3f prev = dir;
			for (int t = 0; t < numSegments; ++t) {
				dir = (rot * vec4f(dir, 0.f)).xyz();

				getCore()->getQuickDraw().drawWiredAdd_Line(prev, dir, color);

				prev = dir;
			}
		};

		addCircle(vec3f::getAxis(0), actionMask.only_x() ? 0xff00ffff : 0xff0000ff);
		addCircle(vec3f::getAxis(1), actionMask.only_y() ? 0xff00ffff : 0xff00ff00);
		addCircle(vec3f::getAxis(2), actionMask.only_z() ? 0xff00ffff : 0xffff0000);

		getCore()->getQuickDraw().drawWiredAdd_Basis(mat4f::getDiagonal(0.5f));

		getCore()->getQuickDraw().drawWired_Execute(rdest, pvw, getCore()->getGraphicsResources().BS_backToFrontAlpha);
	}
}

void drawScaleGizmo(const RenderDestination& rdest, const Gizmo3DScale& gizmo, const mat4f& projView) {
	const mat4f world = mat4f::getTRS(gizmo.getInitialTranslation(), gizmo.getInitalRotation(), vec3f(gizmo.getDisplayScale()));
	const mat4f pvw = projView * world;

	const GizmoActionMask actionMask = gizmo.getActionMask();

	// Draw the lines for scaling along given axis.
	getCore()->getQuickDraw().drawWiredAdd_Line(vec3f(0.f), vec3f::getAxis(0), actionMask.only_x() ? 0xff00ffff : 0xff0000ff);
	getCore()->getQuickDraw().drawWiredAdd_Line(vec3f(0.f), vec3f::getAxis(1), actionMask.only_y() ? 0xff00ffff : 0xff00ff00);
	getCore()->getQuickDraw().drawWiredAdd_Line(vec3f(0.f), vec3f::getAxis(2), actionMask.only_z() ? 0xff00ffff : 0xffff0000);

	getCore()->getQuickDraw().drawWired_Execute(rdest, pvw, getCore()->getGraphicsResources().BS_backToFrontAlpha);

	// Draw the planar sclaing trapezoids.
	const auto addTrapezoid = [&](const vec3f& ax0, const vec3f& ax1, const int color) {
		const float inner = Gizmo3DScale::kTrapezoidStart;
		const float outter = Gizmo3DScale::kTrapezoidEnd;

		getCore()->getQuickDraw().drawSolidAdd_Triangle(ax0 * inner, ax0 * outter, ax1 * outter, color);
		getCore()->getQuickDraw().drawSolidAdd_Triangle(ax1 * inner, ax1 * outter, ax0 * inner, color);
	};

	addTrapezoid(vec3f::getAxis(0), vec3f::getAxis(1), actionMask.only_xy() ? Gizmo_ActiveColor : Gizmo_BaseTransp + 0xff0000);
	addTrapezoid(vec3f::getAxis(1), vec3f::getAxis(2), actionMask.only_yz() ? Gizmo_ActiveColor : Gizmo_BaseTransp + 0x0000ff);
	addTrapezoid(vec3f::getAxis(0), vec3f::getAxis(2), actionMask.only_xz() ? Gizmo_ActiveColor : Gizmo_BaseTransp + 0x00ff00);

	// Add the central triangles for drawing the scale all axes triangles.
	getCore()->getQuickDraw().drawSolidAdd_Triangle(vec3f(0.f), 0.22f * vec3f::getAxis(1), 0.22f * vec3f::getAxis(2),
	                                                actionMask.only_xyz() ? Gizmo_BaseTransp + 0x00eeee : Gizmo_BaseTransp + 0xeeeeee);
	getCore()->getQuickDraw().drawSolidAdd_Triangle(vec3f(0.f), 0.22f * vec3f::getAxis(0), 0.22f * vec3f::getAxis(2),
	                                                actionMask.only_xyz() ? Gizmo_BaseTransp + 0x00cccc : Gizmo_BaseTransp + 0xcccccc);
	getCore()->getQuickDraw().drawSolidAdd_Triangle(vec3f(0.f), 0.22f * vec3f::getAxis(0), 0.22f * vec3f::getAxis(1),
	                                                actionMask.only_xyz() ? Gizmo_BaseTransp + 0x00bbbb : Gizmo_BaseTransp + 0xbbbbbb);

	getCore()->getQuickDraw().drawSolid_Execute(rdest, pvw, false, getCore()->getGraphicsResources().BS_backToFrontAlpha);
}

void drawScaleVolumeGizmo(const RenderDestination& rdest, const Gizmo3DScaleVolume& gizmo, const mat4f& projView) {
	const mat4f os2ws = gizmo.getEditedTrasform().toMatrix();

	const Box3f boxOs = gizmo.getInitialBBoxOS();
	const float handleSizeWS = gizmo.getHandleRadiusWS();

	vec3f boxFaceCentersOs[signedAxis_numElements];
	boxOs.getFacesCenters(boxFaceCentersOs);

	vec3f bboxFacesNormalsOs[signedAxis_numElements];
	boxOs.getFacesNormals(bboxFacesNormalsOs);

	const GizmoActionMask actionMask = gizmo.getActionMask();

	unsigned int axisColors[signedAxis_numElements] = {
	    0xff0000ff, 0xff00ff00, 0xffff0000, 0xff000066, 0xff006600, 0xff660000,
	};

	unsigned int activeColor = 0xff00ffff;

	for (int iAxis = 0; iAxis < signedAxis_numElements; ++iAxis) {
		const vec3f handlePosWs = mat_mul_pos(os2ws, boxFaceCentersOs[iAxis]);
		const vec3f handleNormalWs = mat_mul_dir(os2ws, bboxFacesNormalsOs[iAxis]).normalized0();

		// Draw a quat that faces the camera representing the handle.
		const vec3f e1 = quat_mul_pos(gizmo.getInitialTransform().r, vec3f::getAxis((iAxis + 1) % 3)) * handleSizeWS;
		const vec3f e2 = quat_mul_pos(gizmo.getInitialTransform().r, vec3f::getAxis((iAxis + 2) % 3)) * handleSizeWS;

		getCore()->getQuickDraw().drawSolidAdd_QuadCentered(handlePosWs, e1, e2,
		                                                    actionMask.hasOnly(SignedAxis(iAxis)) ? activeColor : axisColors[iAxis]);
		getCore()->getQuickDraw().drawSolid_Execute(rdest, projView, false, getCore()->getGraphicsResources().BS_backToFrontAlpha);
	}

	getCore()->getQuickDraw().drawWiredAdd_Box(os2ws, gizmo.getInitialBBoxOS(), 0xffffffff);
	getCore()->getQuickDraw().drawWired_Execute(rdest, projView);
}


} // namespace sge
