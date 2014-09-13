#include "..\..\ConstantBuffers\CBufferView.hlsl"
#include "..\..\ConstantBuffers\CBufferLocal.hlsl"
#include "..\..\ShaderInputs\UtilityInputs.hlsl"

//----------------------------------------------------------------------------
PS_INPUT_PLAINCOLOR PlainColor_VS( VS_INPUT input )
{
    PS_INPUT_PLAINCOLOR output = (PS_INPUT_PLAINCOLOR)0;
#if WORLDSPACE    
    output.ViewPos = mul( InvViewProjection, mul(LocalToWorld, input.Pos));
#elif VIEWSPACE
	output.ViewPos = input.Pos;
#endif
    output.Col = input.Col;    
    return output;
}

//----------------------------------------------------------------------------
PS_INPUT_TEXTURED Textured2D_VS( VS_INPUT input )
{
    PS_INPUT_TEXTURED output = (PS_INPUT_TEXTURED)0;
    output.ViewPos = input.Pos;
    output.Tex = input.Tex;
    output.Col = input.Col;    
    return output;
}

