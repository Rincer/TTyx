#ifndef _POSTPROCESS_INPUTS_H_
#define _POSTPROCESS_INPUTS_H_

Texture2D	 HDRTexture;	
SamplerState HDRSampler;	

//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD;
};

//--------------------------------------------------------------------------------------
struct PS_INPUT
{
    float4 ViewPos	: SV_POSITION;
    float2 Tex		: TEXCOORD;
};

#endif