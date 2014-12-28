#ifndef _TTYX_H_
#define _TTYX_H_

#include "Color.h"
#include "Thread.h"
#include "Window.h"
#include "RunModeTest.h"
#include "TimeLine.h"

class CConsole;
class CDebugGUI;
class CStringDictionary;
class CJobSystem;
class CRenderer;
class CTimeLine;
class CInput;
class CConstantsSystem;
class CAnimationSystem;
class CDrawPrimitiveSystem;
class CGeometrySystem;
class CLightSystem;
class CLoadingSystem;
class CMaterialSystem;
class CRenderObjectSystem;
class CShaderPipeline;
class CUtilityDraw;
class CSkeletonSystem;
class CTextureSystem;
class CNetwork;
class CPooledAllocator;

// The main TTyx
class TTyx
{
	public:

		void MainLoop();
		void Startup(HINSTANCE Instance, int CmdShow);
		void Shutdown();

	// Profiling 
		static CTimeLine::CEventDesc m_ProfilingEvents[MainThreadEvents::eTotalEvents + 1];
		
	private:
		void ConstructMemoryPools();
		void DestructMemoryPools();
		void ConstructComponents();
		void DestructComponents();

		unsigned int m_FrameCounter;
		CWindow m_Wnd; // main window for the app.	

		CRunModeTest*		m_pRunModeTest;	

		// Memory pools	shared between components
		CPooledAllocator*		m_pMultiListPool;

		// Components
		CConsole*				m_pConsole;
		CDebugGUI*				m_pDebugGUI;		
		CStringDictionary*		m_pStringDictionary;
		CJobSystem*				m_pJobSystem;
		CRenderer*				m_pRenderer;
		CTimeLine*				m_pTimeLine;
		CInput*					m_pInput;
		CConstantsSystem*		m_pConstantsSystem;
		CAnimationSystem*		m_pAnimationSystem;
		CDrawPrimitiveSystem*	m_pDrawPrimitiveSystem;
		CGeometrySystem*		m_pGeometrySystem;
		CLightSystem*			m_pLightSystem;
		CLoadingSystem*			m_pLoadingSystem;
		CMaterialSystem*		m_pMaterialSystem;
		CRenderObjectSystem*	m_pRenderObjectSystem;
		CShaderPipeline*		m_pShaderPipeline;
		CUtilityDraw*			m_pUtilityDraw;
		CSkeletonSystem*		m_pSkeletonSystem;
		CTextureSystem*			m_pTextureSystem;
		CNetwork*				m_pNetwork;
};

extern TTyx* gTTyx;

#endif