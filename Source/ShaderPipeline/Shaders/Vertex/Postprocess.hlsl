#include "..\..\ShaderInputs\PostprocessInputs.hlsl"

//----------------------------------------------------------------------------
PS_INPUT Main( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;
	output.ViewPos = input.Pos;
    output.Tex = input.Tex;
    return output;
}
