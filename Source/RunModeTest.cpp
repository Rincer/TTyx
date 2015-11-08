#include "stdafx.h"
#include "MaterialSystem.h"
#include "TextureSystem.h"
#include "GeometrySystem.h"
#include "DrawPrimitive.h"
#include "Renderer.h"
#include "LightSystem.h"
#include "VertexFormats.h"
#include "VertexProcessing.h"
#include "LoadingSystem.h"
#include "XFileParser.h"
#include "ConstantsSystem.h"
#include "UtilityDraw.h"
#include "StringDictionary.h"
#include "RenderObject.h"
#include "Animation.h"
#include "DebugGUI.h"
#include "Input.h"
#include "ShaderPipeline.h"
#include "MaterialTextured2D.h"
#include "MaterialExperimental.h"
#include "InstancedObject.h"
#include "SkeletalObject.h"
#include "StaticObjectCollection.h"
#include "PooledAllocator.h"

#include "RunModeTest.h"




CRunModeTest::CRunModeTest() : m_View(NULL),
m_ViewController(NULL, NULL)
{
}

CRunModeTest::CRunModeTest(	CInput* pInput,
							CConstantsSystem* pConstantsSystem,
							CRenderObjectSystem* pRenderObjectSystem,
							CLoadingSystem* pLoadingSystem,
							CLightSystem* pLightSystem,
							CTimeLine* pTimeLine,
							CRenderer* pRenderer,
							CAnimationSystem* pAnimationSystem,
							CTextureSystem* pTextureSystem,
							CMaterialSystem* pMaterialSystem,
							CUtilityDraw* pUtilityDraw,
							CShaderPipeline* pShaderPipeline,
							CStringDictionary* pStringDictionary) : m_View(pConstantsSystem),
																m_ViewController(pInput, &m_View),
																m_pConstantsSystem(pConstantsSystem),
																m_pRenderObjectSystem(pRenderObjectSystem),
																m_pLoadingSystem(pLoadingSystem),
																m_pLightSystem(pLightSystem),
																m_pTimeLine(pTimeLine),
																m_pRenderer(pRenderer),
																m_pAnimationSystem(pAnimationSystem),
																m_pTextureSystem(pTextureSystem),
																m_pMaterialSystem(pMaterialSystem),
																m_pUtilityDraw(pUtilityDraw),
																m_pShaderPipeline(pShaderPipeline),
																m_pStringDictionary(pStringDictionary)
{
}

CRunModeTest::~CRunModeTest()
{
}


// Create vertex buffer
SVertexObj vertices[] =
{
	{ XMFLOAT3( -1.0f, 1.0f, 0.0f ), XMFLOAT2( 0.0f, 0.0f ), XMFLOAT3( 0.0f, 0.0f, -1.0f ), XMFLOAT4( 0.0f, 0.0f, 0.0f, 0.0f )},
    { XMFLOAT3( -1.0f,-1.0f, 0.0f ), XMFLOAT2( 0.0f, 1.0f ), XMFLOAT3( 0.0f, 0.0f, -1.0f ), XMFLOAT4( 0.0f, 0.0f, 0.0f, 0.0f )},
    { XMFLOAT3(  1.0f, 1.0f, 0.0f ), XMFLOAT2( 1.0f, 0.0f ), XMFLOAT3( 0.0f, 0.0f, -1.0f ), XMFLOAT4( 0.0f, 0.0f, 0.0f, 0.0f )},
    { XMFLOAT3(  1.0f,-1.0f, 0.0f ), XMFLOAT2( 1.0f, 1.0f ), XMFLOAT3( 0.0f, 0.0f, -1.0f ), XMFLOAT4( 0.0f, 0.0f, 0.0f, 0.0f )}
};

