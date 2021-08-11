#pragma once

#include "vec4.h"

namespace sge {

/// @brief Creates a vec4f that represents a rgba color from the specified bytes.
inline uint32 colorIntFromRGBA255(ubyte r, ubyte g, ubyte b, ubyte a = 255) {
	uint32 c = 0;
	c |= (int)(r) << 0;
	c |= (int)(g) << 8;
	c |= (int)(b) << 16;
	c |= (int)(a) << 24;
	return c;
};

/// @brief Converts the specified rgba color to  a sigle integer. 
/// Each component [0;1] range is converted to a byte in [0;255]
inline uint32 colorToIntRgba(float r, float g, float b, float a = 1.f) {
	r = clamp01(r);
	g = clamp01(g);
	b = clamp01(b);
	a = clamp01(a);

	uint32 c = 0;
	c |= (int)(r * 255) << 0;
	c |= (int)(g * 255) << 8;
	c |= (int)(b * 255) << 16;
	c |= (int)(a * 255) << 24;
	return c;
};

/// @brief Converts the specified rgb color and alpha to a single to integer. 
/// Each component [0;1] range is converted to a byte in [0;255]
inline uint32 colorToIntRgba(const vec3f& rgb, float a = 1.f) {
	return colorToIntRgba(rgb.x, rgb.y, rgb.z, a);
};

inline uint32 colorToIntRgba(const vec4f& rgba) {
	return colorToIntRgba(rgba.x, rgba.y, rgba.z, rgba.z);
};

/// @brief Converts an int32 encoded RGBA color to vec4f rgba color.
/// 0-7 bits holds red
/// 8-15 bits holds green
/// 16-23 bits holds blue
/// 24-31 bits holds the alpha.
inline vec4f colorFromIntRgba(uint32 rgba) {
	float r = float(rgba & 0xFF) / 255.f;
	float g = float((rgba >> 8) & 0xFF) / 255.f;
	float b = float((rgba >> 16) & 0xFF) / 255.f;
	float a = float((rgba >> 24) & 0xFF) / 255.f;

	return vec4f(r, g, b, a);
}

inline vec4f colorFromIntRgba(ubyte r255, ubyte g255, ubyte b255, ubyte a255 = 255) {
	float r = float(r255) / 255.f;
	float g = float(g255) / 255.f;
	float b = float(b255) / 255.f;
	float a = float(a255) / 255.f;

	return vec4f(r, g, b, a);
}

inline vec4f colorBlack(float alpha) {
	return vec4f(0.f, 0.f, 0.f, alpha);
}

inline vec4f colorWhite(float alpha) {
	return vec4f(1.f, 1.f, 1.f, alpha);
}


} // namespace sge
