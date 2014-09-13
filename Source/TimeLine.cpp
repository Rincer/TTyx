#include "stdafx.h"
#include "Thread.h"
#include "Time.h"
#include "DebugGUI.h"
#include "MaterialSystem.h"
#include "UtilityDraw.h"
#include "Console.h"
#include "Macros.h"
#include "MaterialPlainColor.h"

#include "TimeLine.h"

static const unsigned int scLeftMargin = 10;
static const unsigned int scRightMargin = 10;
static const unsigned int scTopMargin = 10;	// pixels
static const unsigned int scThreadName = 10;	// characters
static const unsigned int scBarHeight = 16;

//----------------------------------------------------------------------------
CTimeLine::CScopedEvent::CScopedEvent(unsigned int Type, CTimeLine* pTimeLine) : m_pTimeLine(pTimeLine),
																				 m_Type(Type)
{	
	if (!m_pTimeLine->IsEnabled())
	{
		m_Type = 0xFFFFFFFF; // Set to invalid so that if timeline becomes enabled before the destructor is called we dont call stop
		return;
	}
	m_pTimeLine->StartEvent(m_Type);
}

//----------------------------------------------------------------------------
CTimeLine::CScopedEvent::~CScopedEvent()
{				
	if (!m_pTimeLine->IsEnabled() || (m_Type == 0xFFFFFFFF)) // Dont stop if wasnt started
		return;		
	m_pTimeLine->StopEvent(m_Type);
}

//----------------------------------------------------------------------------
CTimeLine::CTimeLine(CConsole* pConsole, 
					 CMaterialSystem** ppMaterialSystem,
					 CRenderer** ppRenderer,
					 CUtilityDraw** ppUtilityDraw,
					 CDebugGUI** ppDebugGUI) :	m_ToggleCommand("Timeline", pConsole, this),
												m_ThreadSelectCommand("Thread", pConsole, this),
												m_rMaterialSystem(ppMaterialSystem),
												m_rRenderer(ppRenderer),
												m_rUtilityDraw(ppUtilityDraw),
												m_rDebugGUI(ppDebugGUI)
{
	for(int i = 0; i < scMaxThreads; i++)
	{
		m_pThreadNames[i] = NULL;
		m_pEventDescs[i] = NULL;
	}
		
	memset(m_EventStack, 0, sizeof(m_EventStack));
	memset(m_EventStackPtr, 0, sizeof(m_EventStackPtr));	
	memset(m_Duration, 0, sizeof(m_Duration));
	memset(m_EventRead, 0, sizeof(m_EventRead));
	memset(m_EventWrite, 0, sizeof(m_EventWrite));
	memset(m_EventEnd, 0, sizeof(m_EventEnd));
	memset(m_pActive, 0, sizeof(m_pActive));
	m_TotalThreads = 0;		
	m_CurrFrame = 0;	
	m_FrameCount = 0;
	m_pMaterial = NULL; 
	m_CurrentThread = 0;
	m_Enabled = false;
	m_ToggleRequest = false;
}

//----------------------------------------------------------------------------
CTimeLine::~CTimeLine()
{

}
		
//----------------------------------------------------------------------------
void CTimeLine::RegisterThread(const char* pName, const CEventDesc* pEventDesc)
{
	long ThreadID = CThread::GetThreadID();
	Assert(ThreadID < scMaxThreads);
	m_pThreadNames[ThreadID] = pName;
	m_pEventDescs[ThreadID] = pEventDesc;
	if(pEventDesc)
	{
		unsigned int EventType = 0;
		while(pEventDesc->m_pName)
		{
			Assert(pEventDesc->m_Type == EventType); // Sanity check to make sure events are in expected order
			pEventDesc++;
			EventType++;
		}
	}
	m_TotalThreads++;
}
//----------------------------------------------------------------------------
void  CTimeLine::DrawEvent(	CEvent* pEvent, 
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
							const CEventDesc* pEventDesc)

