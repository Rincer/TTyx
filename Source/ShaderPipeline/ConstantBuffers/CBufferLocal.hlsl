#ifndef _CBUFFERLOCAL_H_
#define _CBUFFERLOCAL_H_

cbuffer cbLocalToWorld : register(b2)
{
    matrix LocalToWorld;
    matrix LocalToWorldNormals;    
};

#endif