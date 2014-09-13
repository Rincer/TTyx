#include "stdafx.h"

#include "Window.h"

HWND CWindow::s_mCurrentViewWnd;

//----------------------------------------------------------------------------------------------
void CWindow::Initialize(WNDPROC WindowProc, HINSTANCE Instance, int CmdShow, LPCWSTR pName, LPCWSTR pDesc, int Width, int Height)
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WindowProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = Instance;
    wcex.hIcon = 0;
    wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = pName;
    wcex.hIconSm = 0;
    WORD Res = RegisterClassEx( &wcex );
	Assert(Res);

    // Create window
    m_Inst = Instance;
    RECT rc = { 0, 0, Width, Height };
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    m_Wnd = CreateWindow( pName, pDesc, WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, Instance,
                           NULL );
	Assert(m_Wnd);
    ShowWindow( m_Wnd, CmdShow );	
    UpdateWindow(m_Wnd);
}

//----------------------------------------------------------------------------------------------
HWND CWindow::GetWnd()
{
	return m_Wnd;
}

//----------------------------------------------------------------------------------------------
CWindow::CWindow()
{
	m_Wnd = 0;
	m_Inst = 0;
}

//----------------------------------------------------------------------------------------------
CWindow::~CWindow()
{
}

//----------------------------------------------------------------------------------------------
void CWindow::SetCurrentViewWnd(HWND Wnd)
{
	s_mCurrentViewWnd = Wnd;
}

//----------------------------------------------------------------------------------------------
bool CWindow::IsMouseCursorInCurrentView()
{
	if(GetFocus() != s_mCurrentViewWnd)
		return false;
	WINDOWINFO Wi;
	POINT CursorPos;
    GetCursorPos(&CursorPos);        
	GetWindowInfo(s_mCurrentViewWnd, &Wi);
	return (CursorPos.x > Wi.rcClient.left) && (CursorPos.x < Wi.rcClient.right) && (CursorPos.y > Wi.rcClient.top) && (CursorPos.y < Wi.rcClient.bottom);
}