{
	unsigned int Index = pEvent->m_Index;
	Assert(Index < scMaxEventTypesPerThread);
	unsigned long long StartTime;
	unsigned long long StopTime;
	XMFLOAT2 Uv0(0.0f, 0.0f);

	StartTime = MAX(pEvent->m_StartTime, m_FrameStart[PrevFrame]);
	if (pEvent->m_State == CEvent::eStopped)
	{
		StopTime = MIN(pEvent->m_StopTime, m_FrameEnd[PrevFrame]);
	}
	else
	{
		StopTime = m_FrameEnd[PrevFrame];
	}
	unsigned long long CycleTime = StopTime - StartTime;
	unsigned long Duration = (unsigned long)(CycleTime * 1000000 / ClockFrequency);
	if (m_CurrentThread == ThreadIndex)
	{
		m_Duration[Index] += Duration;
	}
	X1 = X0 + MicroSecondsToViewSpace * Duration;
	if ((X1 - X0) >= PixelWidth) // Only draw if bigger than a pixel
	{
		XMFLOAT3 Pt0(X0, Y0 - PixelHeight, 0.999f);
		XMFLOAT3 Pt1(X0, Y1 + PixelHeight, 0.999f);
		XMFLOAT3 Pt2(X1, Y0 - PixelHeight, 0.999f);
		XMFLOAT3 Pt3(X1, Y1 + PixelHeight, 0.999f);
		CColor Color = pEventDesc[Index].m_Color;
		m_rUtilityDraw->AddTriangle(Pt0, Pt2, Pt1, Uv0, Uv0, Uv0, Color, Color, Color);
		m_rUtilityDraw->AddTriangle(Pt1, Pt2, Pt3, Uv0, Uv0, Uv0, Color, Color, Color);
		X0 = X1;
	}
}

