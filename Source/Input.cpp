#include "stdafx.h"
#include "HIDInputInterface.h"

#include "Input.h"

// from hidusage.h

#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC		((USHORT) 0x01)
#endif
#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE		((USHORT) 0x02)
#endif

#ifndef HID_USAGE_GENERIC_KEYBOARD
#define HID_USAGE_GENERIC_KEYBOARD	((USHORT) 0x06)
#endif

static CInput* s_pInput = NULL; // Assumes only one input object

//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK InputWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch( message )
    {
		case WM_NCCREATE:
		{
        	RAWINPUTDEVICE Rid[1];
			Rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC; 
			Rid[0].usUsage = HID_USAGE_GENERIC_MOUSE; 
			Rid[0].dwFlags = RIDEV_INPUTSINK;   
			Rid[0].hwndTarget = hWnd;
			RegisterRawInputDevices(Rid, 1, sizeof(Rid[0]));
			Rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC; 
			Rid[0].usUsage = HID_USAGE_GENERIC_KEYBOARD; 
			Rid[0].dwFlags = RIDEV_INPUTSINK;   
			Rid[0].hwndTarget = hWnd;
			RegisterRawInputDevices(Rid, 1, sizeof(Rid[0]));			
            return TRUE;
		}
		
		case WM_INPUT: 
		{
			s_pInput->ProcessInput(lParam);
			break;
		}
   
        case WM_DESTROY:
        {
            PostQuitMessage( 0 );
            break;
		}

        default:
            return DefWindowProc( hWnd, message, wParam, lParam );
    }
    return 0;
}


//------------------------------------------------------------------------------------
CInput::CInput()
{
	m_pRegisteredInterfaces = NULL;
	s_pInput = this;
}

//------------------------------------------------------------------------------------
void CInput::Startup(HINSTANCE Instance)
{
	// Create a separate window to capture the input?
	// Some online discussions talk about capturing the input on a separate thread, apparently DirectInput does it that way, but I dont see how since the
	// WM_INPUT message comes from the main thread.
	m_Window.Initialize(InputWndProc, Instance, 0, L"TTyxInput", L"TTyxInput", 0, 0);
}

//------------------------------------------------------------------------------------
void CInput::ProcessInput(LPARAM lParam)
{
	unsigned int Size = 40;
	unsigned char InputBuffer[40];

	GetRawInputData((HRAWINPUT)lParam, RID_INPUT, InputBuffer, &Size, sizeof(RAWINPUTHEADER));	    
	for(IHIDInputBase* pInterface = m_pRegisteredInterfaces; pInterface != NULL; pInterface = pInterface->m_pNext)
	{
		pInterface->ProcessHIDInput(InputBuffer);
	}	
}

//------------------------------------------------------------------------------------
void CInput::Shutdown()
{
}


//------------------------------------------------------------------------------------
void CInput::RegisterInterface(IHIDInputBase* pInterface)
{
	if(m_pRegisteredInterfaces == NULL)	// empty list
	{
		pInterface->m_pNext = NULL;
	}
	else					// add at the start
	{
		pInterface->m_pNext = m_pRegisteredInterfaces;
		m_pRegisteredInterfaces->m_pPrev = pInterface;
	}
	pInterface->m_pPrev = NULL;						
	m_pRegisteredInterfaces = pInterface;	
}

//------------------------------------------------------------------------------------
void CInput::UnRegisterInterface(IHIDInputBase* pInterface)
{
	if(pInterface == m_pRegisteredInterfaces)	// remove at start
	{
		m_pRegisteredInterfaces = pInterface->m_pNext;
		if(m_pRegisteredInterfaces)
		{
			m_pRegisteredInterfaces->m_pPrev = NULL;
		}
	}
	else						// remove at middle or end
	{
		if(pInterface->m_pNext)
		{
			pInterface->m_pNext->m_pPrev = pInterface->m_pPrev;
		}
		pInterface->m_pPrev->m_pNext = pInterface->m_pNext; 
	}
}