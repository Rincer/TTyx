// TTyx.cpp : Defines the entry point for the application.
#include "stdafx.h"
#include "Macros.h"
#include "Time.h"
#include "Network.h"
#include "LiveEditTree.h"
#include "MemoryManager.h"
#include "PooledAllocator.h"
#include "Renderer.h"
#include "ShaderPipeline.h"
#include "TextureSystem.h"
#include "GeometrySystem.h"
#include "ConstantsSystem.h"
#include "Input.h"
#include "ImageProcessing.h"
#include "LoadingSystem.h"
#include "DrawPrimitive.h"
#include "UtilityDraw.h"
#include "Animation.h"
#include "RenderObject.h"
#include "Math.h"
#include "ThreadPool.h"
#include "MaterialSystem.h"
#include "ShaderPipeline.h"
#include "TimeLine.h"
#include "Console.h"
#include "DebugGUI.h"
#include "StringDictionary.h"
#include "LightSystem.h"
#include "VertexProcessing.h"

#include "TTyx.h"

//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch( message )
    {    
        case WM_PAINT:
            hdc = BeginPaint( hWnd, &ps );
            EndPaint( hWnd, &ps );
            break;

        case WM_DESTROY:
            PostQuitMessage( 0 );
            break;

        default:
            return DefWindowProc( hWnd, message, wParam, lParam );
    }

    return 0;
}

//--------------------------------------------------------------------------------------
CTimeLine::CEventDesc TTyx::m_ProfilingEvents[MainThreadEvents::eTotalEvents + 1] = 
{
	{
		MainThreadEvents::eMainTick,
		CColor(128, 0, 0, 255),
		"Main",
	},
	{
		MainThreadEvents::eRunMode,
		CColor(128, 0, 255, 0),
		"RunMode",
	},
	{
		MainThreadEvents::eRenderSync,
		CColor(128, 255, 0, 0),
		"RenderSync",
	},
	{
		0,
		CColor(0, 0, 0, 0),
		NULL,
	} // Null terminated	
};

//--------------------------------------------------------------------------------------
static TTyx sTTyx; // One global instance of the app

//--------------------------------------------------------------------------------------

void TTyx::ConstructMemoryPools()
{
	m_pMultiListPool = new CPooledAllocator(sizeof(CMultiList<1, int>::CIterator), CMultiList<1, int>::sc_MaxMultiListIterators, 4, &CMemoryManager::GetAllocator());
}

//--------------------------------------------------------------------------------------
void TTyx::DestructMemoryPools()
{
	delete m_pMultiListPool;
}

