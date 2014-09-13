#include "stdafx.h"
#include "LoadingSystem.h"
#include "TextReader.h"
#include "FileSystem.h"
#include "MaterialSystem.h"
#include "TextureSystem.h"
#include "MaterialPhong.h"

#include "JobLoadMaterialLibrary.h"

static const char* scObjMaterialPath = "MaterialLibraries\\";	

//-------------------------------------------------------------------------------
CJobLoadMaterialLibrary::CJobLoadMaterialLibrary(unsigned int Id, CJobSystem::eJobPriority Priority, CJobSystem* pJobSystem) : CJobSystem::CJob(Id, Priority, pJobSystem)
{
}

//-------------------------------------------------------------------------------
CJobLoadMaterialLibrary::CJobLoadMaterialLibrary(const char* pName, CLoadingSystem * pLoadingSystem,
																	CTextureSystem* pTextureSystem,
																	CMaterialSystem* pMaterialSystem,
																	CMaterialPhong::eLocalToWorldTransform LocalToWorldTransform) : CJobSystem::CJob(CJobSystem::eJobLoadMaterialLibrary),
																										m_pLoadingSystem(pLoadingSystem),
																										m_pTextureSystem(pTextureSystem),
																										m_pMaterialSystem(pMaterialSystem),
																										m_LocalToWorldTransform(LocalToWorldTransform)
{
	strcpy_s(m_Name, 255, pName);
}

//-------------------------------------------------------------------------------
CJobLoadMaterialLibrary::~CJobLoadMaterialLibrary()
{
}

