#include "..\..\ShaderInputs\PostprocessInputs.hlsl"

//--------------------------------------------------------------------------------------
float4 Main( PS_INPUT input) : SV_Target
{
    float4 HDRColor = HDRTexture.Sample(HDRSampler, input.Tex);
	//float4 LDRColor = pow(abs(HDRColor), 1.0f / 2.2f);
	//float L = 0.299f*HDRColor.x + 0.587*HDRColor.y + 0.114f*HDRColor.z;
	//float4 LDRColor = HDRColor * L / (L + 0.187f) * 1.035; 
	float4 LDRColor = HDRColor / (HDRColor + 0.187) * 1.035;
	return LDRColor;
}
