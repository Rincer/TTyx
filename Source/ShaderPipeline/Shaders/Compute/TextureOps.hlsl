
ByteAddressBuffer	LinearData;			// Input buffer
Texture2D<float4>	InTexture;			// Input texture in the case of downsampling
RWTexture2D<float4> OutTexture;			// Output texture
Texture2DArray<float4>	InTextureCube;		// Cube faces 
RWTexture2DArray<float4> OutTextureCube;

SamplerState		InSampler;	// Sampler for the input texture

cbuffer TextureParams
{
	float4 PixelsToUVs; // 1/width, 1/height, width
};

//-----------------------------------------------------------------------------------------
// Utility function
float4 Uint_To_Float_RGBA(uint RGBA)
{
	float4 Res;
	Res.x = (float)(RGBA & 0xFF) / 255.0f;
	Res.y = (float)((RGBA >> 8) & 0xFF) / 255.0f;
	Res.z = (float)((RGBA >> 16) & 0xFF) / 255.0f;
	Res.w = (float)((RGBA >> 24) & 0xFF) / 255.0f;
	return Res;
}

float4 SampleLinear(uint x, uint y)
{
	uint index = (uint)(PixelsToUVs.z * y + x);
	uint PixelU = asuint(LinearData.Load(index * 4));	// addressing is in bytes
	float4	PixelF = Uint_To_Float_RGBA(PixelU);
	return PixelF;
}

float4 SampleBox2x2(uint x, uint y)
{
	float4 Pixel0 = InTexture.SampleLevel(InSampler, float2(2 * x, 2 * y) * PixelsToUVs.xy, 0);
	float4 Pixel1 = InTexture.SampleLevel(InSampler, float2(2 * x + 1, 2 * y) * PixelsToUVs.xy, 0);
	float4 Pixel2 = InTexture.SampleLevel(InSampler, float2(2 * x, 2 * y + 1) * PixelsToUVs.xy, 0);
	float4 Pixel3 = InTexture.SampleLevel(InSampler, float2(2 * x + 1, 2 * y + 1) * PixelsToUVs.xy, 0);
	return (Pixel0 + Pixel1 + Pixel2 + Pixel3) / 4.0f;
}

float4 SampleArrayBox2x2(uint x, uint y)
{
	float4 Pixel0 = InTextureCube.SampleLevel(InSampler, float3(float2(2 * x, 2 * y) * PixelsToUVs.xy, 0), 0);
	float4 Pixel1 = InTextureCube.SampleLevel(InSampler, float3(float2(2 * x + 1, 2 * y) * PixelsToUVs.xy, 0), 0);
	float4 Pixel2 = InTextureCube.SampleLevel(InSampler, float3(float2(2 * x, 2 * y + 1) * PixelsToUVs.xy, 0), 0);
	float4 Pixel3 = InTextureCube.SampleLevel(InSampler, float3(float2(2 * x + 1, 2 * y + 1) * PixelsToUVs.xy, 0), 0);
	return (Pixel0 + Pixel1 + Pixel2 + Pixel3) / 4.0f;
}

[numthreads(1, 1, 1)]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{
	uint x = DTid.x;
	uint y = DTid.y;
#if (BYTEADDRESS_TO_TEXTURE)
	OutTexture[uint2(x,y)] = SampleLinear(x, y);
#elif (DOWNSAMPLE_2x2BOX)
	OutTexture[uint2(x, y)] = SampleBox2x2(x, y);
#elif(DOWNSAMPLECUBE_2x2BOX)
	OutTextureCube[uint3(x, y, 0)] = SampleArrayBox2x2(x, y);
#endif

}