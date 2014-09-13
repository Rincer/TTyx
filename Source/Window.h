#ifndef _WINDOW_H_
#define	_WINDOW_H_
// Encapsulates basic OS window

class CWindow
{
	public:
		CWindow();
		~CWindow();
		void Initialize(WNDPROC WindowProc, HINSTANCE Instance, int CmdShow, LPCWSTR pName, LPCWSTR pDesc, int Width, int Height);
		HWND GetWnd();
		static void SetCurrentViewWnd(HWND Wnd);
		static bool IsMouseCursorInCurrentView();
		
	private:
		HINSTANCE	m_Inst;
		HWND        m_Wnd;
		static HWND	s_mCurrentViewWnd;
};

#endif