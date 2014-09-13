#include "stdafx.h"
#include "Macros.h"
#include "JobSystem.h"
#include "LoadingSystem.h"
#include "FileSystem.h"
#include "ImageProcessing.h"
#include "Renderer.h"
#include "TextureSystem.h"
#include "Math.h"

#include "JobLoadTexture.h"

//---------------------------------------------------------------------------------------------
CJobLoadTexture::CJobLoadTexture(unsigned int Id, CJobSystem::eJobPriority Priority, CJobSystem* pJobSystem) : CJobSystem::CJob(Id, Priority, pJobSystem)
{
}

//---------------------------------------------------------------------------------------------
CJobLoadTexture::CJobLoadTexture(CTexture* pTexture, const char* pName, CRenderer* pRenderer, CLoadingSystem* pLoadingSystem) : CJobSystem::CJob(CJobSystem::eJobLoadTexture),
																																m_pTexture(pTexture),
																																m_pRenderer(pRenderer),
																																m_pLoadingSystem(pLoadingSystem)
{
	strcpy_s(m_Name, 255, pName);
} 
 
//-----------------------------------------------------------------------------
void CJobLoadTexture::TGA_To_DDS(unsigned char* pDstData, unsigned int DstPitch, const unsigned char* pSrcData)
{
	CDDSHeader* pDDSHeader = (CDDSHeader*)pDstData;
	CTGAHeader TGAHeader;
	TGAHeader.Deserialize(pSrcData);			
	memset(pDDSHeader, 0, sizeof(CDDSHeader));
	pDDSHeader->m_Magic = 0x20534444; // 'DDS '
	pDDSHeader->m_header.dwSize = 124;
	pDDSHeader->m_header.dwFlags = DDS_HEADER_FLAGS_TEXTURE;
	pDDSHeader->m_header.dwWidth = TGAHeader.m_Width;
	pDDSHeader->m_header.dwHeight = TGAHeader.m_Height;
	pDDSHeader->m_header.dwCaps = DDS_SURFACE_FLAGS_TEXTURE;
	pDDSHeader->m_header.ddspf.dwSize = 32;
	switch(TGAHeader.m_BPP)
	{
		case 24:
			pDDSHeader->m_header.ddspf.dwFlags = DDS_RGB;
			pDDSHeader->m_header.ddspf.dwRGBBitCount = 24;
			pDDSHeader->m_header.ddspf.dwRBitMask = 0x00FF0000;
			pDDSHeader->m_header.ddspf.dwGBitMask = 0x0000FF00;			
			pDDSHeader->m_header.ddspf.dwBBitMask = 0x000000FF;											
			break;
 
		default:
			Assert(0); // unhandled
			break;					
	}
	unsigned int SrcPitch = TGAHeader.m_Width * TGAHeader.m_BPP / 8;
	const unsigned char* pSrc = pSrcData + CTGAHeader::scCTGAHeaderSize + SrcPitch * (TGAHeader.m_Height - 1);
	unsigned char* pDst = pDstData + sizeof(CDDSHeader);
	for (unsigned int Row = 0; Row < TGAHeader.m_Height; Row++)
	{
		memcpy(pDst, pSrc, SrcPitch);
		pDst += DstPitch;
		pSrc -= SrcPitch;
	}
}
   
