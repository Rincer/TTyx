#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include "HIDInputInterface.h"
#include "List.h"
#include "Reference.h"

class CMaterial;
class CInput;
class CMaterialSystem;
class CRenderer;
class CUtilityDraw;
class CDebugGUI;

class CConsole
{
	public:
		CConsole(	CInput* pInput, 
					CMaterialSystem** ppMaterialSystem,
					CRenderer** ppRenderer,
					CUtilityDraw** ppUtilityDraw,
					CDebugGUI** ppDebugGUI);
		~CConsole();
				
		void Tick(float DeltaSec);
		
		class CConsoleInput : public IHIDInput<CConsole>
		{
			public:
				CConsoleInput();				
		};
				
				
		class CConsoleCommand : public IIterant<CList<CConsoleCommand>>
		{
			public:
				CConsoleCommand(const char* pToken, CConsole* pConsole);
				~CConsoleCommand();
				
				bool MatchesToken(const char* pInputLine, unsigned int& CharIndex);
				virtual CList<CConsoleCommand>::CIterator* GetIterator() { return &m_Iterator; }
				virtual bool Execute(const char* pCommandString) = 0; 
				
			private:
				char m_Token[256];
				CList<CConsoleCommand>::CIterator m_Iterator;
		};
		
		void ProcessHIDInput(const unsigned char* pInputBuffer);
		void AddCommand(CConsoleCommand* pCommand);
				
	private:
		void ToggleConsole();	
		bool IsConsoleOn();	
		void ToggleInsertMode();				
		void AddCharacter(unsigned short Key);
		void Backspace();
		void MoveCursor(int NumChars);
		void EndKey();
		void HomeKey();		
		void ReturnKey();
		void Scroll(int Lines);
	
		static const unsigned int scMaxInputLength = 256;	
		static const unsigned int scMaxInputLines = 16;	
		CConsoleInput	m_ConsoleInput;
		bool			m_IsConsoleOn;
		CMaterial*		m_pMaterial;
		int				m_CursorPos;
		int				m_InputLength;
		char			m_InputString[scMaxInputLines][scMaxInputLength];
		bool			m_CursorOn;
		bool			m_InsertMode;
		float			m_CursorTimer;
		float			m_CursorTimerDelay;	
		unsigned int	m_CurrentLine;	
		CList<CConsoleCommand>	m_RegisteredCommands;
// System level components
		CReference<CMaterialSystem*> m_rMaterialSystem;
		CReference<CRenderer*> m_rRenderer;
		CReference<CUtilityDraw*> m_rUtilityDraw;
		CReference<CDebugGUI*> m_rDebugGUI;

		friend class	CConsoleInput;		
};


#endif _DEBUGGUI_H_