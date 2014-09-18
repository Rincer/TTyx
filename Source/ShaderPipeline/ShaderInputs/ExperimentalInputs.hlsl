//--------------------------------------------------------------------------------------
struct VS_INPUT_EXPERIMENTAL
{
	float4 Pos : POSITION;
};

//--------------------------------------------------------------------------------------
struct PS_INPUT_EXPERIMENTAL
{
	float4 ViewPos	: SV_POSITION;
	float3 Normal	: NORMAL;
	float4 WorldPos : WORLDPOS; 
};

//--------------------------------------------------------------------------------------
// Shader constants
cbuffer cbBrdfParams
{
	float4 BaseColor;
	float4 RSM;
};


TextureCube		DynEnvMap;
SamplerState	SamplerDynEnvMap;	