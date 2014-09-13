#ifndef _CBUFFERLIGHTS_H_
#define _CBUFFERLIGHTS_H_
cbuffer cbLights : register(b1)
{
    float4 LightsColorSqR;		// r, g, b, sqr(Radius)
    float4 LightsPosInvSqR;		// x, y, z, 1 / sqr(Radius)
};

#endif