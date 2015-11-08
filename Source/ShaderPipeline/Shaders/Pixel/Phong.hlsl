#include "..\..\ConstantBuffers\CBufferView.hlsl"
#include "..\..\ConstantBuffers\CBufferLights.hlsl"
#include "..\..\ConstantBuffers\CBufferBones.hlsl"
#include "..\..\ShaderInputs\PhongInputs.hlsl"


iBaseDiffuse		gBaseDiffuse;
cTexturedDiffuse	gTexturedDiffuse;
cUnTexturedDiffuse  gUnTexturedDiffuse;        

iBaseOpacity		gBaseOpacity;
cTexturedOpacity	gTexturedOpacity;
cUnTexturedOpacity  gUnTexturedOpacity;        

iBaseNormal	  gBaseNormal;
cNormalMapped gNormalMapped;
cVertexNormal gVertexNormal;
	
iBaseSpecular		gBaseSpecular;
cZeroSpecular		gZeroSpecular;
cTexturedSpecular	gTexturedSpecular;
cUnTexturedSpecular gUnTexturedSpecular;

#if (!USE_DYNAMIC_LINKAGE)
	#if (DIFFUSE_TEXTURED)
		#define gBaseDiffuse gTexturedDiffuse
	#elif (DIFFUSE_UNTEXTURED)
		#define gBaseDiffuse gUnTexturedDiffuse
	#endif

	#if (OPACITY_TEXTURED)
		#define gBaseOpacity gTexturedOpacity
	#elif (OPACITY_UNTEXTURED)
		#define gBaseOpacity gUnTexturedOpacity
	#endif

	
	#if (NORMALMAPPED)
		#define gBaseNormal gNormalMapped
	#elif (VERTEXNORMAL)
		#define gBaseNormal gVertexNormal
	#endif

	
	#if (SPECULAR_ZERO)
		#define gBaseSpecular gZeroSpecular
	#elif (SPECULAR_TEXTURED)
		#define gBaseSpecular gTexturedSpecular
	#elif (SPECULAR_UNTEXTURED)
		#define gBaseSpecular gUnTexturedSpecular
	#endif
#endif
	

//--------------------------------------------------------------------------------------
float3 cTexturedDiffuse::GetDiffuse(float2 TexCoord)
{
	return map_Kd.Sample( sam_Kd, TexCoord ).xyz * DiffuseParams.xyz;
}

//--------------------------------------------------------------------------------------
float3 cUnTexturedDiffuse::GetDiffuse(float2 TexCoord)
{		
	return DiffuseParams.xyz;
}

//--------------------------------------------------------------------------------------
float cTexturedOpacity::GetOpacity(float2 TexCoord)
{
	float Opacity = map_d.Sample(sam_d, TexCoord ).x;
	if(Opacity <= 0.0f)
	{
		discard;			
	}
	return Opacity;
}

//--------------------------------------------------------------------------------------
float cUnTexturedOpacity::GetOpacity(float2 TexCoord)
{		
	return 1;
}


//--------------------------------------------------------------------------------------
float3 cNormalMapped::GetNormal(float2 TexCoord, float3 Normal, float4 Tangent)
{
	float3 N = map_bump.Sample( sam_bump, TexCoord).xyz;  
	N = normalize(N * 2 - 1);
	float3 BiTangent = normalize(cross(Normal, Tangent.xyz) * Tangent.www);	
	float3x3 TangetSpace = float3x3(Tangent.xyz, BiTangent, Normal);
	N = normalize(mul(N, TangetSpace));
	return N;	
}

//--------------------------------------------------------------------------------------
float3 cVertexNormal::GetNormal(float2 TexCoord, float3 Normal, float4 Tangent)
{
	return Normal;	
}	



//--------------------------------------------------------------------------------------
float3 cZeroSpecular::GetSpecular(float2 TexCoord, float3 L, float3 N, float3 WorldToView)
{
	return float3(0, 0, 0);
}


//--------------------------------------------------------------------------------------
float3 cTexturedSpecular::GetSpecular(float2 TexCoord, float3 L, float3 N, float3 WorldToView)
{
	float3 R = 2 * dot(L, N) * N - L;
	float  SpecularAttenuation = pow(saturate(dot(R, normalize(WorldToView))), SpecularParams.w); 
	float3 SpecularCoefficient = map_Ks.Sample(sam_Ks, TexCoord).xyz * SpecularParams.xyz;	
	return 	SpecularAttenuation * SpecularCoefficient;
}


//--------------------------------------------------------------------------------------
float3 cUnTexturedSpecular::GetSpecular(float2 TexCoord, float3 L, float3 N, float3 WorldToView)
{
	float3 R = 2 * dot(L, N) * N - L;
	float  SpecularAttenuation = pow(saturate(dot(R, normalize(WorldToView))), SpecularParams.w); 
	float3 SpecularCoefficient = SpecularParams.xyz;	
	return 	SpecularAttenuation * SpecularCoefficient;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS( PS_INPUT input) : SV_Target
{
	float Alpha = gBaseOpacity.GetOpacity(input.Tex);
	float3 N = gBaseNormal.GetNormal(input.Tex, normalize(input.Nor.xyz), normalize(input.Tan));
		
	// point light
	float3 WorldToLight = LightsPosInvSqR.xyz - input.WorldPos.xyz; 	
	float3 L = normalize(WorldToLight);	
	
	float3 DiffuseReflection = float3(0, 0, 0);
	float3 SpecularReflection = float3(0, 0, 0);
	float AngularAttenuation = saturate(dot(L, N));
	float DistanceAttenuation = saturate((LightsColorSqR.w - dot(WorldToLight,WorldToLight)) * LightsPosInvSqR.w);
								
	DiffuseReflection  = gBaseDiffuse.GetDiffuse(input.Tex);		
	SpecularReflection = gBaseSpecular.GetSpecular(input.Tex, L, N, CameraPos.xyz - input.WorldPos.xyz); 

	float3 LightsColor = DistanceAttenuation * AngularAttenuation * (DiffuseReflection + SpecularReflection) * LightsColorSqR.xyz;

	// directional light
	L = -DirectionalDir.xyz; // directional light direction
	AngularAttenuation = saturate(dot(L, N));
	SpecularReflection = gBaseSpecular.GetSpecular(input.Tex, L, N, CameraPos.xyz - input.WorldPos.xyz); 
	LightsColor = float3(AngularAttenuation, AngularAttenuation, AngularAttenuation); // * (DiffuseReflection + SpecularReflection) * DirectionalCol.xyz;

    return float4(LightsColor, 1);
}

