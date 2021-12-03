#include "lib_textureMapping.hlsl"

cbuffer SkyShaderCBufferParams {
	float4x4 uView;
	float4x4 uProj;
	float4x4 uWorld;
	float3 uColorBottom;
	float3 uColorTop;
	int mode;
};

uniform sampler2D uSkyTexture;

//--------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------
struct VS_INPUT
{
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

PS_OUTPUT psMain(VS_OUTPUT IN)
{
	const int mode_colorGradinet = 0;
	const int mode_textureSphericalMapped = 1;
	const int mode_textureCubeMapped = 2;
	
	PS_OUTPUT psOut;
	
	const float3 normal = normalize(IN.dirV);
	
	float3 color = float3(1.f, 0.f, 1.f);
	if(mode == mode_colorGradinet) {
		color = lerp(uColorBottom, uColorTop, normal.y * 0.5f + 0.5f);
	} else  if(mode == mode_textureSphericalMapped) {
		float2 uv = directionToUV_spherical(normal);
		color = tex2D(uSkyTexture, uv);
	}
	else  if(mode == mode_textureCubeMapped) {
		float2 uv = directionToUV_cubeMapping(normal, 1.f / float2(tex2Dsize(uSkyTexture)));
		// mip-mapping cube mapped 2d textures produces seems when sampling
		// as the color near the edges of the sub-faces blends with the actual texture.
		// This is why we do not do mip-mapping.
		color = tex2Dlod(uSkyTexture, float4(uv, 0.f, 0.f)); 
	} else {
		color = float3(1.f, 0.f, 1.f);
	}
	
	psOut.target0 = float4(color, 1.f);
	psOut.depth = 1.0f;
	return psOut;
}