//-----------------------------------------------------------------------------
void CJobLoadTexture::TGA_To_DDSGreyscale(unsigned char* pDstData, unsigned int DstPitch, const unsigned char* pSrcData)
{
	CDDSHeader* pDDSHeader = (CDDSHeader*)pDstData;
	CTGAHeader TGAHeader;
	TGAHeader.Deserialize(pSrcData);			
	memset(pDDSHeader, 0, sizeof(CDDSHeader));
	pDDSHeader->m_Magic = 0x20534444; // 'DDS '
	pDDSHeader->m_header.dwSize = 124;
	pDDSHeader->m_header.dwFlags = DDS_HEADER_FLAGS_TEXTURE;
	pDDSHeader->m_header.dwWidth = TGAHeader.m_Width;
	pDDSHeader->m_header.dwHeight = TGAHeader.m_Height;
	pDDSHeader->m_header.dwCaps = DDS_SURFACE_FLAGS_TEXTURE;
	pDDSHeader->m_header.ddspf.dwSize = 32;
	pDDSHeader->m_header.ddspf.dwFlags = DDS_ALPHA;
	pDDSHeader->m_header.ddspf.dwRGBBitCount = 8;
	pDDSHeader->m_header.ddspf.dwABitMask = 0xFF;
	
	unsigned int SrcPitch = TGAHeader.m_Width * TGAHeader.m_BPP / 8;
	const unsigned char* pSrc = pSrcData + CTGAHeader::scCTGAHeaderSize + SrcPitch * (TGAHeader.m_Height - 1);
	unsigned char* pDst = pDstData + sizeof(CDDSHeader);
	for (unsigned int Row = 0; Row < TGAHeader.m_Height; Row++)
	{
		memcpy(pDst, pSrc, SrcPitch);
		pDst += DstPitch;
		pSrc -= SrcPitch;
	}
}

//-----------------------------------------------------------------------------
void CJobLoadTexture::TGA24_To_R8G8B8A8(unsigned char* pDstData, const unsigned char* pSrcData, D3D11_TEXTURE2D_DESC& Desc)
{
	unsigned int SrcPitch = 3 * Desc.Width;
	const unsigned char* pSrc = pSrcData + SrcPitch * (Desc.Height - 1);
	unsigned char* pDst = pDstData;
	unsigned int DstPitch = Desc.Width * 4;
	for (unsigned int Row = 0; Row < Desc.Height; Row++)
	{
		const unsigned char* pSrcPixel = pSrc;
		unsigned char* pDstPixel = pDst;
		for (unsigned int Col = 0; Col < Desc.Width; Col++)
		{
			pDstPixel[0] = pSrcPixel[2];
			pDstPixel[1] = pSrcPixel[1];
			pDstPixel[2] = pSrcPixel[0];
			pDstPixel[3] = 0;
			pDstPixel += 4;
			pSrcPixel += 3;
		}
		pDst += DstPitch;
		pSrc -= SrcPitch;
	}
}

//-----------------------------------------------------------------------------
void CJobLoadTexture::TGA8_To_R8G8B8A8(unsigned char* pDstData, const unsigned char* pSrcData, D3D11_TEXTURE2D_DESC& Desc)
{
	unsigned int SrcPitch = Desc.Width;
	const unsigned char* pSrc = pSrcData + SrcPitch * (Desc.Height - 1);
	unsigned char* pDst = pDstData;
	unsigned int DstPitch = Desc.Width * 4;
	for (unsigned int Row = 0; Row < Desc.Height; Row++)
	{
		const unsigned char* pSrcPixel = pSrc;
		unsigned char* pDstPixel = pDst;
		for (unsigned int Col = 0; Col < Desc.Width; Col++)
		{
			pDstPixel[0] = pSrcPixel[0];
			pDstPixel[1] = pSrcPixel[0];
			pDstPixel[2] = pSrcPixel[0];
			pDstPixel[3] = pSrcPixel[0];
			pDstPixel += 4;
			pSrcPixel++;
		}
		pDst += DstPitch;
		pSrc -= SrcPitch;
	}
}

//-----------------------------------------------------------------------------
void CJobLoadTexture::TGA8_To_A8(unsigned char* pDstData, const unsigned char* pSrcData, D3D11_TEXTURE2D_DESC& Desc)
{
	unsigned int SrcPitch = Desc.Width;
	const unsigned char* pSrc = pSrcData + SrcPitch * (Desc.Height - 1);
	unsigned char* pDst = pDstData;
	unsigned int DstPitch = Desc.Width;
	for (unsigned int Row = 0; Row < Desc.Height; Row++)
	{
		const unsigned char* pSrcPixel = pSrc;
		unsigned char* pDstPixel = pDst;
		for (unsigned int Col = 0; Col < Desc.Width; Col++)
		{
			pDstPixel[0] = pSrcPixel[0];
			pDstPixel++;
			pSrcPixel++;
		}
		pDst += DstPitch;
		pSrc -= SrcPitch;
	}
}

