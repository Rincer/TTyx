#include "..\..\ShaderInputs\UtilityInputs.hlsl"

//--------------------------------------------------------------------------------------
// Plain color 
//--------------------------------------------------------------------------------------
float4 PlainColor_PS( PS_INPUT_PLAINCOLOR input) : SV_Target
{
    return input.Col;
}

//--------------------------------------------------------------------------------------
// Textured
//--------------------------------------------------------------------------------------
float4 Textured_PS( PS_INPUT_TEXTURED input) : SV_Target
{
#if TEXTURE_RGBA
    return input.Col * Texture.Sample(Sampler, input.Tex);
#elif TEXTURE_A // for mask only texture
    return float4(input.Col.rgb, Texture.Sample(Sampler, input.Tex).r);    
#endif
}
