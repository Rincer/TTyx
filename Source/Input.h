#ifndef _INPUT_H_
#define _INPUT_H_

#include "Window.h"

class IHIDInputBase;

class CInput
{
	public:
		CInput();
				
		void Startup(HINSTANCE Instance);
		void Shutdown();
		void ProcessInput(LPARAM lParam);
		void RegisterInterface(IHIDInputBase* pInterface);
		void UnRegisterInterface(IHIDInputBase* pInterface);
		
	private:		
		CWindow m_Window; // Window that receives the input
		IHIDInputBase* m_pRegisteredInterfaces;

};
#endif