#include "stdafx.h"
#include "Input.h"
#include "MaterialSystem.h"
#include "Renderer.h"
#include "UtilityDraw.h"
#include "DebugGUI.h"
#include "MaterialPlainColor.h"

#include "Console.h"

//------------------------------------------------------------------------------------------------
CConsole::CConsole(	CInput* pInput,
					CMaterialSystem** ppMaterialSystem,
					CRenderer** ppRenderer,
					CUtilityDraw** ppUtilityDraw,
					CDebugGUI** ppDebugGUI) : m_rMaterialSystem(ppMaterialSystem),
											m_rRenderer(ppRenderer),
											m_rUtilityDraw(ppUtilityDraw),
											m_rDebugGUI(ppDebugGUI)
{	
	m_CursorPos = 1;
	m_InputLength = 0;
	m_CursorOn = true;
	m_InsertMode = true;
	m_CursorTimerDelay = 0.5f;
	for( unsigned int StringIndex = 0; StringIndex < scMaxInputLines; StringIndex++)
	{
		m_InputString[StringIndex][0] = '\0';	
	}	
	m_CursorTimer = m_CursorTimerDelay;
	m_CurrentLine = 0;				
	m_ConsoleInput.SetContext(this);
	pInput->RegisterInterface(&m_ConsoleInput);
}

//------------------------------------------------------------------------------------------------
CConsole::~CConsole()
{
}

//------------------------------------------------------------------------------------------------
void CConsole::Tick(float DeltaSec)
{
	if(m_IsConsoleOn)
	{
		if(m_pMaterial == NULL)
		{
			CMaterialPlainColor::CParameters Params("CConsole.PlainColorParams", CMaterialPlainColor::eViewSpace);
			m_pMaterial = &m_rMaterialSystem->GetMaterial<CMaterialPlainColor>(&Params);
		}
		m_CursorTimer -= DeltaSec;
		if(m_CursorTimer < 0.0f)
		{
			m_CursorOn = !m_CursorOn;
			m_CursorTimer = m_CursorTimerDelay;
		}
		unsigned int Width, Height;
		m_rRenderer->GetViewportSize(Width, Height);	
		float PixelHeight = 2.0f / (float)Height;		
		float InputTop = -1.0f + 18.0f * PixelHeight;
		float InputBot = -1.0f;
		float LineTop = -1.0f + 17.0f * PixelHeight;
		float LineBot = -1.0f;
		XMFLOAT3 Pt0(-1.0f, InputTop, 0.999f);
		XMFLOAT3 Pt1(-1.0f, InputBot, 0.999f);		
		XMFLOAT3 Pt2( 1.0f, InputBot, 0.999f);				
		XMFLOAT3 Pt3( 1.0f, InputTop, 0.999f);	
		XMFLOAT2 Uv(0.0f, 0.0f);
		CColor Color(0, 0, 0, 0);				
		XMMATRIX LocalToWorld = XMMatrixIdentity();	
		m_rUtilityDraw->BeginTriangleList();		
		m_rUtilityDraw->AddTriangle(Pt0, Pt3, Pt1, Uv, Uv, Uv, Color, Color, Color);
		m_rUtilityDraw->AddTriangle(Pt1, Pt3, Pt2, Uv, Uv, Uv, Color, Color, Color);
		m_rUtilityDraw->EndTriangleList(LocalToWorld, m_pMaterial, CRenderer::eDepthNone, CRenderer::eBlendNone);	
		XMFLOAT3 LinePt0(-1.0f, LineTop, 0.999f);
		XMFLOAT3 LinePt1( 1.0f, LineTop, 0.999f);		
		XMFLOAT3 LinePt2(-1.0f, LineBot, 0.999f);				
		XMFLOAT3 LinePt3( 1.0f, LineBot, 0.999f);	
		CColor ColorLine(0, 128, 128, 128);						
		m_rUtilityDraw->BeginLineList();		
		m_rUtilityDraw->AddLine(LinePt0, LinePt1, ColorLine, ColorLine);
		m_rUtilityDraw->AddLine(LinePt2, LinePt3, ColorLine, ColorLine);
		m_rUtilityDraw->EndLineList(LocalToWorld, m_pMaterial, CRenderer::eDepthNone, CRenderer::eBlendNone);	
		m_rDebugGUI->DrawString(0, (float)Height - 17, ">", ColorLine);
		m_rDebugGUI->DrawString(8.0f, (float)Height - 17, m_InputString[m_CurrentLine], ColorLine);		
		if(m_CursorOn)
		{
			m_rDebugGUI->DrawString(8.0f * m_CursorPos, (float)Height - 14, "_", ColorLine);
		}
	}
}

