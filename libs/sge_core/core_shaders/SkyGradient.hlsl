#include "lib_textureMapping.hlsl"

cbuffer SkyShaderCBufferParams
{
	float4x4 uView;
	float4x4 uProj;
	float4x4 uWorld;
	float3 uColorBottom;
	float3 uColorTop;
	float3 uSunDir;
	int mode;
};

uniform sampler2D uSkyTexture;

//--------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------
struct VS_INPUT {
	float3 a_position : a_position;
};

struct VS_OUTPUT {
	float4 SV_Position : SV_Position;
	float3 dirV : dirV;
};

VS_OUTPUT vsMain(VS_INPUT vsin)
{
	VS_OUTPUT res;
	float4 pointScaled = mul(uWorld, float4(vsin.a_position, 0.f));
	float3 pointViewSpace = mul(uView, pointScaled).xyz;
	float4 pointProj = mul(uProj, float4(pointViewSpace, 1.0));
	res.SV_Position = pointProj;

	res.dirV = vsin.a_position;

	return res;
}
//--------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------
struct PS_OUTPUT {
	float4 target0 : SV_Target0;
	float depth : SV_Depth;
};


float rayleighPhaseFunction(float mu)
{
	return 3.0f * (1.0 + mu * mu) / (16.0f * 3.1415f);
}

float henyeyGreensteinPhaseFunc(float mu)
{
	const float g = 0.76f;
	return (1. - g * g) / ((4.f * 3.14f) * pow(1. + g * g - 2.f * g * mu, 1.5f));
}

bool rayIntersectSphere(float3 ro, float3 rd, float3 sphereOrigin, float sphereRaduis, out float t0, out float t1)
{
	ro -= sphereOrigin;
	float a = dot(rd, rd);
	float b = 2.f * dot(ro, rd);
	float c = dot(ro, ro) - sphereRaduis * sphereRaduis;
	float discriminant = b * b - 4.f * a * c;

	if (discriminant < 0.f) {
		return false;
	}

	float denomMult = 1.f / 2.f * a;

	t0 = (-b - sqrt(discriminant)) * denomMult;
	t1 = (-b + sqrt(discriminant)) * denomMult;

	return true;
}

// const variables
static const float kEarthRadiusMeters = 6360e3f;

