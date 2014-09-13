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
	float4 Specular;
	float4 Diffuse;
	float4 Roughness;
};


TextureCube		DynEnvMap;
SamplerState	SamplerDynEnvMap;	