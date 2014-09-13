#ifndef _UTILITY_INPUTS_H_
#define _UTILITY_INPUTS_H_

Texture2D	 Texture;	
SamplerState Sampler;	

//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD;
    float4 Col : COLOR;
};

//--------------------------------------------------------------------------------------
struct PS_INPUT_PLAINCOLOR
{
    float4 ViewPos	: SV_POSITION;
    float4 Col		: COLOR;    
};


//--------------------------------------------------------------------------------------
struct PS_INPUT_TEXTURED
{
    float4 ViewPos	: SV_POSITION;
    float2 Tex		: TEXCOORD;
    float4 Col		: COLOR;    
};

#endif