/// A direct copy of:
/// https://www.scratchapixel.com/lessons/procedural-generation-virtual-worlds/simulating-sky/simulating-colors-of-the-sky
float3 computeSemiRealisticSky(float3 viewOrigin, float3 viewDirection, float3 sunDir)
{
	const float S_Luminance = 20.f;
	const float R_atmo = 6420e3f;
	const float H_Air = 7994.f;
	const float H_Aerosols = 1200.f;
	const float3 betaRayleigh = float3(3.8e-6f, 13.5e-6f, 33.1e-6f);
	const float3 betaMie = float3(21.0e-6f, 21.0e-6f, 21.0e-6f);
	const int numSamples_View = 16;
	const int numSamples_Sun = 8;

	// Ray setup -> view ray
	float t0_View, t1_View;
	float3 densityAir_View, densityAerosols_View;
	float3 totalRayleigh = float3(0.f, 0.f, 0.f);
	float3 totalMie = float3(0.f, 0.f, 0.f);

	// Atmosphere intersection -> view ray
	bool hit = rayIntersectSphere(viewOrigin, viewDirection, float3(0.0, 0.0, 0.0), R_atmo, t0_View, t1_View);
	if (!hit) {
		return totalRayleigh;
	}

	// Phase functions
	float mu = dot(viewDirection, sunDir);
	float phaseRayleigh = rayleighPhaseFunction(mu);
	float phaseMie = henyeyGreensteinPhaseFunc(mu);

	// Ray marching -> view ray
	float stepSize_View = t1_View / float(numSamples_View);
	float t_View = 0.0;
	for (int i = 0; i < numSamples_View; ++i) {
		// Ray pos and height
		float3 pos_View = viewOrigin + viewDirection * (t_View + stepSize_View * 0.5f);
		float height_View = length(pos_View) - kEarthRadiusMeters;

		// Density at current location
		float airDensityAtPos_View = exp(-height_View / H_Air) * stepSize_View;
		float aerosolDensityAtPos_View = exp(-height_View / H_Aerosols) * stepSize_View;
		densityAir_View += airDensityAtPos_View;
		densityAerosols_View += aerosolDensityAtPos_View;

		// Ray setup -> Sun ray
		float3 densityAir_Sun, densityAerosols_Sun;
		float t0_Sun, t1_Sun;

		// Atmosphere intersection -> Sun ray
		rayIntersectSphere(pos_View, sunDir, float3(0.0, 0.0, 0.0), R_atmo, t0_Sun, t1_Sun);

		// Raymarching from point in view ray -> sunDir
		float t_Sun = 0.f;
		float stepSize_Sun = t1_Sun / float(numSamples_Sun);
		bool hitGround = false;
		for (int i = 0; i < numSamples_Sun; ++i) {
			// Ray pos and height
			float3 pos_Sun = pos_View + sunDir * (t_Sun + stepSize_Sun * 0.5f);
			float height_Sun = length(pos_Sun) - kEarthRadiusMeters;

			// Early out if you're in earth shadow
			if (height_Sun < 0.) {
				hitGround = true;
				break;
			}

			// Density at current location
			densityAir_Sun += exp(-height_Sun / H_Air) * stepSize_Sun;
			densityAerosols_Sun += exp(-height_Sun / H_Aerosols) * stepSize_Sun;

			t_Sun += stepSize_Sun;
		}

		// Earth shadow
		if (!hitGround) {
			// Multiplication of exponentials == sum of exponents
			float3 tau = betaRayleigh * (densityAir_View);
			tau += betaMie * 1.1 * (densityAerosols_Sun + densityAerosols_View);
			float3 transmittance = exp(-tau);

			// Total scattering from both view and sun ray direction up to this point
			totalRayleigh += airDensityAtPos_View * transmittance;
			totalMie += aerosolDensityAtPos_View * transmittance;
		}

		t_View += stepSize_View;
	}

	return S_Luminance * (totalRayleigh * phaseRayleigh * betaRayleigh + totalMie * phaseMie * betaMie);
}



PS_OUTPUT psMain(VS_OUTPUT IN)
{
	// [SKY_ENUM_DUPLICATED]
	const int mode_colorGradinet = 0;
	const int mode_semiRealistic = 1;
	const int mode_textureSphericalMapped = 2;
	const int mode_textureCubeMapped = 3;


	PS_OUTPUT psOut;

	const float3 normal = normalize(IN.dirV);

	float3 color = float3(1.f, 0.f, 1.f);
	if (mode == mode_colorGradinet) {
		color = lerp(uColorBottom, uColorTop, normal.y * 0.5f + 0.5f);
	}
	else if (mode == mode_semiRealistic) {
		// Shoot the ray at the surface of the Earth as if a human was watching it.
		float3 rayOrigin = float3(0.f, kEarthRadiusMeters + 1.8f, 0.f);
		color = computeSemiRealisticSky(rayOrigin, normal, uSunDir);
	}
	else if (mode == mode_textureSphericalMapped) {
		float2 uv = directionToUV_spherical(normal);
		color = tex2D(uSkyTexture, uv);
	}
	else if (mode == mode_textureCubeMapped) {
		float2 uv = directionToUV_cubeMapping(normal, 1.f / float2(tex2Dsize(uSkyTexture)));
		// mip-mapping cube mapped 2d textures produces seems when sampling
		// as the color near the edges of the sub-faces blends with the actual texture.
		// This is why we do not do mip-mapping.
		color = tex2Dlod(uSkyTexture, float4(uv, 0.f, 0.f));
	}
	else {
		color = float3(1.f, 0.f, 1.f);
	}

	psOut.target0 = float4(color, 1.f);
	psOut.depth = 1.0f;
	return psOut;
}