//-------------------------------------------------------------------------------
unsigned int CJobLoadMaterialLibrary::Execute(unsigned int ThreadID)
{
	char FullPathName[256];
	strcpy_s(FullPathName, 255, scObjMaterialPath);
	strcat_s(FullPathName, 255, m_Name);
	HANDLE File = CFileSystem::Open(FullPathName);
	unsigned int Size = GetFileSize(File, NULL);
		
	unsigned char* pData;
	CLoadingSystem::CBlockingCondition BlockingCondition((void**)&pData, Size, this, m_pLoadingSystem);
	if(BlockingCondition.IsBlocked())
	{
		m_pJobSystem->YieldExecution(m_Id, m_Type, ThreadID, &BlockingCondition);
	}
	
	CFileSystem::Read(pData, Size, File);
	CFileSystem::Close(File);
	
		
	const char* pDataStart = (const char*)pData;
	const char* pDataEnd = (const char*)&pData[Size];			
	
	char  MtlName[256];
	char  DiffuseTextureName[256];
	char  SpecularTextureName[256]; 			
	char  AlphaTextureName[256]; 			
	char  NormalTextureName[256];
	XMVECTORF32 DiffuseParameters;
	XMVECTORF32 SpecularParameters;
				
	SpecularParameters.v = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	MtlName[0] = 0;
	DiffuseTextureName[0] = 0;
	NormalTextureName[0] = 0;
	SpecularTextureName[0] = 0; 
	AlphaTextureName[0] = 0; 
	bool FirstParameters = true;			
	while((pDataStart != pDataEnd))// && (!m_ForceExit)) Todo
	{
		CTextReader::SkipSpacesEOLAndTab(&pDataStart);	
		if(pDataStart >= pDataEnd)
		{
			break;		
		}				
		// newmtl
		if((pDataStart[0] == 'n') && (pDataStart[1] == 'e') && (pDataStart[2] =='w') && (pDataStart[3] == 'm') && (pDataStart[4] == 't') && (pDataStart[5] == 'l'))
		{
			if(FirstParameters)
			{
				FirstParameters = false;
			}
			else
			{
				AddMaterial(MtlName, DiffuseTextureName, SpecularTextureName, AlphaTextureName, NormalTextureName, DiffuseParameters, SpecularParameters);
				MtlName[0] = 0;
				DiffuseTextureName[0] = 0;
				NormalTextureName[0] = 0;
				SpecularTextureName[0] = 0; 
				AlphaTextureName[0] = 0; 						
			}
			pDataStart += 6;
			CTextReader::ReadString(MtlName, &pDataStart);
		}

		// Diffuse color
		else if((pDataStart[0] == 'K') && (pDataStart[1] == 'd'))
		{
			pDataStart += 2;
			CTextReader::ReadFloat(DiffuseParameters.f[0], &pDataStart);
			CTextReader::ReadFloat(DiffuseParameters.f[1], &pDataStart);					
			CTextReader::ReadFloat(DiffuseParameters.f[2], &pDataStart);					
		}
		
		// Specular power
		else if((pDataStart[0] == 'N') && (pDataStart[1] == 's'))
		{
			pDataStart += 2;
			CTextReader::ReadFloat(SpecularParameters.f[3], &pDataStart);
		}
										
		// Specular color
		else if((pDataStart[0] == 'K') && (pDataStart[1] == 's'))
		{
			pDataStart += 2;
			CTextReader::ReadFloat(SpecularParameters.f[0], &pDataStart);
			CTextReader::ReadFloat(SpecularParameters.f[1], &pDataStart);					
			CTextReader::ReadFloat(SpecularParameters.f[2], &pDataStart);					
		}
		
		// map_Kd
		else if((pDataStart[0] == 'm') && (pDataStart[1] == 'a') && (pDataStart[2] =='p') && (pDataStart[3] == '_') && (pDataStart[4] == 'K') && (pDataStart[5] == 'd'))
		{
			pDataStart += 6;
			CTextReader::ReadFilename(DiffuseTextureName, &pDataStart);				
		}
		
		// map_Ks
		else if((pDataStart[0] == 'm') && (pDataStart[1] == 'a') && (pDataStart[2] =='p') && (pDataStart[3] == '_') && (pDataStart[4] == 'K') && (pDataStart[5] == 's'))
		{
			pDataStart += 6;
			CTextReader::ReadFilename(SpecularTextureName, &pDataStart);				
		}

		// map_d
		else if((pDataStart[0] == 'm') && (pDataStart[1] == 'a') && (pDataStart[2] =='p') && (pDataStart[3] == '_') && (pDataStart[4] == 'd'))
		{
			pDataStart += 5;
			CTextReader::ReadFilename(AlphaTextureName, &pDataStart);				
		}

		// map_bump
		else if((pDataStart[0] == 'm') && (pDataStart[1] == 'a') && (pDataStart[2] =='p') && (pDataStart[3] == '_') && (pDataStart[4] == 'b') && (pDataStart[5] == 'u') && (pDataStart[6] == 'm') && (pDataStart[7] == 'p'))
		{
			pDataStart += 8;
			CTextReader::ReadFilename(NormalTextureName, &pDataStart);				
		}
		else if(pDataStart[0] == '#')
		{
			CTextReader::SkipComments('#', &pDataStart);
		}
		
		// All unused parameters
		else if((pDataStart[0] == 'N') && (pDataStart[1] == 'i'))
		{
			pDataStart += 2;
			float Dummy;
			CTextReader::ReadFloat(Dummy, &pDataStart);
		}

		else if((pDataStart[0] == 'T') && (pDataStart[1] == 'r'))
		{
			pDataStart += 2;
			float Dummy;
			CTextReader::ReadFloat(Dummy, &pDataStart);
		}

		else if(pDataStart[0] == 'd')
		{
			pDataStart += 2;
			float Dummy;
			CTextReader::ReadFloat(Dummy, &pDataStart);
		}								
		else if((pDataStart[0] == 'T') && (pDataStart[1] == 'f'))
		{
			pDataStart += 2;
			float Dummy;
			CTextReader::ReadFloat(Dummy, &pDataStart);
			CTextReader::ReadFloat(Dummy, &pDataStart);					
			CTextReader::ReadFloat(Dummy, &pDataStart);					
		}

		else if((pDataStart[0] == 'K') && (pDataStart[1] == 'a'))
		{
			pDataStart += 2;
			float Dummy;
			CTextReader::ReadFloat(Dummy, &pDataStart);
			CTextReader::ReadFloat(Dummy, &pDataStart);					
			CTextReader::ReadFloat(Dummy, &pDataStart);					
		}

		else if((pDataStart[0] == 'K') && (pDataStart[1] == 'e'))
		{
			pDataStart += 2;
			float Dummy;
			CTextReader::ReadFloat(Dummy, &pDataStart);
			CTextReader::ReadFloat(Dummy, &pDataStart);					
			CTextReader::ReadFloat(Dummy, &pDataStart);					
		}
		
		else if((pDataStart[0] == 'i') && (pDataStart[1] == 'l') && (pDataStart[2] == 'l') && (pDataStart[3] == 'u') && (pDataStart[4] == 'm'))
		{
			pDataStart += 5;
			int Dummy;
			CTextReader::ReadInt(Dummy, &pDataStart);
		}				
		else if((pDataStart[0] == 'm') && (pDataStart[1] == 'a') && (pDataStart[2] =='p') && (pDataStart[3] == '_') && (pDataStart[4] == 'K') && (pDataStart[5] == 'a'))
		{
			pDataStart += 6;
			char Str[256];
			CTextReader::ReadFilename(Str, &pDataStart);				
		}				
		else if((pDataStart[0] == 'b') && (pDataStart[1] == 'u') && (pDataStart[2] =='m') && (pDataStart[3] == 'p'))
		{
			pDataStart += 4;
			char Str[256];
			CTextReader::ReadFilename(Str, &pDataStart);				
		}								
		else
		{
			Assert(0);
		}
	}			
	AddMaterial(MtlName, DiffuseTextureName, SpecularTextureName, AlphaTextureName, NormalTextureName, DiffuseParameters, SpecularParameters);																		
	m_pLoadingSystem->Free(pData, Size);
	return 0;
}