//------------------------------------------------------------------------------------------------
void CConsole::ToggleConsole()
{
	m_IsConsoleOn = !m_IsConsoleOn;
}

//------------------------------------------------------------------------------------------------
bool CConsole::IsConsoleOn()
{
	return m_IsConsoleOn;
}

//------------------------------------------------------------------------------------------------
void CConsole::ToggleInsertMode()
{
	m_InsertMode = !m_InsertMode;
}

//------------------------------------------------------------------------------------------------
void CConsole::AddCharacter(unsigned short Key)
{
	if(m_InsertMode)
	{
		Assert(m_InputLength < scMaxInputLength - 1);
		int CharIndex;
		for(CharIndex = m_InputLength + 1; CharIndex > m_CursorPos - 1; CharIndex--)
		{
			m_InputString[m_CurrentLine][CharIndex] = m_InputString[m_CurrentLine][CharIndex - 1];
		}
		m_InputString[m_CurrentLine][CharIndex] = (char)Key;
		m_InputLength++;
		m_CursorPos++;
	}	
	else
	{
		if(m_InputLength == m_CursorPos - 1)
		{
			Assert(m_InputLength < scMaxInputLength - 1);
		}
		m_InputString[m_CurrentLine][m_CursorPos - 1] = (char)Key;
		m_CursorPos++;
		if(m_CursorPos - 1 > m_InputLength)
		{
			m_InputLength++;
			m_InputString[m_CurrentLine][m_InputLength] = '\0';
		}
	}
}

//------------------------------------------------------------------------------------------------
void CConsole::Backspace()
{
	if(m_CursorPos > 1)
	{
		for(int CharIndex = m_CursorPos - 2; CharIndex < m_InputLength; CharIndex++)
		{
			m_InputString[m_CurrentLine][CharIndex] = m_InputString[m_CurrentLine][CharIndex + 1];
		}
		m_CursorPos--;
		m_InputLength--;
	}
}

//------------------------------------------------------------------------------------------------
void CConsole::MoveCursor(int NumChars)
{
	if((NumChars + m_CursorPos > 0) && (NumChars + m_CursorPos <= m_InputLength + 1))
	{
		m_CursorPos += NumChars;
	}
}

//------------------------------------------------------------------------------------------------
void CConsole::EndKey()
{
	m_CursorPos = m_InputLength + 1;
}

//------------------------------------------------------------------------------------------------
void CConsole::HomeKey()
{
	m_CursorPos = 1;
}

//------------------------------------------------------------------------------------------------
void CConsole::ReturnKey()
{
	for (CList<CConsoleCommand>::CIterator* pIterator = m_RegisteredCommands.GetFirst(); pIterator; pIterator = pIterator->Next())
	{
		CConsoleCommand* pCommand = pIterator->GetData();
		unsigned int CharIndex; // Index of the next char after the token
		if(pCommand->MatchesToken(m_InputString[m_CurrentLine], CharIndex))
		{
			pCommand->Execute(&m_InputString[m_CurrentLine][CharIndex]);
			break;
		}
	}	
	m_CurrentLine = (m_CurrentLine + 1) % scMaxInputLines;
	m_CursorPos = 1;
	m_InputString[m_CurrentLine][0] = '\0';
}

