#include "..\..\ConstantBuffers\CBufferView.hlsl"
#include "..\..\ConstantBuffers\CBufferLights.hlsl"
#include "..\..\ShaderInputs\ExperimentalInputs.hlsl"

#define PI 3.14159265359


float D_GGX( float NdotH, float Roughness)
{
	float m = Roughness * Roughness;
	float m2 = m * m;
	float d = ( NdotH * m2 - NdotH ) * NdotH + 1;	
	return m2 / ( PI*d*d );							
}

float3 F_Schlick( float3 SpecularColor, float VdotH )
{
	// Anything less than 2% is physically impossible and is instead considered to be shadowing 
	return SpecularColor + ( saturate( 50.0 * SpecularColor.g ) - SpecularColor ) * exp2( (-5.55473 * VdotH - 6.98316) * VdotH );
}

float Vis_Smith( float NdotV, float NdotL, float Roughness )
{
	float a = Roughness * Roughness;
	float a2 = a*a;
	float Vis_SmithV = NdotV + sqrt( NdotV * (NdotV - NdotV * a2) + a2 );
	float Vis_SmithL = NdotL + sqrt( NdotL * (NdotL - NdotL * a2) + a2 );
	return rcp( Vis_SmithV * Vis_SmithL );
}

float4 Main(PS_INPUT_EXPERIMENTAL Input) : SV_Target
{          
	float3 N = normalize(Input.Normal);
	float3 V = normalize(CameraPos.xyz - Input.WorldPos.xyz);      
	float3 WorldToLight = LightsPosInvSqR.xyz - Input.WorldPos.xyz; 	
	float3 L = normalize(WorldToLight);	
	float3 R = 2 * dot(V, N) * N - V;   
	float3 H = normalize(L + N);
	float3 Diffuse = BaseColor.xyz - BaseColor.xyz * RSM.zzz;
	float3 Specular = lerp(RSM.yyy, BaseColor.xyz, RSM.z);

	float3 DiffuseReflection = float3(0, 0, 0);
	float3 SpecularReflection = float3(0, 0, 0);
	float3 EnvReflection = float3(0, 0, 0);
	float3 IndirectIrradiance = float3(0, 0, 0);

	float DistanceAttenuation = saturate((LightsColorSqR.w - dot(WorldToLight,WorldToLight)) * LightsPosInvSqR.w);	
	DiffuseReflection  = Diffuse.xyz * LightsColorSqR.xyz;		

	float VdotH = saturate(dot(V, H));
	float NdotV = saturate(dot(N, V));
	float NdotL = saturate(dot(N, L));
	float NdotH = saturate(dot(N, H));

	float3 F = F_Schlick(Specular, VdotH); 
	float D = D_GGX(NdotH, RSM.x);
	float G = Vis_Smith(NdotV, NdotL, RSM.x); 

	IndirectIrradiance = DynEnvMap.SampleLevel(SamplerDynEnvMap, R, 7).xyz;

	EnvReflection = IndirectIrradiance * Specular * DynEnvMap.SampleLevel(SamplerDynEnvMap, R, 8 * RSM.x / 0.8).xyz;
	   
	SpecularReflection = F * D * G * LightsColorSqR.xyz; 
	float3 OutColor = NdotL * DistanceAttenuation * (DiffuseReflection + SpecularReflection) + EnvReflection;
	return float4( OutColor, 1);   
}



 