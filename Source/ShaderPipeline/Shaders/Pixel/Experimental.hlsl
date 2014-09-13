#include "..\..\ConstantBuffers\CBufferView.hlsl"
#include "..\..\ConstantBuffers\CBufferLights.hlsl"
#include "..\..\ShaderInputs\ExperimentalInputs.hlsl"

#define PI 3.14159265359

float3 Fresnel(float LdotH)
{
   return Specular.xyz + (float3(1,1,1) - Specular.xyz) * pow(abs(1 - LdotH), 5);  
}

float GGX(float NdotH)
{
    float CosSquared = NdotH*NdotH;
    float TanSquared = (1-CosSquared)/CosSquared;
    float Temp = Roughness.x / (CosSquared * (Roughness.x * Roughness.x + TanSquared));
    return (1.0/PI) * Temp * Temp;
}

float SmithG_GGX(float NdotV)
{
    float a2 = Roughness.x * Roughness.x;
    float b = NdotV*NdotV;
    return 1/(NdotV + sqrt(a2 + b - a2*b));
}

float GTR2(float NdotH)
{
    float a2 = Roughness.x *Roughness.x;
    float t = 1 + (a2-1)*NdotH*NdotH;
    return a2 / (PI * t*t);
}


float4 Main(PS_INPUT_EXPERIMENTAL Input) : SV_Target
{          
	float3 N = normalize(Input.Normal);
	float3 V = normalize(CameraPos.xyz - Input.WorldPos.xyz);      
	float3 WorldToLight = LightsPosInvSqR.xyz - Input.WorldPos.xyz; 	
	float3 L = normalize(WorldToLight);	
	float3 R = 2 * dot(V, N) * N - V;   
	float3 H = normalize(L + N);
	float LdotH = dot(L,H);

	float3 DiffuseReflection = float3(0, 0, 0);
	float3 SpecularReflection = float3(0, 0, 0);
	float AngularAttenuation = saturate(dot(L, N));
	float DistanceAttenuation = saturate((LightsColorSqR.w - dot(WorldToLight,WorldToLight)) * LightsPosInvSqR.w);								
	DiffuseReflection  = AngularAttenuation * DistanceAttenuation * Diffuse.xyz * LightsColorSqR.xyz;		


	float Normalization = 4 * dot(N, V) * dot(N, L);
	float3 F = Fresnel(LdotH); // Normalization; 
	float D = SmithG_GGX(dot(N, V));
	float G = GTR2(dot(N, H));
	float3 Reflection = DynEnvMap.SampleLevel(SamplerDynEnvMap, R, 8 * Roughness.x / 0.8).xyz;
	
   
	float3 TotalSpecular = F * D * G / Normalization * LightsColorSqR.xyz; 
//	float3 FinalSpecular = TotalSpecular * Reflection;
//	float3 Final = DiffRef * Diffuse.w + FinalSpecular * (1 - Diffuse.w);
//	float3 Final = DiffRef + FinalSpecular;
	return( float4(DiffuseReflection + TotalSpecular + Reflection, 1) );   
//	return( float4(Reflection, 1));
//	return( float4(TotalSpecular, 1));
}



 