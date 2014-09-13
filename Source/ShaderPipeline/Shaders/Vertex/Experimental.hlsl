#include "..\..\ConstantBuffers\CBufferView.hlsl"
#include "..\..\ConstantBuffers\CBufferLocal.hlsl"
#include "..\..\ShaderInputs\ExperimentalInputs.hlsl"

//----------------------------------------------------------------------------
PS_INPUT_EXPERIMENTAL Main(VS_INPUT_EXPERIMENTAL input)
{
	PS_INPUT_EXPERIMENTAL output = (PS_INPUT_EXPERIMENTAL)0;
	output.WorldPos = mul(LocalToWorld, input.Pos);
	output.ViewPos = mul(InvViewProjection, output.WorldPos);
	output.Normal = input.Pos.xyz;
	return output;
}
