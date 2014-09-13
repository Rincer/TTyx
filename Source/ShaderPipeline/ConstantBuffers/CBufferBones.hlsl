#ifndef _CBUFFERBONES_H_
#define _CBUFFERBONES_H_

cbuffer cbBones : register(b3)
{
	float4 Bones[256 * 3];	// Must match CSkeleton::scMaxBones, transpose matrices to save a vec4
							// Bones are computed as 4x4 matrices (in place) on the CPU then transposed and 'compressed' into 3x4
							// we provide bone pad for the last matrix when its still a 4x4	
};


#endif