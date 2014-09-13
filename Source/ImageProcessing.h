#ifndef _IMAGEPROCESSING_H_
#define _IMAGEPROCESSING_H_

#include "MemoryManager.h"
#include "JobSystem.h"

class CHeapAllocator;
class CImageConversion;

// This is not the real binary layout, due to alignment issues of member variables
class CTGAHeader
{	
	public:
		CTGAHeader() :	m_IdLength(0),
						m_ColorMapType(0),
						m_ImageType(0),
						m_FirstEntry(0),
						m_NumEntries(0),
						m_EntrySize(0),
						m_XOrigin(0),
						m_YOrigin(0),
						m_Width(0),
						m_Height(0),
						m_BPP(0),
						m_ImageDescriptor(0)
		{
		}
		static const unsigned int scCTGAHeaderSize = 18;
		unsigned int GetRawRGBAImageSize() const;	
		void Deserialize(const void* pData);
		
		unsigned char	m_IdLength;
		unsigned char	m_ColorMapType;
		unsigned char	m_ImageType;
		unsigned short	m_FirstEntry;
		unsigned short	m_NumEntries;
		unsigned char	m_EntrySize;
		unsigned short	m_XOrigin;
		unsigned short	m_YOrigin;
		unsigned short	m_Width;
		unsigned short	m_Height;
		unsigned char	m_BPP;
		unsigned char	m_ImageDescriptor;		
};

#define DDS_HEADER_FLAGS_TEXTURE        0x00001007  // DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT 
#define DDS_HEADER_FLAGS_MIPMAP         0x00020000  // DDSD_MIPMAPCOUNT
#define DDS_HEADER_FLAGS_VOLUME         0x00800000  // DDSD_DEPTH
#define DDS_HEADER_FLAGS_PITCH          0x00000008  // DDSD_PITCH
#define DDS_HEADER_FLAGS_LINEARSIZE     0x00080000  // DDSD_LINEARSIZE

#define DDS_SURFACE_FLAGS_TEXTURE 0x00001000 // DDSCAPS_TEXTURE
#define DDS_SURFACE_FLAGS_MIPMAP  0x00400008 // DDSCAPS_COMPLEX | DDSCAPS_MIPMAP
#define DDS_SURFACE_FLAGS_CUBEMAP 0x00000008 // DDSCAPS_COMPLEX

#define DDS_FOURCC      0x00000004  // DDPF_FOURCC
#define DDS_RGB         0x00000040  // DDPF_RGB
#define DDS_RGBA        0x00000041  // DDPF_RGB | DDPF_ALPHAPIXELS
#define DDS_LUMINANCE   0x00020000  // DDPF_LUMINANCE
#define DDS_ALPHA       0x00000002  // DDPF_ALPHA

class CDDSHeader
{
	public:
		// direct draw surface structs, so that we dont have to include ddraw.h
		struct DDS_PIXELFORMAT
		{
			unsigned int dwSize;
			unsigned int dwFlags;
			unsigned int dwFourCC;
			unsigned int dwRGBBitCount;
			unsigned int dwRBitMask;
			unsigned int dwGBitMask;
			unsigned int dwBBitMask;
			unsigned int dwABitMask;
		};
		
		struct DDS_HEADER
		{
			unsigned int	dwSize;
			unsigned int	dwFlags;
			unsigned int	dwHeight;
			unsigned int	dwWidth;
			unsigned int	dwPitchOrLinearSize;
			unsigned int	dwDepth;
			unsigned int	dwMipMapCount;
			unsigned int	dwReserved1[11];
			DDS_PIXELFORMAT ddspf;
			unsigned int	dwCaps;
			unsigned int	dwCaps2;
			unsigned int	dwCaps3;
			unsigned int	dwCaps4;
			unsigned int	dwReserved2;	
		};
		unsigned int	m_Magic;
		DDS_HEADER      m_header;
};


class CImageProcessing
{
	public:
		
		enum eFormat
		{
			eBMP = 0,		
			eTGA,		
			eDDS,
			eR8G8B8A8,
			eUnknown,					
			eTotalFormats	
		};

		CImageProcessing();
		inline static CImageProcessing& Instance()
		{
			static CImageProcessing s_ImageProcessing;
			return s_ImageProcessing;
		} 
		
		bool RequiresConversion(const unsigned char* pImage) const;
		eFormat GetFileFormat(const unsigned char* pImage) const;
};

#endif