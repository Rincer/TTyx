#ifndef _COLOR_H_
#define _COLOR_H_

class CColor
{
	public:
		CColor();
		CColor(unsigned char A, unsigned char R, unsigned char G, unsigned char B);
		union 
		{
			struct
			{
				unsigned char m_R;
				unsigned char m_G;
				unsigned char m_B;				
				unsigned char m_A;									
			} m_Channels;
			unsigned int m_Argb;
		} m_Color;
};

#endif