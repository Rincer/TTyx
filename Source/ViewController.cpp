#include "stdafx.h"

#include "Input.h"
#include "View.h"
#include "ViewController.h"
#include "Macros.h"

//----------------------------------------------------------------------------------------------
CViewController::CViewController(CInput* pInput, CView* pView) :
m_pInput(pInput),
m_pView(pView),
m_LeftMouseButtonPressed(false),
m_IsConsoleMode(false)
{
	m_ViewControllerInput.SetContext(this);
	pInput->RegisterInterface(&m_ViewControllerInput);
	m_PitchVelocity = 10.0f / XM_PI;
	m_YawVelocity = 10.0f / XM_PI;
	m_RollVelocity = 10.0f / XM_PI;
	m_ForwardVelocity = 100.0f;
	m_LateralVelocity = 100.0f;
}

void CViewController::Tick(float DeltaSec)
{
	m_pView->Update(m_PitchVelocity * DeltaSec * m_PitchChange,
					m_YawVelocity * DeltaSec * m_YawChange,
					m_RollVelocity * DeltaSec * m_RollChange,
					m_ForwardVelocity * DeltaSec * m_ForwardChange,
					m_LateralVelocity * DeltaSec * m_LateralChange);
	m_PitchChange = 0.0f;
	m_YawChange = 0.0f;
	m_RollChange = 0.0f;
}


//----------------------------------------------------------------------------------------------
void CViewController::Forward(float Val)
{
	m_ForwardChange = Val;
}

//----------------------------------------------------------------------------------------------
void CViewController::Lateral(float Val)
{
	m_LateralChange = Val;
}

//----------------------------------------------------------------------------------------------
void CViewController::Pitch(float Val)
{
	Clamp(-10.0f, 10.0f, Val);
	m_PitchChange = Val;
}

//----------------------------------------------------------------------------------------------
void CViewController::Yaw(float Val)
{
	Clamp(-10.0f, 10.0f, Val);
	m_YawChange = Val;
}


//----------------------------------------------------------------------------------------------
void CViewController::Roll(float Val)
{
	Clamp(-10.0f, 10.0f, Val);
	m_RollChange = Val;
}

//----------------------------------------------------------------------------------------------
void CViewController::ProcessHIDInput(const unsigned char* pInputBuffer)
{
	RAWINPUT* pRawInput = (RAWINPUT*)pInputBuffer;

	if (m_IsConsoleMode) // If in console mode don't process the input unless its the ~ character which turns off console mode
	{
		if ((pRawInput->header.dwType == RIM_TYPEKEYBOARD) &&
			(pRawInput->data.keyboard.Message == WM_KEYDOWN) &&
			(pRawInput->data.keyboard.VKey == VK_OEM_3))
		{
			m_IsConsoleMode = false;
		}
		return;
	}

	switch (pRawInput->header.dwType)
	{
		case RIM_TYPEMOUSE:
		{
			if (pRawInput->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)
			{
				if (CWindow::IsMouseCursorInCurrentView())
				{
					m_LeftMouseButtonPressed = true;
				}
			}
			if (pRawInput->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)
			{
				m_LeftMouseButtonPressed = false;
			}
			if (m_LeftMouseButtonPressed)
			{
				Yaw((float)pRawInput->data.mouse.lLastX);
				Pitch((float)pRawInput->data.mouse.lLastY);
			}
			break;
		}

		case RIM_TYPEKEYBOARD:
		{
			if (pRawInput->data.keyboard.Message == WM_KEYDOWN)
			{
				switch (pRawInput->data.keyboard.VKey)
				{
				case VK_OEM_3: // ~ character
				{
					m_IsConsoleMode = true;
					break;
				}

				case 'W':
				{
					Forward(1.0f);
					break;
				}

				case 'S':
				{
					Forward(-1.0f);
					break;
				}
				case 'D':
				{
					Lateral(1.0f);
					break;
				}

				case 'A':
				{
					Lateral(-1.0f);
					break;
				}
				case 'Q':
				{
					Roll(10.0f);
					break;
				}
				case 'E':
				{
					Roll(-10.0f);
					break;
				}
				}
			}

			if (pRawInput->data.keyboard.Message == WM_KEYUP)
			{
				switch (pRawInput->data.keyboard.VKey)
				{
					case 'W':
					case 'S':
					{
						Forward(0.0f);
						break;
					}
					case 'A':
					case 'D':
					{
						Lateral(0.0f);
						break;
					}

					case 'Q':
					case 'E':
					{
						Roll(0.0f);
						break;
					}
				}
			}
		}
	}
}
