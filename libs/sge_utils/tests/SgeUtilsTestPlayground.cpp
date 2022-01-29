#include "doctest/doctest.h"
#include "sge_utils/math/EulerAngles.h"
#include "sge_utils/math/common.h"
#include "sge_utils/math/transform.h"
#include "sge_utils/text/format.h"
#include <string>
using namespace sge;

bool isAboutTheSame(const mat4f& a, const mat4f& b, const float t = 1e-6f) {
	for (int c = 0; c < 4; ++c)
		for (int r = 0; r < 4; ++r) {
			if (fabsf(a.data[c][r] - b.data[c][r]) > t) {
				return false;
			}
		}

	return true;
}

std::string toString(const mat4f& m) {
	std::string result;

	for (int c = 0; c < 4; ++c)
		for (int r = 0; r < 4; ++r) {
			if (r == 0) {
				result += "|\t";
			}

			result += string_format("%.3g\t", fabsf(m.data[c][r]) <= 1e-5f ? 0.f : m.data[c][r]);

			if (r == 3) {
				result += "\t|\n";
			}
		}

	return result;
}

std::string toString(const quatf& q) {
	return string_format("(%.3f %.3f %.3f) %.3f", q.x, q.y, q.z, q.w);
}


TEST_CASE("SGE Utils Playground") {
	SUBCASE("Identity") {
		const mat4f mi = mat4f::getIdentity();
		CHECK(isAboutTheSame(mi, mat4f::getRotationQuat(mi.toQuat())));
	}

	SUBCASE("X90") { const mat4f m = mat4f::getRotationX(deg2rad(90.f)); }

	SUBCASE("Y90") {
		const mat4f m = mat4f::getRotationY(deg2rad(90.f));
		CHECK(isAboutTheSame(m, mat4f::getRotationQuat(m.toQuat())));
	}

	SUBCASE("Z90") {
		const mat4f m = mat4f::getRotationZ(deg2rad(90.f));
		CHECK(isAboutTheSame(m, mat4f::getRotationQuat(m.toQuat())));
	}

	SUBCASE("Random 0") {
		const mat4f m = mat4f::getRotationQuat(eulerToQuaternionDegrees(35.f, 12.f, 125.f));
		CHECK(isAboutTheSame(m, mat4f::getRotationQuat(m.toQuat())));
	}

	SUBCASE("Random 1") {
		const mat4f m = mat4f::getRotationQuat(eulerToQuaternionDegrees(-125.f, 68.f, 7.f));
		CHECK(isAboutTheSame(m, mat4f::getRotationQuat(m.toQuat())));
	}

	SUBCASE("Random 1") {
		const mat4f m = mat4f::getRotationQuat(eulerToQuaternionDegrees(24.f, 1.f, 212.f));
		CHECK(isAboutTheSame(m, mat4f::getRotationQuat(m.toQuat())));
	}
}