//----------------------------------------------------------------------------
void CTimeLine::Draw()
{
	if(!IsEnabled())
		return;

	unsigned int PrevFrame = 1 - m_CurrFrame; // draw the previously recorded frame, since the current one is still being filled
	unsigned long long FrameDuration = m_FrameEnd[PrevFrame] - m_FrameStart[PrevFrame];
	unsigned long long ClockFrequency = CTime::GetClockFrequency();
	unsigned long DurationMicroSecs = (unsigned long)(FrameDuration * 1000000 / ClockFrequency);
	unsigned int Width, Height;
	m_rRenderer->GetViewportSize(Width, Height);	

	// Draw the total frame duration	
	char FrameDurationSt[256];
	sprintf_s(FrameDurationSt, 255, "Frame %d.%dms", DurationMicroSecs / 1000, DurationMicroSecs % 1000);
	CColor White(255, 255, 255, 255);		
	m_rDebugGUI->DrawString((float)scLeftMargin, (float)scTopMargin, FrameDurationSt, White);
	
	if(m_pMaterial == NULL)
	{
		CMaterialPlainColor::CParameters Params("CTimeline.PlainColorParams", CMaterialPlainColor::eViewSpace);
		m_pMaterial = &m_rMaterialSystem->GetMaterial<CMaterialPlainColor>(&Params);
	}	
	
	unsigned int BarStart = scLeftMargin + scThreadName * 8 + 5; // 8 is the debug fonts character width
	unsigned int BarEnd = Width - scRightMargin;
	
	float PixelWidth = 2.0f / (float)Width;
	float PixelHeight = 2.0f / (float)Height;
	float ViewSpaceFrameWidth = (BarEnd - BarStart) * PixelWidth;
	float MicroSecondsToViewSpace = ViewSpaceFrameWidth / DurationMicroSecs;
			
	
	XMMATRIX LocalToWorld = XMMatrixIdentity();		
	XMFLOAT2 Uv0(0.0f, 0.0f);							
	
	m_rUtilityDraw->BeginTriangleList();	
	
	float X0;
	float X1;
	float Y0 = 1.0f - (scTopMargin + scBarHeight) * PixelHeight; // After frame duration
	float Y1 = Y0 - scBarHeight * PixelHeight;	
	
	for(unsigned int ThreadIndex = 0; ThreadIndex < scMaxThreads; ThreadIndex++)
	{
		X0 = -1.0f + BarStart * PixelWidth;
		X1 = X0; 
		const CEventDesc* pEventDesc = m_pEventDescs[ThreadIndex];
		if(pEventDesc)
		{
			if ((m_EventRead[ThreadIndex] == m_EventEnd[ThreadIndex]) && m_pActive[ThreadIndex])
			{
				CEvent* pEvent = m_pActive[ThreadIndex];
				if (pEvent->m_State == CEvent::eStarted)
				{
					DrawEvent(	pEvent,
								PrevFrame,
								ClockFrequency,
								ThreadIndex,
								MicroSecondsToViewSpace,
								X0,
								X1,
								Y0,
								Y1,
								PixelWidth,
								PixelHeight,
								pEventDesc);
				}
			}
			for ( ;(m_EventRead[ThreadIndex] != m_EventEnd[ThreadIndex]); m_EventRead[ThreadIndex] = (m_EventRead[ThreadIndex] + 1) % scMaxEvents)
			{
				unsigned int EventIndex = m_EventRead[ThreadIndex];
				CEvent* pEvent = &m_Events[ThreadIndex][EventIndex];
				DrawEvent(pEvent,
					PrevFrame,
					ClockFrequency,
					ThreadIndex,
					MicroSecondsToViewSpace,
					X0,
					X1,
					Y0,
					Y1,
					PixelWidth,
					PixelHeight,
					pEventDesc);

			}			
		}
		Y0 = Y1;
		Y1 = Y0 - scBarHeight * PixelHeight;	
	}
	m_rUtilityDraw->EndTriangleList(LocalToWorld, m_pMaterial, CRenderer::eDepthNone, CRenderer::eBlendModulate);	

	// Draw thread names	
	Y0 = scTopMargin + scBarHeight; // After frame duration, this is in pixels not view space
	X0 = scLeftMargin;
	for(unsigned int ThreadIndex = 0; ThreadIndex < scMaxThreads; ThreadIndex++)
	{
		if(m_pThreadNames[ThreadIndex])
		{
			m_rDebugGUI->DrawString(X0, Y0, m_pThreadNames[ThreadIndex], White);		
			Y0 += scBarHeight;
		}
	}			
	
	// Now draw all events for the current thread
	const CEventDesc* pEventDesc = m_pEventDescs[m_CurrentThread];
	if(pEventDesc)
	{
		while(pEventDesc->m_pName)
		{
			sprintf_s(FrameDurationSt, 255, "%-10s %d.%dms", pEventDesc->m_pName, m_Duration[pEventDesc->m_Type] / 1000, m_Duration[pEventDesc->m_Type] % 1000);
			m_rDebugGUI->DrawString(X0, Y0, FrameDurationSt, pEventDesc->m_Color);	
			Y0 += scBarHeight;	
			m_Duration[pEventDesc->m_Type] = 0;		
			pEventDesc++;
		}		
	}	
}

//----------------------------------------------------------------------------
void CTimeLine::FrameStart()
{	
	if (m_ToggleRequest) // Can only toggle at frame start
	{
		Toggle();
		m_ToggleRequest = false;
	}
	if (!IsEnabled())
	{
		return;
	}
	m_FrameStart[m_CurrFrame] = CTime::RDTSC();
}

//----------------------------------------------------------------------------
void CTimeLine::FrameEnd()
{
	if(!IsEnabled())
		return;

	m_FrameEnd[m_CurrFrame] = CTime::RDTSC();		
	m_CurrFrame = 1 - m_CurrFrame;
	for(unsigned int ThreadIndex = 0; ThreadIndex < 8; ThreadIndex++)
	{
		m_EventEnd[ThreadIndex] = m_EventWrite[ThreadIndex];
	}	
	m_FrameCount++;
}
		
//----------------------------------------------------------------------------		
void CTimeLine::Toggle()		
{
	m_Enabled = !m_Enabled;
}

//----------------------------------------------------------------------------		
void CTimeLine::ThreadSelect(const char* pThreadName)
{
	const char* pName = pThreadName;
	while((*pName == ' ') && (*pName))
		pName++;
	
	for(unsigned int ThreadIndex = 0; ThreadIndex < m_TotalThreads; ThreadIndex++)
	{
		if(!_stricmp(pName, m_pThreadNames[ThreadIndex]))
		{
			m_CurrentThread = ThreadIndex;
			return;
		}
	}
}

