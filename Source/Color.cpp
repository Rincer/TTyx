#include "stdafx.h"

#include "Color.h"

//---------------------------------------------------------------------------------------------------
CColor::CColor()
{
	m_Color.m_Argb = 0;
}


//---------------------------------------------------------------------------------------------------
CColor::CColor(unsigned char A, unsigned char R, unsigned char G, unsigned char B)
{
	m_Color.m_Channels.m_A = A;
	m_Color.m_Channels.m_R = R;	
	m_Color.m_Channels.m_G = G;	
	m_Color.m_Channels.m_B = B;	
}
