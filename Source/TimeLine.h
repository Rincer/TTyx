#ifndef _TIMELINE_H_
#define _TIMELINE_H_

#include "Color.h"
#include "Console.h"
#include "LWMutex.h"
#include "Reference.h"

class CMaterial;
class CMaterialSystem;
class CRenderer;
class CUtilityDraw;
class CDebugGUI;


namespace MainThreadEvents
{
	enum eProfilingEvents
	{
		eMainTick,
		eRunMode,
		eRenderSync,
		eTotalEvents
	};
};

namespace RenderThreadEvents
{
	enum eProfilingEvents
	{
		eThreadLoop,
		eExecuteCommand,
		eCommandDrawPrim,
		eTotalEvents
	};
};



class CTimeLine
{
	public:
	
		class CScopedEvent
		{
			public:
				CScopedEvent(unsigned int Type, CTimeLine* pTimeLine);
				~CScopedEvent();
			private:
				CTimeLine* m_pTimeLine;
				unsigned int m_Type;
		};
		
		class CEventDesc
		{
			public:
				unsigned int			m_Type; // This is not strictly needed since it corresponds to a position in an array
				CColor					m_Color;
				const char*				m_pName;
		};
		
		
		class CEvent
		{
			public:
				enum eState
				{
					eStarted,
					eStopped
				};
				CEvent() 
				{
					m_StartTime = 0;		//
					m_StopTime = 0;			//
					m_Index = 0xFFFFFFFF;	// invalid on purpose for verification
					m_State = eStopped;

				};				
				unsigned long long	m_StartTime;	//
				unsigned long long	m_StopTime;		//
				unsigned int		m_Index;		// Index into event descriptor table
				eState				m_State;
		};
	
		CTimeLine(CConsole* pConsole,
				  CMaterialSystem** ppMaterialSystem,
				  CRenderer** ppRenderer,
				  CUtilityDraw** ppUtilityDraw,
				  CDebugGUI** ppDebugGUI);
		~CTimeLine();
		void RegisterThread(const char* pName, const CEventDesc* pEventDesc);
		void Draw();
		void FrameStart();
		void FrameEnd();
		void ToggleRequest();
		bool IsEnabled();
		void Shutdown();
		void StartEvent(unsigned int EventIndex);
		void StopEvent(unsigned int EventIndex);


	private:		
		void Toggle();
		void ThreadSelect(const char* pThreadName);
		void StartEventInternal(unsigned int EventIndex, bool Internal);
		void StopEventInternal(unsigned int EventIndex, bool Internal);
		void DrawEvent(	CEvent* pEvent,
						unsigned int PrevFrame,
						unsigned long long ClockFrequency,
						unsigned int ThreadIndex,
						float MicroSecondsToViewSpace,
						float& X0,
						float& X1,
						float Y0,
						float Y1,
						float PixelWidth,
						float PixelHeight,
						const CEventDesc* pEventDesc);


		static const unsigned int	scMaxEvents = 4 * 1024;
		static const unsigned int	scEventStackSize = 128;
		static const unsigned int	scMaxEventTypesPerThread = 64;
		static const long			scMaxThreads = 32; // Should be the total amount of threads spawned by the application
		
		const char*			m_pThreadNames[scMaxThreads];
		const CEventDesc*	m_pEventDescs[scMaxThreads];
		CEvent				m_Events[scMaxThreads][scMaxEvents]; 
		unsigned int		m_EventRead[scMaxThreads];				
		unsigned int		m_EventWrite[scMaxThreads];				
		unsigned int		m_EventEnd[scMaxThreads];
		CEvent*				m_pActive[scMaxThreads];
		CEvent*				m_EventStack[scMaxThreads][scEventStackSize];
		int					m_EventStackPtr[scMaxThreads];

		unsigned long long	m_FrameStart[2];
		unsigned long long	m_FrameEnd[2];
		unsigned int		m_Duration[scMaxEventTypesPerThread];
		unsigned int		m_CurrFrame;
		unsigned int		m_FrameCount;
		unsigned int		m_CurrentThread;
		unsigned int		m_TotalThreads;
		CMaterial*			m_pMaterial;
		bool				m_Enabled;
		bool				m_ToggleRequest;
				
		
		//-----------------------------------------------------------------------------
		// Console commands
		class CTimeLineToggle : public CConsole::CConsoleCommand
		{
			public:
				CTimeLineToggle(const char* pToken, CConsole* pConsole, CTimeLine* pTimeLine);				
				virtual bool Execute(const char* pCommandString); 	
			private:
				CTimeLine* m_pTimeLine;
		};		

		//-----------------------------------------------------------------------------
		class CTimeLineThreadSelect : public CConsole::CConsoleCommand
		{
			public:
				CTimeLineThreadSelect(const char* pToken, CConsole* pConsole, CTimeLine* pTimeLine);
				virtual bool Execute(const char* pCommandString); 	
			private:
				CTimeLine* m_pTimeLine;
		};		
		
		CTimeLineToggle			m_ToggleCommand;
		CTimeLineThreadSelect	m_ThreadSelectCommand;	

		// System level components
		CReference<CMaterialSystem*> m_rMaterialSystem;
		CReference<CRenderer*> m_rRenderer;
		CReference<CUtilityDraw*> m_rUtilityDraw;
		CReference<CDebugGUI*> m_rDebugGUI;

};
#endif