//----------------------------------------------------------------------------		
void CTimeLine::ToggleRequest()		
{
	m_ToggleRequest = true;
}

//----------------------------------------------------------------------------		
bool CTimeLine::IsEnabled()
{
	return m_Enabled;
}

//----------------------------------------------------------------------------
void CTimeLine::StartEvent(unsigned int EventIndex)
{
	StartEventInternal(EventIndex, false);
}

//----------------------------------------------------------------------------
void CTimeLine::StopEvent(unsigned int EventIndex)
{
	StopEventInternal(EventIndex, false);
}

//----------------------------------------------------------------------------
void CTimeLine::StartEventInternal(unsigned int EventIndex, bool Internal)
{
	long ThreadID = CThread::GetThreadID();
	CEvent* pActive = m_pActive[ThreadID];
	if ((pActive != NULL) && !Internal) // Only push active event onto the stack if its called from outside
	{
		// push current event to the event stack and stop it
		Assert(m_EventStackPtr[ThreadID] < CTimeLine::scEventStackSize);
		m_EventStack[ThreadID][m_EventStackPtr[ThreadID]] = pActive;
		m_EventStackPtr[ThreadID]++;
		StopEventInternal(pActive->m_Index, true);
	}
	unsigned int WriteIndex = m_EventWrite[ThreadID];
	CEvent* pEvent = &m_Events[ThreadID][WriteIndex];
	pEvent->m_Index = EventIndex;
	pEvent->m_StartTime = CTime::RDTSC(); 
	pEvent->m_State = CEvent::eStarted;
	m_pActive[ThreadID] = pEvent;
	unsigned int NextWriteIndex = (m_EventWrite[ThreadID] + 1) % scMaxEvents;
	Assert(NextWriteIndex != m_EventRead[ThreadID]);	// Check for overwrite
	m_EventWrite[ThreadID] = NextWriteIndex;			// Increment write index

}

//----------------------------------------------------------------------------
void CTimeLine::StopEventInternal(unsigned int EventIndex, bool Internal)
{
	long ThreadID = CThread::GetThreadID();
	CEvent* pEvent = m_pActive[ThreadID];
	Assert(pEvent->m_Index == EventIndex); // sanity check, only 1 event can be active 
	pEvent->m_StopTime = CTime::RDTSC();
	pEvent->m_State = CEvent::eStopped;	
	m_pActive[ThreadID] = NULL; // No event is currently running on this thread
	if ((m_EventStackPtr[ThreadID] > 0) && !Internal)  // Only pop event from the stack if its called from outside
	{
		// pop a higher scoped event from the stack and re-start it
		Assert(m_EventStackPtr[ThreadID] >= 1);
		m_EventStackPtr[ThreadID]--;
		StartEventInternal(m_EventStack[ThreadID][m_EventStackPtr[ThreadID]]->m_Index, true);
	}
}

//----------------------------------------------------------------------------
void CTimeLine::Shutdown()
{
}

// Console commands
//----------------------------------------------------------------------------
CTimeLine::CTimeLineToggle::CTimeLineToggle(const char* pToken, CConsole* pConsole, CTimeLine* pTimeLine) : CConsole::CConsoleCommand(pToken, pConsole),
																											m_pTimeLine(pTimeLine)
{
}

//----------------------------------------------------------------------------
bool CTimeLine::CTimeLineToggle::Execute(const char* pCommandString)
{
	pCommandString; // C4100
	m_pTimeLine->ToggleRequest();
	return true;
}

//----------------------------------------------------------------------------
CTimeLine::CTimeLineThreadSelect::CTimeLineThreadSelect(const char* pToken, CConsole* pConsole, CTimeLine* pTimeLine) : CConsole::CConsoleCommand(pToken, pConsole),
																														m_pTimeLine(pTimeLine)
{
}

//----------------------------------------------------------------------------
bool CTimeLine::CTimeLineThreadSelect::Execute(const char* pCommandString)
{
	m_pTimeLine->ThreadSelect(pCommandString);
	return true;
}
