#pragma once

#include <sge_utils/math/mat4.h>

namespace sge {

struct orbit_camera {
	float radius = 2.f;
	float yaw = 0.f;
	float pitch = 0.f;
	vec3f orbitPoint = vec3f(0.f);
	vec2f prevMousePos = vec2f(0.f);

	void orientation(vec3f& orbitToEye, vec3f& up, vec3f& right) const {
		orbitToEye = vec3f(cos(yaw) * cos(pitch), sin(pitch), -sin(yaw) * cos(pitch)) * radius;

		up = normalized(
		    vec3f(cos(yaw) * cos(pitch + half_pi<float>()), sin(pitch + half_pi<float>()), -sin(yaw) * cos(pitch + half_pi<float>())));

		right = normalized(cross(up, orbitToEye));
	}

	vec3f eyePosition() const { return orbitPoint + vec3f(cos(yaw) * cos(pitch), sin(pitch), -sin(yaw) * cos(pitch)) * radius; }

	mat4f GetViewMatrix() const {
		vec3f orbitToEye, up, right;
		orientation(orbitToEye, up, right);

		vec3f const cameraPosition = orbitPoint + orbitToEye;
		return mat4f::getLookAtRH(cameraPosition, orbitPoint, up);
	}

	void strafe(const float amountRight, const float amountUp) {
		vec3f orbitToEye, up, right;
		orientation(orbitToEye, up, right);

		orbitPoint += amountRight * right;
		orbitPoint += amountUp * up;
	}

	// Does a "Maya" style camera movement.
	// Returns true if the the camera position has been changed by the specfied input.
	bool update(const bool alt, const bool left, const bool middle, const bool right, const vec2f mousePos) {
		bool hasChanged = false;

		const vec2f diff = mousePos - prevMousePos;

		if (alt) {
			// Rotate.
			if (left && !(middle || right)) {
				yaw += -diff.x * 0.01f;
				pitch += diff.y * 0.01f;

				hasChanged = true;
			}

			// Zoom.
			if (right) {
				radius += diff.y * 0.08f * radius * 0.1f;
				if (radius < 0.1f)
					radius = 0.1f;

				hasChanged = true;
			}

			// Pan.
			else if (middle) {
				strafe(-diff.x * 0.007f * radius * 0.1f, diff.y * 0.007f * radius * 0.1f);

				hasChanged = true;
			}
		}

		prevMousePos = mousePos;

		return hasChanged;
	}
};

} // namespace sge