//-----------------------------------------------------------------------------
void CJobLoadTexture::DDSB8G8R8A8_To_R8G8B8A8(unsigned char* pDstData, const unsigned char* pSrcData, D3D11_TEXTURE2D_DESC& Desc)
{
	const unsigned char* pSrc = pSrcData;
	unsigned char* pDst = pDstData;
	for (unsigned int Row = 0; Row < Desc.Height; Row++)
	{
		for (unsigned int Col = 0; Col < Desc.Width; Col++)
		{
			pDst[0] = pSrc[2];
			pDst[1] = pSrc[1];
			pDst[2] = pSrc[0];
			pDst[3] = pSrc[3];
			pDst += 4;
			pSrc += 4;
		}
	}
}

//-----------------------------------------------------------------------------
CJobLoadTexture::CConversionParameters CJobLoadTexture::GetConversionParameters(CImageProcessing::eFormat DstFormat, const unsigned char* pSrcImage)
{	
	unsigned int ImageWidth = 0;
	unsigned int ImageHeight = 0;
	unsigned int ImageBPP = 0;
	CTGAHeader TGAHeader;		
	CConversionParameters ConversionParameters;
	switch(CImageProcessing::Instance().GetFileFormat(pSrcImage))
	{
		case CImageProcessing::eTGA:
		{
			TGAHeader.Deserialize(pSrcImage);
			// Only handle non palettized, non inverted, non compressed 24 bpp or 8bpp greyscale images for now
			Assert(TGAHeader.m_ColorMapType == 0);												
			Assert(((TGAHeader.m_ImageType == 2) && (TGAHeader.m_BPP == 24)) || 
				  ((TGAHeader.m_ImageType == 3) && (TGAHeader.m_BPP == 8)));												
			Assert(TGAHeader.m_XOrigin == 0);							
			Assert(TGAHeader.m_YOrigin == 0);	
			ImageWidth = TGAHeader.m_Width;		
			ImageHeight = TGAHeader.m_Height;		
			ImageBPP = TGAHeader.m_BPP;					
			break;
		}
		
		default:
		{
			Assert(0); // Conversion is not supported
			break;
		}
	}
				
	switch(DstFormat)
	{
		case CImageProcessing::eDDS:
		{
			ConversionParameters.m_DstImagePitch = (ImageWidth * ImageBPP + 7) / 8;
			ConversionParameters.m_DstImageSize = ConversionParameters.m_DstImagePitch * ImageHeight + sizeof(CDDSHeader);
			break;
		}

		case CImageProcessing::eR8G8B8A8:
		{
			ConversionParameters.m_DstImagePitch = ImageWidth * 4;
			ConversionParameters.m_DstImageSize = ConversionParameters.m_DstImagePitch * ImageHeight;
			break;
		}

		default:
		{
			Assert(0); // Conversion is not supported
			break;
		}				
	}
	
	if(TGAHeader.m_ImageType == 2)
		ConversionParameters.m_ConversionType = eTgaToDDS;
	else if(TGAHeader.m_ImageType == 3)
		ConversionParameters.m_ConversionType = eTgaToDDSGreyscale;
	else 
		Assert(0); // Need to write some more conversion code
	return ConversionParameters;
}

