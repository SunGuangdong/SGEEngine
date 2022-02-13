//
// A file written in C containing commonalities between C++ and HLSL
//
#ifndef SHADECOMMON_H
#define SHADECOMMON_H

#define kMaxLights 4

// Setting for OPT_HasVertexColor, vertex color can be used as diffuse source or for tinting.
#define kHasVertexColor_No 0
#define kHasVertexColor_Yes 1

// OPT_HasUV
#define kHasUV_No 0
#define kHasUV_Yes 1

// OPT_HasNormals
#define kHasTangetSpace_No 0
#define kHasTangetSpace_Yes 1

// OPT_HasVertexSkinning settings
#define kHasVertexSkinning_No 0
#define kHasVertexSkinning_Yes 1

// OPT_DiffuseTexForAlphaMasking
#define kHasDiffuseTexForAlphaMasking_No 0
#define kHasDiffuseTexForAlphaMasking_Yes 1

// Material flags.
#define kPBRMtl_Flags_HasNormalMap 1
#define kPBRMtl_Flags_HasMetalnessMap 2
#define kPBRMtl_Flags_HasRoughnessMap 4

#define kPBRMtl_Flags_DiffuseFromConstantColor 8
#define kPBRMtl_Flags_DiffuseFromTexture 16
#define kPBRMtl_Flags_DiffuseFromVertexColor 32
#define kPBRMtl_Flags_DiffuseTintByVertexColor 64
#define kPBRMtl_Flags_NoLighting 128

#endif
