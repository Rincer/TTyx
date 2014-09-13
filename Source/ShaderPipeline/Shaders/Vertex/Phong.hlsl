#include "..\..\ConstantBuffers\CBufferView.hlsl"
#include "..\..\ConstantBuffers\CBufferBones.hlsl"
#include "..\..\ConstantBuffers\CBufferLocal.hlsl"
#include "..\..\ShaderInputs\PhongInputs.hlsl"

iBaseTransform			gBaseTransform;
iLocalToWorldTransform  gLocalToWorldTransform;

cUnskinnedTransform		gUnSkinnedTransform;
cSkinnedTransform		gSkinnedTransform;    

cNullLocalToWorldTransform	gWorldTransform;
cLocalToWorldTransform		gLocalTransform;

#if (!USE_DYNAMIC_LINKAGE)
	#if (UNSKINNED)
		#define gBaseTransform gUnSkinnedTransform	
	#elif (SKINNED)
		#define gBaseTransform gSkinnedTransform	
	#endif
	
	#if (WORLDSPACE)
		#define gLocalToWorldTransform gWorldTransform
	#elif (LOCALSPACE)
		#define gLocalToWorldTransform gLocalTransform
	#endif
#endif


//--------------------------------------------------------------------------------------------
float4 cUnskinnedTransform::Transform(float4 VertexPosition, uint4 BoneIndices, float4 BoneWeights)
{
	return VertexPosition;
}


//--------------------------------------------------------------------------------------------
float4 cSkinnedTransform::Transform(float4 VertexPosition, uint4 BoneIndices, float4 BoneWeights)
{
	float4 SkinnedPosition;
	SkinnedPosition.w = 1;
	SkinnedPosition.x = dot(Bones[BoneIndices.x * 3 + 0], VertexPosition) * BoneWeights.x;
	SkinnedPosition.y = dot(Bones[BoneIndices.x * 3 + 1], VertexPosition) * BoneWeights.x;
	SkinnedPosition.z = dot(Bones[BoneIndices.x * 3 + 2], VertexPosition) * BoneWeights.x;				
	SkinnedPosition.x += dot(Bones[BoneIndices.y * 3 + 0], VertexPosition) * BoneWeights.y;
	SkinnedPosition.y += dot(Bones[BoneIndices.y * 3 + 1], VertexPosition) * BoneWeights.y;
	SkinnedPosition.z += dot(Bones[BoneIndices.y * 3 + 2], VertexPosition) * BoneWeights.y;				
	SkinnedPosition.x += dot(Bones[BoneIndices.z * 3 + 0], VertexPosition) * BoneWeights.z;
	SkinnedPosition.y += dot(Bones[BoneIndices.z * 3 + 1], VertexPosition) * BoneWeights.z;
	SkinnedPosition.z += dot(Bones[BoneIndices.z * 3 + 2], VertexPosition) * BoneWeights.z;						
	SkinnedPosition.x += dot(Bones[BoneIndices.w * 3 + 0], VertexPosition) * BoneWeights.w;
	SkinnedPosition.y += dot(Bones[BoneIndices.w * 3 + 1], VertexPosition) * BoneWeights.w;
	SkinnedPosition.z += dot(Bones[BoneIndices.w * 3 + 2], VertexPosition) * BoneWeights.w;								
	return SkinnedPosition;
}

//--------------------------------------------------------------------------------------------
void cNullLocalToWorldTransform::Transform(out float4 WorldPos, out float4 WorldNor, out float4 WorldTan, float4 LocalPos, float4 LocalNor, float4 LocalTan)
{
	WorldPos = LocalPos;
	WorldNor = LocalNor;
	WorldTan = LocalTan;	
}

//--------------------------------------------------------------------------------------------
void cLocalToWorldTransform::Transform(out float4 WorldPos, out float4 WorldNor, out float4 WorldTan, float4 LocalPos, float4 LocalNor, float4 LocalTan)
{
	WorldPos = mul(LocalToWorld, LocalPos);
	WorldNor = mul(LocalToWorldNormals, LocalNor);
	WorldTan = mul(LocalToWorldNormals, LocalTan);	
}

//--------------------------------------------------------------------------------------------
PS_INPUT VS( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;
    output.Tex = input.Tex;      
        
    float4 LocalPos = gBaseTransform.Transform(input.Pos, input.BoneIndices, input.BoneWeights);    
    float4 LocalNor = gBaseTransform.Transform(float4(input.Nor.xyz, 0), input.BoneIndices, input.BoneWeights); 
	float4 LocalTan = gBaseTransform.Transform(float4(input.Tan.xyz, 0), input.BoneIndices, input.BoneWeights);    			
	gLocalToWorldTransform.Transform(output.WorldPos, output.Nor, output.Tan, LocalPos, LocalNor, LocalTan);
    output.Tan.w = input.Tan.w;    
	output.ViewPos = mul( InvViewProjection, output.WorldPos);    
    return output;
}