//--------------------------------------------------------------------------------------
void TTyx::ConstructComponents()
{
	// Order is important!
	CTime::Initialize();
	m_pStringDictionary = new CStringDictionary();
	m_pInput = new CInput();
	m_pConsole = new CConsole(m_pInput, &m_pMaterialSystem, &m_pRenderer, &m_pUtilityDraw, &m_pDebugGUI, m_pMultiListPool);
	m_pTimeLine = new CTimeLine(m_pConsole, &m_pMaterialSystem, &m_pRenderer, &m_pUtilityDraw, &m_pDebugGUI);
	m_pNetwork = new CNetwork(&m_pTimeLine);
	CLiveEditTree::Instance().Initialize(m_pNetwork);
	m_pDrawPrimitiveSystem = new CDrawPrimitiveSystem(&m_pStringDictionary);
	m_pTextureSystem = new CTextureSystem(&m_pStringDictionary, &m_pJobSystem, &m_pRenderer, &m_pLoadingSystem, &m_pDrawPrimitiveSystem, &m_pMaterialSystem, &m_pConstantsSystem);
	m_pRenderer = new CRenderer(m_Wnd.GetWnd(), &m_pTimeLine, &m_pTextureSystem, &m_pJobSystem, &m_pMaterialSystem, &m_pConstantsSystem, &m_pDrawPrimitiveSystem, &m_pGeometrySystem, m_pMultiListPool);
	m_pConstantsSystem = new CConstantsSystem(&m_pRenderer, &m_pStringDictionary );
	m_pShaderPipeline = new CShaderPipeline(&m_pStringDictionary, &m_pJobSystem, &m_pRenderer);
	m_pMaterialSystem = new CMaterialSystem(&m_pStringDictionary, &m_pJobSystem, &m_pShaderPipeline, &m_pConstantsSystem);
	m_pGeometrySystem = new CGeometrySystem(&m_pRenderer, &m_pStringDictionary);
	m_pJobSystem = new CJobSystem(&m_pTimeLine, m_pMultiListPool);
	m_pUtilityDraw = new CUtilityDraw(&m_pRenderer, &m_pConstantsSystem, &m_pGeometrySystem, &m_pStringDictionary, &m_pMaterialSystem, &m_pDrawPrimitiveSystem, m_pMultiListPool);
	m_pAnimationSystem = new CAnimationSystem(&m_pStringDictionary);
	m_pSkeletonSystem = new CSkeletonSystem(&m_pStringDictionary);
	m_pRenderObjectSystem = new CRenderObjectSystem(&m_pStringDictionary, &m_pUtilityDraw);
	m_pLightSystem = new CLightSystem();
	m_pLoadingSystem = new CLoadingSystem(	&m_pTextureSystem,
											&m_pMaterialSystem,
											&m_pAnimationSystem,
											&m_pStringDictionary,
											&m_pGeometrySystem,
											&m_pDrawPrimitiveSystem,
											&m_pSkeletonSystem,
											&m_pRenderObjectSystem,
											&m_pJobSystem);
	m_pDebugGUI = new CDebugGUI(&m_pTextureSystem, &m_pUtilityDraw, &m_pMaterialSystem, &m_pRenderer);
	m_pRunModeTest = new(CMemoryManager::GetAllocator().AlignedAlloc(sizeof(CRunModeTest), 16)) CRunModeTest(m_pInput,
																											 m_pConstantsSystem,
																											 m_pRenderObjectSystem,
																											 m_pLoadingSystem,
																											 m_pLightSystem,
																											 m_pTimeLine,
																											 m_pRenderer,
																											 m_pAnimationSystem,
																											 m_pTextureSystem,
																											 m_pMaterialSystem,
																											 m_pUtilityDraw,
																											 m_pShaderPipeline,
																											 m_pStringDictionary);
	//----------------------------------------------------------------------------------
	// starting up
	m_pTextureSystem->Startup();
}

//--------------------------------------------------------------------------------------
void TTyx::DestructComponents()
{
	// Order is important!
	delete m_pRunModeTest;
	delete m_pDebugGUI;
	delete m_pConsole;
	delete m_pInput;
	delete m_pLoadingSystem;
	delete m_pLightSystem;
	delete m_pRenderObjectSystem;
	delete m_pSkeletonSystem;
	delete m_pAnimationSystem;
	delete m_pUtilityDraw;
	delete m_pGeometrySystem;
	delete m_pDrawPrimitiveSystem;
	delete m_pMaterialSystem;
	delete m_pTextureSystem;
	delete m_pShaderPipeline;
	delete m_pConstantsSystem;
	delete m_pRenderer;
	delete m_pNetwork;
	delete m_pJobSystem;
	delete m_pTimeLine;
	delete m_pStringDictionary;
}


//--------------------------------------------------------------------------------------
void TTyx::Startup(HINSTANCE Instance, int CmdShow)
{
	m_Wnd.Initialize(WndProc, Instance, CmdShow, L"TTyx", L"TTyx", 1024, 768);
	CWindow::SetCurrentViewWnd(m_Wnd.GetWnd());
	ConstructMemoryPools();
	ConstructComponents();

	m_pTimeLine->RegisterThread("Main", TTyx::m_ProfilingEvents);
	m_pRenderer->Startup();
	m_pInput->Startup(Instance);	

//	for (unsigned int StartFrames = 0; StartFrames < 2)
	m_pTimeLine->FrameStart();
	{
		m_pRenderer->Tick(m_FrameCounter, 0.0f);
		m_FrameCounter++;
	}
	m_pTimeLine->FrameEnd();
}

