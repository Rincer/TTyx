#ifndef _CBUFFERLIGHTS_H_
#define _CBUFFERLIGHTS_H_
cbuffer cbLights : register(b1)
{
	// point light
    float4 LightsColorSqR;	// r, g, b, sqr(Radius)
    float4 LightsPosInvSqR;	// x, y, z, 1 / sqr(Radius)
	// directional light
	float4 DirectionalDir;	// directional light direction
	float4 DirectionalCol;	// directional light color	
};

#endif