//-----------------------------------------------------------------------------
void CJobLoadMaterialLibrary::AddMaterial(const char* pName, const char* pDiff, const char* pSpec, const char* pAlpha, const char* pNorm, XMVECTORF32& DiffuseParameters, XMVECTORF32& SpecularParameters)
{
	CTexture* pDiffuseTexture = pDiff[0] ? &m_pTextureSystem->CreateTextureFromFile(DXGI_FORMAT_R8G8B8A8_TYPELESS, 0, pDiff) : NULL;
	CTexture* pSpecularTexture = pSpec[0] ? &m_pTextureSystem->CreateTextureFromFile(DXGI_FORMAT_R8G8B8A8_TYPELESS, 0, pSpec) : NULL;
	CTexture* pAlphaTexture = pAlpha[0] ? &m_pTextureSystem->CreateTextureFromFile(DXGI_FORMAT_R8G8B8A8_TYPELESS, 0, pAlpha) : NULL;
	CTexture* pNormalTexture = pNorm[0] ? &m_pTextureSystem->CreateTextureFromFile(DXGI_FORMAT_R8G8B8A8_TYPELESS, 0, pNorm) : NULL;
	CSampler* pLinearSampler = &m_pTextureSystem->GetSampler(CTextureSystem::eSamplerLinear);
	CMaterialPhong::CParameters Params(	pName, 
										pDiffuseTexture, 
										pSpecularTexture, 
										pAlphaTexture, 
										pNormalTexture, 
										pLinearSampler,
										pLinearSampler,
										pLinearSampler,
										pLinearSampler,
										CMaterialPhong::eUnskinned, 
										m_LocalToWorldTransform, 
										DiffuseParameters, SpecularParameters);
	CMaterial* pMaterial = &m_pMaterialSystem->GetMaterial<CMaterialPhong>(&Params);
	pMaterial; // C4189
}

