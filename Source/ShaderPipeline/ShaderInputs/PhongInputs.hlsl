#ifndef _OBJ_INPUTS_H_
#define _OBJ_INPUTS_H_


// Vertex shader------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float4 Pos			: POSITION;
    float2 Tex			: TEXCOORD0;
    float4 Nor			: TEXCOORD1;
    float4 Tan			: TEXCOORD2;
	uint4  BoneIndices	: BONEINDICES;
	float4 BoneWeights	: BONEWEIGHTS;
};


// Local position interface
interface iBaseTransform
{
	float4 Transform(float4 VertexPosition, uint4 BoneIndices, float4 BoneWeights);
};

class cUnskinnedTransform : iBaseTransform
{
	float a; // Need this to prevent the interface from being optimized out due to 0 size
	float4 Transform(float4 VertexPosition, uint4 BoneIndices, float4 BoneWeights);
};


class cSkinnedTransform : iBaseTransform
{
	float b; // Need this to prevent the interface from being optimized out due to 0 size
	float4 Transform(float4 VertexPosition, uint4 BoneIndices, float4 BoneWeights);
};



// Local to world interface
interface iLocalToWorldTransform
{
	void Transform(out float4 WorldPos, out float4 WorldNor, out float4 WorldTan, float4 LocalPos, float4 LocalNor, float4 LocalTan);
};

class cNullLocalToWorldTransform : iLocalToWorldTransform
{
	float a; // Need this to prevent the interface from being optimized out due to 0 size
	void Transform(out float4 WorldPos, out float4 WorldNor, out float4 WorldTan, float4 LocalPos, float4 LocalNor, float4 LocalTan);	
};


class cLocalToWorldTransform : iLocalToWorldTransform
{
	float b; // Need this to prevent the interface from being optimized out due to 0 size
	void Transform(out float4 WorldPos, out float4 WorldNor, out float4 WorldTan, float4 LocalPos, float4 LocalNor, float4 LocalTan);	
};



// Pixel shader-------------------------------------------------------------------------
// Sampler and texture stages
Texture2D map_Kd;	// Diffuse                            
Texture2D map_Ks;	// Specular
Texture2D map_d;	// Alpha
Texture2D map_bump;	// Bump

SamplerState sam_Kd;	// Diffuse                            
SamplerState sam_Ks;	// Specular
SamplerState sam_d;		// Alpha
SamplerState sam_bump;	// Bump

//--------------------------------------------------------------------------------------
struct PS_INPUT
{
    float4 ViewPos	: SV_POSITION;
    float2 Tex		: TEXCOORD0;
    float4 Nor		: TEXCOORD1;
	float4 Tan		: TEXCOORD2;    
	float4 WorldPos : TEXCOORD3;    
};

// Diffuse interface
//--------------------------------------------------------------------------------------
interface iBaseDiffuse
{
	float3 GetDiffuse(float2 TexCoord);
};

//--------------------------------------------------------------------------------------
class cTexturedDiffuse : iBaseDiffuse
{	
	float a; // Need this to prevent the interface from being optimized out due to 0 size
	float3 GetDiffuse(float2 TexCoord);
};

//--------------------------------------------------------------------------------------
class cUnTexturedDiffuse : iBaseDiffuse
{
	float b; // Need this to prevent the interface from being optimized out due to 0 size
	float3 GetDiffuse(float2 TexCoord);
};


// Opacity interface
//--------------------------------------------------------------------------------------
interface iBaseOpacity
{
	float GetOpacity(float2 TexCoord);
};


//--------------------------------------------------------------------------------------
class cTexturedOpacity : iBaseOpacity
{	
	float a; // Need this to prevent the interface from being optimized out due to 0 size
	float GetOpacity(float2 TexCoord);
};

//--------------------------------------------------------------------------------------
class cUnTexturedOpacity : iBaseOpacity
{
	float b; // Need this to prevent the interface from being optimized out due to 0 size
	float GetOpacity(float2 TexCoord);
};



// Normal interface
//--------------------------------------------------------------------------------------
interface iBaseNormal
{
	float3 GetNormal(float2 TexCoord, float3 Normal, float4 Tangent);
};


//--------------------------------------------------------------------------------------
class cNormalMapped : iBaseNormal
{	
	float a; // Need this to prevent the interface from being optimized out due to 0 size
	float3 GetNormal(float2 TexCoord, float3 Normal, float4 Tangent);
};

//--------------------------------------------------------------------------------------
class cVertexNormal : iBaseNormal
{
	float b; // Need this to prevent the interface from being optimized out due to 0 size
	float3 GetNormal(float2 TexCoord, float3 Normal, float4 Tangent);
};


// Specular interface
//--------------------------------------------------------------------------------------
interface iBaseSpecular
{
	float3 GetSpecular(float2 TexCoord, float3 L, float3 N, float3 WorldToView);
};

//--------------------------------------------------------------------------------------
class cZeroSpecular: iBaseSpecular
{
	float a; // Need this to prevent the interface from being optimized out due to 0 size
	float3 GetSpecular(float2 TexCoord, float3 L, float3 N, float3 WorldToView);
};

//--------------------------------------------------------------------------------------
class cTexturedSpecular: iBaseSpecular
{
	float b; // Need this to prevent the interface from being optimized out due to 0 size
	float3 GetSpecular(float2 TexCoord, float3 L, float3 N, float3 WorldToView);
};

//--------------------------------------------------------------------------------------
class cUnTexturedSpecular: iBaseSpecular
{
	float c; // Need this to prevent the interface from being optimized out due to 0 size
	float3 GetSpecular(float2 TexCoord, float3 L, float3 N, float3 WorldToView);
};


//--------------------------------------------------------------------------------------
// Shader constants
cbuffer cbObjParams
{
	float4 DiffuseParams;	// xyz = color
	float4 SpecularParams;	// xyz = color, w = shinyness
};


#endif