//--------------------------------------------------------------------------------------
void TTyx::Shutdown()
{
	bool AllSystemsShutDown;
	// Let the renderer execute a frame to clear up any set resources 
	m_pTimeLine->FrameStart();
	m_pRenderer->Tick(m_FrameCounter, 0.0f);
	m_pTimeLine->FrameEnd();
	m_FrameCounter++;

	// Let the renderer execute until all systems have been shut down
	do
	{
		m_pTimeLine->FrameStart();
		{
			m_pTextureSystem->Shutdown();
			m_pGeometrySystem->Shutdown();
			m_pConstantsSystem->Shutdown();
			m_pShaderPipeline->Shutdown();
			m_pMaterialSystem->Shutdown();
			m_pRenderer->Tick(m_FrameCounter, 0.0f); // Needs to be last for correct main/render sync
		}
		m_pTimeLine->FrameEnd();
		m_FrameCounter++;
		AllSystemsShutDown = m_pTextureSystem->IsShutdown();
		AllSystemsShutDown &= m_pGeometrySystem->IsShutdown();
		AllSystemsShutDown &= m_pConstantsSystem->IsShutdown();
		AllSystemsShutDown &= m_pLoadingSystem->IsShutdown();
		AllSystemsShutDown &= m_pShaderPipeline->IsShutdown();
		AllSystemsShutDown &= m_pMaterialSystem->IsShutdown();
	} while (!AllSystemsShutDown);

	m_pTimeLine->FrameStart(); // Need to do this to release the per thread event recording mutexes
	m_pRenderer->Shutdown();
	m_pTimeLine->FrameEnd();

	m_pTimeLine->Shutdown();
	DestructComponents();
	DestructMemoryPools();
}


//--------------------------------------------------------------------------------------
void TTyx::MainLoop()
{
	m_pTimeLine->FrameStart();
    {
		CTimeLine::CScopedEvent ScopedEvent((unsigned int)MainThreadEvents::eMainTick, m_pTimeLine);
		CTime::Tick();
		float DeltaSec = CTime::GetDeltaTimeSec();
		
		m_pNetwork->Tick(m_FrameCounter, DeltaSec);
		
		m_pRunModeTest->Tick(DeltaSec);			
		m_pTextureSystem->Tick(DeltaSec);
		m_pGeometrySystem->Tick(DeltaSec);
		m_pConstantsSystem->Tick(DeltaSec);
		m_pDrawPrimitiveSystem->Tick(DeltaSec);
		m_pAnimationSystem->Tick(DeltaSec);
		m_pRenderObjectSystem->Tick(DeltaSec);
		m_pShaderPipeline->Tick(DeltaSec);
		m_pMaterialSystem->Tick(DeltaSec);
		m_pConsole->Tick(DeltaSec);
		m_pTimeLine->Draw();
		m_pUtilityDraw->Tick();	
		m_pRenderer->Tick(m_FrameCounter, DeltaSec);	 // Needs to be last for correct main/render sync
	}	
	m_pTimeLine->FrameEnd();
	m_FrameCounter++;
}



int APIENTRY _tWinMain(	HINSTANCE hInstance,
						HINSTANCE hPrevInstance,
						LPTSTR    lpCmdLine,
						int       nCmdShow)
{	
// C4100
	SPos Verts[1000];
	unsigned short Indices[1000];
	CVertexProcessing::Instance().CreateSphere(Verts, Indices, 2);
	lpCmdLine;
	hPrevInstance;
	CThread::UpdateThreadID();
	sTTyx.Startup(hInstance, nCmdShow);
	MSG Msg;
	// Main message loop:
	bool bGotMsg;
    Msg.message = WM_NULL;
    PeekMessage( &Msg, NULL, 0U, 0U, PM_NOREMOVE );
    while( WM_QUIT != Msg.message )
    {
        bGotMsg = ( PeekMessage( &Msg, NULL, 0U, 0U, PM_REMOVE ) != 0 );
        if( bGotMsg )
        {
			TranslateMessage( &Msg );
			DispatchMessage( &Msg );
        }
        else
        {
            sTTyx.MainLoop();
        }
    }
	sTTyx.Shutdown();
    // we don't have a destructor for heap allocator since c-runtime would try to add a call to it when it creates the main heap allocator, at which point 
    // the array of destructor calls (for static objects) doesn't exist yet, so we need to shut it down manually when program exits.
    atexit(CMemoryManager::ShutDown);
	return (int) Msg.wParam;
}