//---------------------------------------------------------------------------------------------
CJobLoadTexture::CConversionParameters CJobLoadTexture::GetConversionParameters(DXGI_FORMAT DstFormat, const unsigned char* pSrcImage)
{
	unsigned int ImageBPP = 0;
	CTGAHeader TGAHeader;
	CConversionParameters ConversionParameters;
	switch (CImageProcessing::Instance().GetFileFormat(pSrcImage))
	{
		case CImageProcessing::eTGA:
		{
			TGAHeader.Deserialize(pSrcImage);
			// Only handle non palettized, non inverted, non compressed 24 bpp or 8bpp greyscale images for now
			Assert(TGAHeader.m_ColorMapType == 0);
			Assert(((TGAHeader.m_ImageType == 2) && (TGAHeader.m_BPP == 24)) ||
				((TGAHeader.m_ImageType == 3) && (TGAHeader.m_BPP == 8)));
			Assert(TGAHeader.m_XOrigin == 0);
			Assert(TGAHeader.m_YOrigin == 0);
			ConversionParameters.m_Width = TGAHeader.m_Width;
			ConversionParameters.m_Height = TGAHeader.m_Height;
			ImageBPP = TGAHeader.m_BPP;
			switch (DstFormat)
			{
				case DXGI_FORMAT_R8G8B8A8_TYPELESS:
				{
					if (TGAHeader.m_ImageType == 2)
					{
						ConversionParameters.m_ConversionType = eTga24ToR8G8B8A8;
					}
					else if (TGAHeader.m_ImageType == 3)
					{
						ConversionParameters.m_ConversionType = eTga8ToR8G8B8A8;
					}
					else
					{
						Assert(0);
					}
					break;
				}

				case DXGI_FORMAT_R8_TYPELESS:
				{
					Assert(TGAHeader.m_ImageType == 3);
					ConversionParameters.m_ConversionType = eTga8ToA8;
					break;
				}

				default:
				{
					Assert(0); // Conversion is not supported
					break;
				}
			}
			break;
		}

		case CImageProcessing::eDDS:
		{
			CDDSHeader* pDDSHeader = (CDDSHeader*)pSrcImage;
			Assert(pDDSHeader->m_header.ddspf.dwFlags & DDS_RGB);
			Assert(pDDSHeader->m_header.ddspf.dwRGBBitCount == 32);
			Assert(pDDSHeader->m_header.ddspf.dwRBitMask == 0xFF0000);
			Assert(pDDSHeader->m_header.ddspf.dwGBitMask == 0x00FF00);
			Assert(pDDSHeader->m_header.ddspf.dwBBitMask == 0x0000FF);
			ConversionParameters.m_Width = pDDSHeader->m_header.dwWidth;
			ConversionParameters.m_Height = pDDSHeader->m_header.dwHeight;
			switch (DstFormat)
			{
				case DXGI_FORMAT_R8G8B8A8_TYPELESS:
				{
					ConversionParameters.m_ConversionType = eDDSB8G8R8A8ToR8G8B8A8;
					break;
				}
				default:
				{
					Assert(0); // Conversion is not supported
					break;
				}

			}
			break;
		}

		default:
		{
			Assert(0); // Conversion is not supported
			break;
		}
	}

	return ConversionParameters;
}