// Create vertex buffer
SPosTexColor verticesPlainColor[] =
{
	{ XMFLOAT3( -1.0f, 1.0f, 0.0f ), XMFLOAT2(0.0f, 0.0f ), 0x000000FF},
    { XMFLOAT3( -1.0f,-1.0f, 0.0f ), XMFLOAT2(0.0f, 0.0f ), 0x0000FF00},
    { XMFLOAT3(  1.0f, 1.0f, 0.0f ), XMFLOAT2(0.0f, 0.0f ), 0x00FF0000},
    { XMFLOAT3(  1.0f,-1.0f, 0.0f ), XMFLOAT2(0.0f, 0.0f ), 0x0000FFFF}
};

unsigned short indices[] =
{
	2, 1, 0,
	2, 3, 1	
};

bool PrintAllNames = false;

void MatchAllocs();

XMFLOAT4 Iron	(0.560f, 0.570f, 0.580f, 0.f);
XMFLOAT4 Silver	(0.972f, 0.960f, 0.915f, 0.f);
XMFLOAT4 Aluminum	(0.913f, 0.921f, 0.925f, 0.f);
XMFLOAT4 Gold	(1.000f, 0.766f, 0.336f, 0.f);
XMFLOAT4 Copper	(0.955f, 0.637f, 0.538f, 0.f);
XMFLOAT4 Chromium	(0.550f, 0.556f, 0.554f, 0.f);
XMFLOAT4 Nickel	(0.660f, 0.609f, 0.526f, 0.f);
XMFLOAT4 Titanium	(0.542f, 0.497f, 0.449f, 0.f);
XMFLOAT4 Cobalt	(0.662f, 0.655f, 0.634f, 0.f);
XMFLOAT4 Platinum	(0.672f, 0.637f, 0.585f, 0.f);
XMFLOAT4 Bronze ( 205.0f / 255.0f, 127.0f / 255.0f, 50.0f / 255.0f, 0.f);
//--------------------------------------------------------------------------------------------
void CRunModeTest::Tick(float DeltaSec)
{
	CTimeLine::CScopedEvent ScopedEvent((unsigned int)MainThreadEvents::eRunMode, m_pTimeLine);

	static CSkeletalObject s_TTyxSkeletalObject;
	static CInstancedObject s_TTyxInstancedObject;
	static CStaticObjectCollection s_TTyxSector;
	static bool FirstTime = true;
	static CDrawPrimitive* pDrawPrimitive = NULL;
	static CDrawPrimitive* pDrawPrimitive1 = NULL;
	static CPointLight* pLight = NULL;
	static CDirectionalLight* pDirLight = NULL;
	
	static CResourceInfo  m_CBufferInfo;
	static CCBuffer*	  m_pCBuffer;	
	static float Yaw = 0.0f;	
	static float Pitch = 0.0f;	
	static const unsigned int scRoughNess = 5;
	static const unsigned int scMetalNess = 5;
	static CMaterial* pMaterial[scRoughNess][scMetalNess] = { NULL };
	if(FirstTime)
	{		
		CPooledAllocator* pPooledAllocator = new CPooledAllocator(16, 10, 8, &CMemoryManager::GetAllocator());
		pPooledAllocator->VerifyConsistency();
		void* p0 = pPooledAllocator->Alloc(0);
		void* p1 = pPooledAllocator->Alloc(0);
		void* p2 = pPooledAllocator->Alloc(0);
		void* p3 = pPooledAllocator->Alloc(0);
		void* p4 = pPooledAllocator->Alloc(0);
		pPooledAllocator->VerifyConsistency();
		pPooledAllocator->Free(p1);
		pPooledAllocator->Free(p3);
		pPooledAllocator->VerifyConsistency();
		void* p5 = pPooledAllocator->Alloc(0);
		void* p6 = pPooledAllocator->Alloc(0);
		void* p7 = pPooledAllocator->Alloc(0);
		void* p8 = pPooledAllocator->Alloc(0);
		void* p9 = pPooledAllocator->Alloc(0);
		pPooledAllocator->VerifyConsistency();
		p1 = pPooledAllocator->Alloc(0);
		p3 = pPooledAllocator->Alloc(0);
		pPooledAllocator->VerifyConsistency();
		pPooledAllocator->Free(p6);
		pPooledAllocator->VerifyConsistency();
//		XMMATRIX LocalToWorld = XMMatrixIdentity();
//		CMaterialPlainColor::CParameters Params("LocalToWorld", LocalToWorld);
//		gTTyx->GetMaterialSystem().AddParameters(CMaterialSystem::eMaterialPlainColor, &Params);	
//		CVBuffer& VBuffer = gTTyx->GetGeometrySystem().CreateVBufferFromMemory(sizeof(SPosTexColor), verticesPlainColor, sizeof(verticesPlainColor));
//		CIBuffer& IBuffer = gTTyx->GetGeometrySystem().CreateIBufferFromMemory(indices, sizeof(indices));							
//		static CDrawPrimitive sDrawPrimitive(&VBuffer, &IBuffer, CMaterialSystem::eMaterialPlainColor, 6, "Default", "LocalToWorld");
//		static CDrawPrimitive sDrawPrimitive1(&VBuffer, &IBuffer, CMaterialSystem::eMaterialPlainColor, 6, "Default", "LocalToWorld");
//		pDrawPrimitive = &sDrawPrimitive;
//		pDrawPrimitive1 = &sDrawPrimitive1;		
//		m_CBufferInfo.m_StartSlot = CConstantsSystem::eLocalToWorld;
//		m_CBufferInfo.m_NumViews = 1;
//		m_CBufferInfo.m_Mask = CResourceInfo::eVShader | CResourceInfo::ePShader;
//		m_pCBuffer = CConstantsSystem::Instance().CreateCBuffer(NULL, sizeof(CConstantsSystem::cbLocal));
						
//		gTTyx->GetMaterialSystem().AddLibrary(CMaterialSystem::eMaterialPhong, 10, "Default");
//		CAssetObj& AssetObj = gTTyx->GetLoadingSystem().LoadAssetObjFromFile("Cube.obj");
//		CAssetObj& AssetObj = gTTyx->GetLoadingSystem().LoadAssetObjFromFile("sponza.obj");

		s_TTyxSector.Create("plain.obj", 2, m_pRenderObjectSystem);
//		s_TTyxSkeletalObject.Create("tiny_4anim.x", m_pRenderObjectSystem, m_pAnimationSystem);		
//		s_TTyxSector.Create("sponza.obj", 392, m_pRenderObjectSystem);
//		s_TTyxInstancedObject.Create("teapot.obj", m_pRenderObjectSystem);

		m_pLoadingSystem->LoadFromFile("plain.obj", &CRenderObjectSystem::s_mRenderObjectCreator);
//		m_pLoadingSystem->LoadFromFile("tiny_4anim.x", &CRenderObjectSystem::s_mSkeletalObjectCreator);
//		m_pLoadingSystem->LoadFromFile("sponza.obj", &CRenderObjectSystem::s_mRenderObjectCreator);
//		m_pLoadingSystem->LoadFromFile("teapot.obj", &CRenderObjectSystem::s_mInstancedRenderObjectCreator);


//		Texture test
//		CTexture* pTexture = &m_pTextureSystem->CreateTextureFromFile(DXGI_FORMAT_R8G8B8A8_TYPELESS, 0, "tiny_skin.dds");
//		CMaterialTextured2D::CParameters Params("TestMaterial", pTexture, &m_pTextureSystem->GetSampler(CTextureSystem::eSamplerLinear), CMaterialTextured2D::eRGBA);
//		pMaterial = &m_pMaterialSystem->GetMaterial<CMaterialTextured2D>(&Params);


//		CMaterialMipGenerator::CParameters Params("MipGenerator");
//		pMaterial = &m_pMaterialSystem->GetMaterial(CMaterialSystem::eMaterialMipGenerator, &Params);

		for(unsigned int RoughNess = 0; RoughNess < scRoughNess; RoughNess++)
		{
			for(unsigned int MetalNess = 0; MetalNess < scMetalNess; MetalNess++)
			{
				char Name[256];
				sprintf_s(Name, 255, "Experimental_%d_%d", RoughNess, MetalNess);
				CMaterialExperimental::CParameters Params(	Name, 
															&m_pTextureSystem->GetTextureRef("DynamicEnvironmentMap"),
															&m_pTextureSystem->GetSampler(CTextureSystem::eSamplerLinear),
															Bronze, //XMFLOAT4(1.0f, 0.79f, 0.21f, 1.0f), // Base
															XMFLOAT4(0.1f + RoughNess * 0.9f / scRoughNess, 0.1f, 1.0f - 1.0f / (scMetalNess - 1) * MetalNess, 0.0f));	// RSM
				pMaterial[RoughNess][MetalNess] = &m_pMaterialSystem->GetMaterial<CMaterialExperimental>(&Params);
			}
		}

//		pComputeShader = &m_pShaderPipeline->GetComputeShader("MipGenerator.hlsl", "CSMain", NULL);


//		gTTyx->GetLoadingSystem().LoadFromFile("vine.obj");
//		gTTyx->GetLoadingSystem().LoadFromFile("cube.obj");
		
//		CTexture& DiffuseTexture = gTTyx->GetTextureSystem().CreateTextureFromFile("Test.bmp");
//		CTexture& SpecularTexture = gTTyx->GetTextureSystem().CreateTextureFromFile("spnza_bricks_a_spec.tga");		
//		CTexture& AlphaTexture = gTTyx->GetTextureSystem().CreateTextureFromFile("sponza_thorn_mask.tga");
//		CTexture& NormalTexture = gTTyx->GetTextureSystem().CreateTextureFromFile("spnza_bricks_a_ddn.tga");		
//		XMVECTORF32 SpecularParameters;
//		SpecularParameters.v = XMVectorSet(1.0f, 1.0f, 1.0f, 10.0f);
//		CMaterialPhong::CParameters Params("TestParams", &DiffuseTexture, &SpecularTexture, &AlphaTexture, &NormalTexture, SpecularParameters);		
//		gTTyx->GetMaterialSystem().AddParameters(CMaterialSystem::eMaterialPhong, &Params);	
				
//		CVertexProcessing::Instance().CreateTangents(vertices, 4, indices, 6);
//		CVBuffer& VBuffer = gTTyx->GetGeometrySystem().CreateVBufferFromMemory(sizeof(SVertexObj), vertices, sizeof(vertices));
//		CIBuffer& IBuffer = gTTyx->GetGeometrySystem().CreateIBufferFromMemory(indices, sizeof(indices));		
//					
//		static CDrawPrimitive sDrawPrimitive(&VBuffer, &IBuffer, CMaterialSystem::eMaterialPhong, 6, "Default", "TestParams");
//		pDrawPrimitive = &sDrawPrimitive;
		m_View.Initialize(XM_PIDIV4, m_pRenderer->GetAspectRatio(),  1.0f, 5000.0f);
		m_View.SetPosition(0.0f, 100.0f, -100.0f);
//		m_View.SetPosition(476.1832f, 128.5883f, -38.4587f);
		CColor ColorPt(0, 255, 255, 255);
		XMFLOAT3 Position(0.0f, 30.0f, -100.0f);
		CPointLight& Light = m_pLightSystem->CreateLight(ColorPt, 1.0f, Position, 5000.0f);
		pLight = &Light;
		CColor ColorDir(0, 255, 255, 255);
		CDirectionalLight& DirLight = m_pLightSystem->CreateLight(ColorDir, 1.0f, XMFLOAT3(-0.5f, 0.0f, -0.5f));
		pDirLight = &DirLight;
		FirstTime = false;
	} 
	else
	{
		m_ViewController.Tick(DeltaSec);
		m_View.Set();
//		pLight->m_Position = m_View.GetPosition();
		m_pLightSystem->CalculateDrawnLights();
		m_pLightSystem->SetDrawnLights(m_pConstantsSystem);
//		gTTyx->GetUtilityDraw().BeginTriangleList(XMMatrixIdentity(), 6);
//		gTTyx->GetUtilityDraw().AddTriangle(	verticesPlainColor[2].m_Pos, verticesPlainColor[1].m_Pos, verticesPlainColor[0].m_Pos,
//												CColor(0xff, 0, 0xff, 0), CColor(0xff, 0, 0xff, 0), CColor(0xff, 0, 0xff, 0));
//		gTTyx->GetUtilityDraw().AddTriangle(	verticesPlainColor[2].m_Pos, verticesPlainColor[3].m_Pos, verticesPlainColor[1].m_Pos,
//												CColor(0xff, 0xff, 0, 0), CColor(0xff, 0xff, 0, 0), CColor(0xff, 0xff, 0, 0));												
//		gTTyx->GetUtilityDraw().EndTriangleList();	
		
//		XMVECTOR v0 = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);				
//		XMVECTOR v1 = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);			
//		Yaw += DeltaSec * 1.0f;			
//		Pitch += DeltaSec * 1.0f;					
//		XMMATRIX RotPitch = XMMatrixRotationAxis(v0, Pitch);
//		XMMATRIX RotYaw = XMMatrixRotationAxis(v1, Yaw);									
//		gTTyx->GetUtilityDraw().DrawCube(XMMatrixMultiply(RotPitch, RotYaw));
//		CConstantsSystem::cbLocal* pcbLocal;
//		gTTyx->GetRenderer().UpdateConstantBuffer(m_pCBuffer, (void**)&pcbLocal, sizeof(CConstantsSystem::cbLocal));	
//		pcbLocal->m_LocalToWorld = XMMatrixIdentity();
//		gTTyx->GetRenderer().SetConstantBuffer(m_pCBuffer, &m_CBufferInfo);		
//		gTTyx->GetRenderer().DrawIndexedPrimitive(pDrawPrimitive);	
		
//		gTTyx->GetRenderer().UpdateConstantBuffer(m_pCBuffer, (void**)&pcbLocal, sizeof(CConstantsSystem::cbLocal));	
//		pcbLocal->m_LocalToWorld = XMMatrixIdentity();
//		pcbLocal->m_LocalToWorld._41 = 5.0f;
//		gTTyx->GetRenderer().SetConstantBuffer(m_pCBuffer, &m_CBufferInfo);		
//		gTTyx->GetRenderer().DrawIndexedPrimitive(pDrawPrimitive);	
//		pAssetObject->DrawSkeleton();
//		pAssetObject->TickAnimation(DeltaSec);
//		pAssetObject->DrawSkinned();
//		pAssetObject->Draw();
		if(PrintAllNames)
		{
			m_pStringDictionary->PrintAllStrings();
			//gTTyx->GetJobSystem().PrintStats();
		}
//		DebugPrintf("\n");
//		gTTyx->GetDebugGUI().DrawString(10.0f, 10.0f, "Puhi");
		s_TTyxSkeletalObject.TickAnimation(DeltaSec);
		s_TTyxSkeletalObject.SetScaleRotationPosition(0.1f, 0.1f, 0.1f,
												XM_PIDIV2, -XM_PIDIV2, 0.0f,
												-100.0f, 0.0f, 0.0f);

		s_TTyxInstancedObject.SetScaleRotationPosition(	2.0f, 2.0f, 2.0f,
														0.0f, 0.0f, 0.0f,
														0.0f, 0.0f, 0.0f);
		s_TTyxInstancedObject.Draw(m_pRenderer, m_pConstantsSystem);

		XMFLOAT3 Pos(-50, 15, 0);

		float Color[4] = { 0, 0, 1, 0 };
		m_pRenderer->ClearColor(CRenderTargets::eDynamicEnvironmentMapFace0, Color);
		m_pRenderer->ClearColor(CRenderTargets::eDynamicEnvironmentMapFace1, Color);
		m_pRenderer->ClearColor(CRenderTargets::eDynamicEnvironmentMapFace2, Color);
		m_pRenderer->ClearColor(CRenderTargets::eDynamicEnvironmentMapFace3, Color);
		m_pRenderer->ClearColor(CRenderTargets::eDynamicEnvironmentMapFace4, Color);
		m_pRenderer->ClearColor(CRenderTargets::eDynamicEnvironmentMapFace5, Color);
		unsigned int OldWidth;
		unsigned int OldHeight;
		m_pRenderer->GetViewportSize(OldWidth, OldHeight);

		m_pRenderer->SetViewport(0, 0, (float)CRenderTargets::sc_DynamicEnvironmentMapWidth, (float)CRenderTargets::sc_DynamicEnvironmentMapHeight, 0, 1);

		CView View(m_pConstantsSystem);
		View.Initialize(XM_PIDIV2, 1.0f, 1.0f, 5000.0f);
		View.SetPosition(Pos.x, Pos.y, Pos.z);
		View.Update(0, 0, 0, 0, 0);
		View.Set();

		m_pRenderer->ClearDepthStencil(CRenderTargets::eDynamicEnvironmentMapDepth, D3D11_CLEAR_DEPTH, 1.0f, 0);
		m_pRenderer->SetRenderTargets(CRenderTargets::eDynamicEnvironmentMapFace4, CRenderTargets::eDynamicEnvironmentMapDepth);
		s_TTyxSkeletalObject.Draw(m_pRenderer, m_pConstantsSystem);
		s_TTyxSector.Draw(m_pRenderer, m_pConstantsSystem);

		View.Initialize(XM_PIDIV2, 1.0f, 1.0f, 5000.0f);
		View.SetPosition(Pos.x, Pos.y, Pos.z);
		View.Update(0, XM_PIDIV2, 0, 0, 0);
		View.Set();
		m_pRenderer->ClearDepthStencil(CRenderTargets::eDynamicEnvironmentMapDepth, D3D11_CLEAR_DEPTH, 1.0f, 0);
		m_pRenderer->SetRenderTargets(CRenderTargets::eDynamicEnvironmentMapFace0, CRenderTargets::eDynamicEnvironmentMapDepth);
		s_TTyxSkeletalObject.Draw(m_pRenderer, m_pConstantsSystem);
		s_TTyxSector.Draw(m_pRenderer, m_pConstantsSystem);

		View.Initialize(XM_PIDIV2, 1.0f, 1.0f, 5000.0f);
		View.SetPosition(Pos.x, Pos.y, Pos.z);
		View.Update(0, 2 * XM_PIDIV2, 0, 0, 0);
		View.Set();
		m_pRenderer->ClearDepthStencil(CRenderTargets::eDynamicEnvironmentMapDepth, D3D11_CLEAR_DEPTH, 1.0f, 0);
		m_pRenderer->SetRenderTargets(CRenderTargets::eDynamicEnvironmentMapFace5, CRenderTargets::eDynamicEnvironmentMapDepth);
		s_TTyxSkeletalObject.Draw(m_pRenderer, m_pConstantsSystem);
		s_TTyxSector.Draw(m_pRenderer, m_pConstantsSystem);

		View.Initialize(XM_PIDIV2, 1.0f, 1.0f, 5000.0f);
		View.SetPosition(Pos.x, Pos.y, Pos.z);
		View.Update(0, 3 * XM_PIDIV2, 0, 0, 0);		
		View.Set();
		m_pRenderer->ClearDepthStencil(CRenderTargets::eDynamicEnvironmentMapDepth, D3D11_CLEAR_DEPTH, 1.0f, 0);
		m_pRenderer->SetRenderTargets(CRenderTargets::eDynamicEnvironmentMapFace1, CRenderTargets::eDynamicEnvironmentMapDepth);
		s_TTyxSkeletalObject.Draw(m_pRenderer, m_pConstantsSystem);
		s_TTyxSector.Draw(m_pRenderer, m_pConstantsSystem);

		View.Initialize(XM_PIDIV2, 1.0f, 1.0f, 5000.0f);
		View.SetPosition(Pos.x, Pos.y, Pos.z);
		View.Update(-XM_PIDIV2, 0, 0, 0, 0);
		View.Set();
		m_pRenderer->ClearDepthStencil(CRenderTargets::eDynamicEnvironmentMapDepth, D3D11_CLEAR_DEPTH, 1.0f, 0);
		m_pRenderer->SetRenderTargets(CRenderTargets::eDynamicEnvironmentMapFace2, CRenderTargets::eDynamicEnvironmentMapDepth);
		s_TTyxSkeletalObject.Draw(m_pRenderer, m_pConstantsSystem);
		s_TTyxSector.Draw(m_pRenderer, m_pConstantsSystem);

		View.Initialize(XM_PIDIV2, 1.0f, 1.0f, 5000.0f);
		View.SetPosition(Pos.x, Pos.y, Pos.z);
		View.Update(XM_PIDIV2, 0, 0, 0, 0);
		View.Set();
		m_pRenderer->ClearDepthStencil(CRenderTargets::eDynamicEnvironmentMapDepth, D3D11_CLEAR_DEPTH, 1.0f, 0);
		m_pRenderer->SetRenderTargets(CRenderTargets::eDynamicEnvironmentMapFace3, CRenderTargets::eDynamicEnvironmentMapDepth);
		s_TTyxSkeletalObject.Draw(m_pRenderer, m_pConstantsSystem);
		s_TTyxSector.Draw(m_pRenderer, m_pConstantsSystem);

		m_View.Set();
		m_pRenderer->SetViewport(0, 0, (float)OldWidth, (float)OldHeight, 0, 1);
		m_pRenderer->SetRenderTargets(CRenderTargets::eFrameBuffer, CRenderTargets::eDepthStencil);

		m_pRenderer->GenerateDynamicEnvMipMaps();

		s_TTyxSkeletalObject.Draw(m_pRenderer, m_pConstantsSystem);
		s_TTyxSector.Draw(m_pRenderer, m_pConstantsSystem);

		//for(unsigned int RoughNess = 0; RoughNess < scRoughNess; RoughNess++)
		//{
		//	for(unsigned int MetalNess = 0; MetalNess < scMetalNess; MetalNess++)
		//	{
		//		XMMATRIX LocalToWorld;		
		//		LocalToWorld = XMMatrixScaling(15, 15, 15) * XMMatrixTranslation(Pos.x + 35 * RoughNess, Pos.y + 35 * MetalNess, Pos.z);
		//		m_pUtilityDraw->DrawSphere(LocalToWorld, pMaterial[RoughNess][MetalNess]);
		//	}
		//}

		m_pRenderer->Postprocess();

//		Texture test
/*
		XMMATRIX LocalToWorld = XMMatrixIdentity();
		float ScreenPixelWidth = 2.0f / (float)1024;
		float ScreenPixelHeight = 2.0f / (float)768;

		m_pUtilityDraw->BeginTriangleList();
		float X0 = -1.0f + 100.0f * ScreenPixelWidth;
		float X1 = X0 + 256.0f * ScreenPixelWidth;
		float Y0 = 1.0f - 100.0f * ScreenPixelHeight;
		float Y1 = Y0 - 256.0f * ScreenPixelHeight;
		CColor Color(255, 255, 255, 255);

		XMFLOAT3 Pt0(X0, Y0, 0.999f);
		XMFLOAT3 Pt1(X0, Y1, 0.999f);
		XMFLOAT3 Pt2(X1, Y0, 0.999f);
		XMFLOAT3 Pt3(X1, Y1, 0.999f);

		XMFLOAT2 Uv0(0.0f, 0.0f);
		XMFLOAT2 Uv1(0.0f, 1.0f);
		XMFLOAT2 Uv2(1.0f, 0.0f);
		XMFLOAT2 Uv3(1.0f, 1.0f);

		m_pUtilityDraw->AddTriangle(Pt0, Pt2, Pt1, Uv0, Uv2, Uv1, Color, Color, Color);
		m_pUtilityDraw->AddTriangle(Pt1, Pt2, Pt3, Uv1, Uv2, Uv3, Color, Color, Color);
		m_pUtilityDraw->EndTriangleList(LocalToWorld, pMaterial, CRenderer::eDepthNone, CRenderer::eBlendNone);
*/		
	}
}