//------------------------------------------------------------------------------------------------
void CConsole::Scroll(int Lines)
{
	int NewLine = m_CurrentLine + Lines;
	if(NewLine < 0)
		NewLine = scMaxInputLines - 1;
	else if ( NewLine > (scMaxInputLines - 1))
		NewLine = 0;

	if(m_InputString[NewLine][0] != '\0')
	{
		m_CurrentLine = NewLine;
		m_CursorPos = strlen(m_InputString[NewLine]) + 1;
	}	
}

//------------------------------------------------------------------------------------------------
void CConsole::AddCommand(CConsoleCommand* pCommand)
{
	m_RegisteredCommands.AddBack(pCommand);
}

//------------------------------------------------------------------------------------------------
CConsole::CConsoleInput::CConsoleInput()
{
}

//------------------------------------------------------------------------------------------------
void CConsole::ProcessHIDInput(const unsigned char* pInputBuffer)
{
	RAWINPUT* pRawInput = (RAWINPUT*)pInputBuffer;	
	    
	switch(pRawInput->header.dwType)
	{
		case RIM_TYPEKEYBOARD:		
		{
			if(pRawInput->data.keyboard.Message == WM_KEYDOWN)
			{
				if(pRawInput->data.keyboard.VKey == VK_OEM_3) // ~ character
				{
					ToggleConsole();
				}				
				else if(IsConsoleOn())
				{				
					if (pRawInput->data.keyboard.VKey == VK_BACK)
					{
						Backspace();
					}	
					else if (pRawInput->data.keyboard.VKey == VK_LEFT)
					{
						MoveCursor(-1);
					}					
					else if (pRawInput->data.keyboard.VKey == VK_RIGHT)
					{
						MoveCursor(1);
					}									
					else if (pRawInput->data.keyboard.VKey == VK_INSERT)
					{
						ToggleInsertMode();
					}
					else if (pRawInput->data.keyboard.VKey == VK_HOME)
					{
						HomeKey();
					}
					else if (pRawInput->data.keyboard.VKey == VK_END)
					{
						EndKey();
					}				
					else if (pRawInput->data.keyboard.VKey == VK_RETURN)
					{
						ReturnKey();
					}								
					else if (pRawInput->data.keyboard.VKey == VK_UP)
					{
						Scroll(-1);
					}												
					else if (pRawInput->data.keyboard.VKey == VK_DOWN)
					{
						Scroll(1);
					}																
					else
					{
						unsigned char ks[256];
						GetKeyboardState(ks);				
						unsigned short Char;
						int Res = ToAscii(pRawInput->data.keyboard.VKey, 0, ks, &Char, 1);
						if(Res == 1)
						{
							AddCharacter(Char & 0xFF);
						}
					}				
				}
			} 
		}		
		default:
			break;
	}
}

//---------------------------------------------------------------------------------------------			
CConsole::CConsoleCommand::CConsoleCommand(const char* pToken, CConsole* pConsole) : m_Iterator(this)
{
	strcpy_s(m_Token, 255, pToken);
	pConsole->AddCommand(this);
}

//---------------------------------------------------------------------------------------------
CConsole::CConsoleCommand::~CConsoleCommand()
{
}
				
//---------------------------------------------------------------------------------------------
bool CConsole::CConsoleCommand::MatchesToken(const char* pInputLine, unsigned int& CharIndex)
{
	for(CharIndex = 0; CharIndex < strlen(pInputLine); CharIndex++)
	{
		if((pInputLine[CharIndex] == ' ') && (CharIndex))			
			break;
		unsigned char Src = m_Token[CharIndex];	
		if(Src == 0) // end of token
		{
			return false;
		}
		unsigned char Dst = pInputLine[CharIndex];	
		// Convert to lower case
		if((Src >= 'A') && (Src <= 'Z'))
		{
			Src += 0x20;
		}
		if((Dst >= 'A') && (Dst <= 'Z'))
		{
			Src += 0x20;
		}		
		if(Src != Dst)
		{
			return false;
		}
	}	
	return true;
}