//---------------------------------------------------------------------------------------------
unsigned int CJobLoadTexture::Execute(unsigned int ThreadID)
{
	unsigned char Header[sizeof(CDDSHeader)]; // Biggest header structure, cant use sizeof(CTGAHeader) due to data alignment of it's members
	unsigned int HeaderSize = sizeof(CDDSHeader);
	HANDLE File = CFileSystem::Open(m_Name);
	CFileSystem::Read(Header, HeaderSize, File); // read header data to determine what kind of file format we are reading
	CConversionParameters ConversionParameters;
	unsigned int FileSize = GetFileSize(File, NULL); 
	unsigned int AlignedFileSize = POW2ALIGN(FileSize, 16); // so that destination image starts on an aligned boundary
	unsigned int ImageDataSize = 0;

	ConversionParameters = GetConversionParameters(m_pTexture->m_Desc.Format, Header);
	if (m_pTexture->m_Desc.MipLevels != 1)
	{
		unsigned int LargestDimension = MAX(ConversionParameters.m_Width, ConversionParameters.m_Height);
		Assert(IsPowerOf2(LargestDimension));											// Mipmapped textures can only be power of 2 sized	
		Assert(m_pTexture->m_Desc.MipLevels < Log2(LargestDimension) + 1);
		m_pTexture->m_Desc.MipLevels = m_pTexture->m_Desc.MipLevels ? m_pTexture->m_Desc.MipLevels : Log2(LargestDimension) + 1; // If 0 set to log2 of largest dimension
	}
	m_pTexture->m_Desc.Width = ConversionParameters.m_Width;
	m_pTexture->m_Desc.Height = ConversionParameters.m_Height;
	m_pTexture->m_Desc.ArraySize = 1;
	m_pTexture->m_Desc.SampleDesc.Count = 1;
	m_pTexture->m_Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE; 
	if (m_pTexture->m_Desc.MipLevels > 1)
	{
		m_pTexture->m_Desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
	}

	ImageDataSize = m_pTexture->GetSubresourceDataSize(0, 0);
	m_Size = AlignedFileSize + ImageDataSize;

	// Allocate enough space for both original data and conversion data
	CLoadingSystem::CBlockingCondition BlockingCondition((void**)&m_pData, m_Size, this, m_pLoadingSystem);
	if(BlockingCondition.IsBlocked())
	{
		m_pJobSystem->YieldExecution(m_Id, m_Type, ThreadID, &BlockingCondition);
	}

	memcpy(m_pData, Header, HeaderSize); // Copy the header
	CFileSystem::Read(&m_pData[HeaderSize], FileSize - HeaderSize, File); // Read the rest of the data
	CFileSystem::Close(File);

	if (ConversionParameters.m_ConversionType == eTga24ToR8G8B8A8)
	{
		TGA24_To_R8G8B8A8(&m_pData[AlignedFileSize], &m_pData[CTGAHeader::scCTGAHeaderSize], m_pTexture->m_Desc); 
	}
	else if (ConversionParameters.m_ConversionType == eTga8ToR8G8B8A8)
	{
		TGA8_To_R8G8B8A8(&m_pData[AlignedFileSize], &m_pData[CTGAHeader::scCTGAHeaderSize], m_pTexture->m_Desc);
	}
	else if (ConversionParameters.m_ConversionType == eTga8ToA8)
	{
		TGA8_To_A8(&m_pData[AlignedFileSize], &m_pData[CTGAHeader::scCTGAHeaderSize], m_pTexture->m_Desc);
	}
	else if (ConversionParameters.m_ConversionType == eDDSB8G8R8A8ToR8G8B8A8)
	{
		DDSB8G8R8A8_To_R8G8B8A8(&m_pData[AlignedFileSize], &m_pData[sizeof(CDDSHeader)], m_pTexture->m_Desc);
	}
	else
	{
		Assert(0);
	}

	if (m_pTexture->m_Desc.MipLevels == 1)
	{
		m_pTexture->CreateTexture2D(m_pRenderer->GetDevice(), &m_pData[AlignedFileSize], m_Name);
	}
	else
	{
		m_pTexture->CreateTexture2D_Compute(m_pRenderer->GetDevice(), &m_pData[AlignedFileSize], m_Name);
	}

	/*
	if(RequiresConversion)
	{
		if(ConversionParameters.m_ConversionType == eTgaToDDS)
			TGA_To_DDS(&m_pData[AlignedFileSize], ConversionParameters.m_DstImagePitch, m_pData);
		else if(ConversionParameters.m_ConversionType == eTgaToDDSGreyscale)
			TGA_To_DDSGreyscale(&m_pData[AlignedFileSize], ConversionParameters.m_DstImagePitch, m_pData);
		else 
			Assert(0);
		m_pTexture->CreateFromMemory(m_pRenderer->GetDevice(), &m_pData[AlignedFileSize], ImageDataSize, m_Name);			
	}
	else
	{
		m_pTexture->CreateFromMemory(m_pRenderer->GetDevice(), m_pData, FileSize, m_Name);

	}
	*/
	m_pLoadingSystem->Free(m_pData, m_Size);
	return 0;
}

//---------------------------------------------------------------------------------------------
CJobLoadTexture::~CJobLoadTexture()
{
}

