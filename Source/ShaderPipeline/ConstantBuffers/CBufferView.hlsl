#ifndef _CBUFFERVIEW_H_
#define _CBUFFERVIEW_H_
cbuffer cbViewMatrices : register(b0)
{
    matrix InvViewProjection;
    float4 CameraPos;
};

#endif