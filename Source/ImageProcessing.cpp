#include "stdafx.h"
#include "HeapAllocator.h"
#include "Thread.h"
#include "MemoryStream.h"

#include "ImageProcessing.h"


//-------------------------------------------------------------------------
unsigned int CTGAHeader::GetRawRGBAImageSize() const
{
	return m_Height * m_Width * m_BPP / 8;
}

//-------------------------------------------------------------------------
void CTGAHeader::Deserialize(const void* pData)
{
	CMemoryStreamReader Stream(pData, 0, scCTGAHeaderSize);  
	m_IdLength = Stream.Read<unsigned char>();
	m_ColorMapType = Stream.Read<unsigned char>();
	m_ImageType = Stream.Read<unsigned char>();
	m_FirstEntry = Stream.Read<unsigned short>();
	m_NumEntries = Stream.Read<unsigned short>();
	m_EntrySize = Stream.Read<unsigned char>();
	m_XOrigin = Stream.Read<unsigned short>();
	m_YOrigin = Stream.Read<unsigned short>();
	m_Width = Stream.Read<unsigned short>();
	m_Height = Stream.Read<unsigned short>();
	m_BPP = Stream.Read<unsigned char>();
	m_ImageDescriptor = Stream.Read<unsigned char>();					
}

//----------------------------------------------------------------------------------
CImageProcessing::CImageProcessing()
{
}
		
//----------------------------------------------------------------------------------
bool CImageProcessing::RequiresConversion(const unsigned char* pImage) const
{
	return (GetFileFormat(pImage) == CImageProcessing::eTGA);
}

//----------------------------------------------------------------------------------
CImageProcessing::eFormat CImageProcessing::GetFileFormat(const unsigned char* pImage) const
{
	if((pImage[0] == 'B') && (pImage[1] == 'M'))
	{
		return CImageProcessing::eBMP;
	}
	// TGA-------------------------------------------------------------------------------
	else if ((pImage[1] == 0) && (pImage[2] == 2)) // Uncompressed true color image .tga RGB
	{
		return CImageProcessing::eTGA;
	}
	else if ((pImage[1] == 0) && (pImage[2] == 3)) // Uncompressed true color image .tga Greyscale
	{
		return CImageProcessing::eTGA;
	}	
	else if ((pImage[0] == 'D') && (pImage[1] == 'D') && (pImage[2] == 'S')) // .dds
	{
		return CImageProcessing::eDDS;
	}		
	else
	{
		Assert(0); // Unhandled format
	}
	return CImageProcessing::eUnknown;
}

