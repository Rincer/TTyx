#ifndef _JOBLOADTEXTURE_H_
#define _JOBLOADTEXTURE_H_

#include "ImageProcessing.h"

class CTexture;
class CRenderer;
class CLoadingSystem;

class CJobLoadTexture : public CJobSystem::CJob
{
	public:
		CJobLoadTexture(unsigned int Id, CJobSystem::eJobPriority Priority, CJobSystem* pJobSystem);
		CJobLoadTexture(CTexture* pTexture, const char* pName, CRenderer* pRenderer, CLoadingSystem* pLoadingSystem);
		virtual ~CJobLoadTexture();
		virtual unsigned int Execute(unsigned int ThreadID); 
//		virtual void OnComplete() {} // Override tio do nothing, since data is needed until texture is created, texture system will release this job manually.

	private:
		enum eConversionType
		{
			eTgaToDDS,
			eTgaToDDSGreyscale,
			eTga24ToR8G8B8A8,
			eTga8ToR8G8B8A8,
			eTga8ToA8,
			eDDSB8G8R8A8ToR8G8B8A8,
		};
		
		class CConversionParameters
		{
			public:
				CConversionParameters() :	m_ConversionType(eTgaToDDS),
											m_DstImagePitch(0),
											m_DstImageSize(0)
				{
				}
				unsigned int	m_Width;
				unsigned int	m_Height;
				eConversionType m_ConversionType;
				unsigned int	m_DstImagePitch;
				unsigned int	m_DstImageSize;
		};

		CConversionParameters GetConversionParameters(CImageProcessing::eFormat DstFormat, const unsigned char* pSrcImage);
		CConversionParameters GetConversionParameters(DXGI_FORMAT DstFormat, const unsigned char* pSrcImage);
		void TGA_To_DDS(unsigned char* pDstData, unsigned int DstPitch, const unsigned char* pSrcData);
		void TGA_To_DDSGreyscale(unsigned char* pDstData, unsigned int DstPitch, const unsigned char* pSrcData);
		void TGA24_To_R8G8B8A8(unsigned char* pDstData, const unsigned char* pSrcData, D3D11_TEXTURE2D_DESC& Desc);
		void TGA8_To_R8G8B8A8(unsigned char* pDstData, const unsigned char* pSrcData, D3D11_TEXTURE2D_DESC& Desc);
		void TGA8_To_A8(unsigned char* pDstData, const unsigned char* pSrcData, D3D11_TEXTURE2D_DESC& Desc);
		void DDSB8G8R8A8_To_R8G8B8A8(unsigned char* pDstData, const unsigned char* pSrcData, D3D11_TEXTURE2D_DESC& Desc);

		char m_Name[256];
		CTexture* m_pTexture;
		unsigned char* m_pData;
		unsigned int   m_Size;
		CRenderer* m_pRenderer;
		CLoadingSystem* m_pLoadingSystem;

};


#endif