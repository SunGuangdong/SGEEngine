#ifndef SGE_LIB_TEXTUREMAPPING_SHADER
#define SGE_LIB_TEXTUREMAPPING_SHADER

/// Generates UV coordinates for sampling spherical mapped textures from a direction.
/// Usually spherical mapped texture are used for hdri images form light probes.
/// @dir direction of sampling, must be normalized.
float2 directionToUV_spherical(float3 dir) {
    float2 uv = float2(atan2(dir.z, dir.x), asin(-dir.y));
    const float2 invAtan = float2(0.1591, 0.3183);
	uv *= invAtan;
    uv += float2(0.5, 0.5);
    return uv;
}

/// The implementation here is based on https://en.wikipedia.org/wiki/Cube_mapping
/// A function that gives a UV coordinates for sampling a cube-mapped 2D texture from a direction.
/// The texture that is intended to be sampled roughly looks like so:
///
///     *---*
///     | +Y|
/// *---*---*---*---*
/// |+X | +Z| -X| -Z|
/// *---*---*---*---*
///     | -Y|
///     *---*
///
/// index 0 +X
/// index 1 -X
/// index 2 +Y
/// index 3 -Y
/// index 4 +Z
/// index 5 -Z
///
/// @param dirNormalized normal is the direction to be used for generating the UVs. It needs to be normalized.
/// @param pixelSizeUV is optional (pass (0,0) if not used). It should be the size of a single pixel in UV space, 
///        it is used to avoid seems when sampling the texture, by modifying the UV a bit.
float2 directionToUV_cubeMapping(float3 dirNormalized, float2 pixelSizeUV) {
	const float x = dirNormalized.x;
	const float y = dirNormalized.y;
	const float z = dirNormalized.z;
		
	float absX = abs(x);
	float absY = abs(y);
	float absZ = abs(z);

	const bool isXPositive = x > 0.f;
	const bool isYPositive = y > 0.f;
	const bool isZPositive = z > 0.f;

	float maxAxis, uc, vc;
	int index; // The index of the axis that the direction hits on the cube.

	// POSITIVE X
	if (isXPositive && absX >= absY && absX >= absZ) {
		// u (0 to 1) goes from +z to -z
		// v (0 to 1) goes from -y to +y
		maxAxis = absX;
		uc = -z;
		vc = y;
		index = 0;
	}
	// NEGATIVE X
	if (!isXPositive && absX >= absY && absX >= absZ) {
		// u (0 to 1) goes from -z to +z
		// v (0 to 1) goes from -y to +y
		maxAxis = absX;
		uc = z;
		vc = y;
		index = 1;
	}
	// POSITIVE Y
	if (isYPositive && absY >= absX && absY >= absZ) {
		// u (0 to 1) goes from -x to +x
		// v (0 to 1) goes from +z to -z
		maxAxis = absY;
		uc = x;
		vc = -z;
		index = 2;
	}
	// NEGATIVE Y
	if (!isYPositive && absY >= absX && absY >= absZ) {
		// u (0 to 1) goes from -x to +x
		// v (0 to 1) goes from -z to +z
		maxAxis = absY;
		uc = x;
		vc = z;
		index = 3;
	}
	// POSITIVE Z
	if (isZPositive && absZ >= absX && absZ >= absY) {
		// u (0 to 1) goes from -x to +x
		// v (0 to 1) goes from -y to +y
		maxAxis = absZ;
		uc = x;
		vc = y;
		index = 4;
	}
	// NEGATIVE Z
	if (!isZPositive && absZ >= absX && absZ >= absY) {
		// u (0 to 1) goes from +x to -x
		// v (0 to 1) goes from -y to +y
		maxAxis = absZ;
		uc = -x;
		vc = y;
		index = 5;
	}

	// Convert range from -1 to 1 to 0 to 1.
	// After this index will specify witch face index (+X, =X, +Y, ...) is the @dirNormalized point to and
	// in @uv we have UVs for sampling the image corresponding to that face.
	float2 uv = float2(0.5f * (uc / maxAxis + 1.0f), 0.5f * (vc / maxAxis + 1.0f));

	uv.x = clamp(uv.x, 0.f, 1.f);
	uv.y = clamp(uv.y, 0.f, 1.f);

	// The UV coordinates so far were in OpenGL stlye, convert them to D3D style as this is the one we default to in SGE.
	//uv.x = -uv.x + 1.f;
	uv.y = -uv.y + 1.f;

	

	const float oneThird = 0.333333333f; // 1.f / 3.f value in float32 precision.

	// Shrink the uv range to be as big as one sub-face in the bigger texture.
	uv = uv * float2(0.25f, oneThird);

	float faceWidthUv = 0.25f;
	float faceHeightUv = oneThird;

	// Move inwards the UVs near the edge of the face avoid having seems.
	// If we do not do that, the floating point precision + non-point sampling will
	// end up in nearing pixels that are not part of the cubemap to get used.
	// You might be tempted to do before we have shrinked the UVs to be as big as one sub-face, but
	// the floating point precision with the adding below will just destroy it.
	//uv.x = clamp(uv.x, pixelSizeUV.x * 0.5f, faceWidthUv - pixelSizeUV.x * 0.5f);
	//uv.y = clamp(uv.y, pixelSizeUV.y * 0.5f, faceHeightUv - pixelSizeUV.y * 0.5f);

	float2 halfPixelSize = pixelSizeUV * 0.5f;

	// The function so far returned a face index and a UVs inside that face.
	// We need to return UVs that correspond to the big cube-mapped texture.
	if(index == 0) {
		// +X
		uv = uv + float2(0.f, oneThird); 
	} else if(index == 1) {
		// -X
		uv = uv + float2(0.5f, oneThird); 
	} else if(index == 2) {
		// +Y
		uv = uv + float2(0.25f, 0.f); 
	} else if(index == 3) {
		// -Y
		uv = uv + float2(0.25f, 2.f * oneThird); 
	} else if(index == 4) {
		// +Z
		uv = uv + float2(0.25f, oneThird); 
	} else if(index == 5) {
		// -Z
		uv = uv + float2(0.75f, oneThird); 
	}

	return uv;
}


#endif
