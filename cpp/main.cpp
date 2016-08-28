#include "stdafx.h"
#include "../VisualStudio/resource.h"

#define MAX_LOADSTRING 100

// Global Variables:
HWND hWnd;
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
HACCEL hAccelTable;

static void ShowLastError()
{
	wchar_t* msg;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&msg, 0, nullptr);
	MessageBox(hWnd, msg, L"", MB_OK);
	LocalFree(msg);
}

// WindowMessage
static BOOL ProcessWindowMessage(){
	MSG msg;
	for (;;){
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)){
			if (msg.message == WM_QUIT){
				return FALSE;
			}
			if (!TranslateAccelerator(hWnd, hAccelTable, &msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		BOOL active = !IsIconic(hWnd) && GetForegroundWindow() == hWnd;

		if (!active){
			WaitMessage();
			continue;
		}

		return TRUE;
	}
}

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);


#ifdef _DEBUG
int main(int, char**)
#else
int APIENTRY wWinMain(_In_ HINSTANCE,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR    lpCmdLine,
	_In_ int)
#endif

{
	HINSTANCE hInstance = GetModuleHandle(nullptr);

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_DX12PLAYGROUND, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DX12PLAYGROUND));


	int lastW = 0;
	int lastH = 0;

	// Main message loop:
	for (;;) {
		if (!ProcessWindowMessage()) {
			break;
		}

		RECT rc;
		GetClientRect(hWnd, &rc);
		int w = rc.right - rc.left;
		int h = rc.bottom - rc.top;
		if (w != lastW || h != lastH) {
			lastW = w;
			lastH = h;
			app.Destroy();
			deviceMan.Destroy();
			deviceMan.Create(hWnd);
			app.Init();
		}
		app.Update();
		app.Draw();
		deviceMan.Present();
		Sleep(1);
	}
	return 0;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASS wcex;

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DX12PLAYGROUND));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_DX12PLAYGROUND);
	wcex.lpszClassName	= szWindowClass;

	return RegisterClass(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance)
{
   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }
   ShowWindow(hWnd, SW_SHOWNORMAL);

   DragAcceptFiles(hWnd, TRUE);
   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	IVec2 screenSize = systemMisc.GetScreenSize();
	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_CLOSE:
		app.Destroy();
		deviceMan.Destroy();
		DestroyWindow(hWnd);
		return 0;
	case WM_DROPFILES:
	{
		HDROP hDrop = (HDROP)wParam;
		char fileName[MAX_PATH];
		DragQueryFileA(hDrop, 0, fileName, MAX_PATH);
		DragFinish(hDrop);
//		app.LoadMesh(fileName);
		break;
	}
	case WM_LBUTTONDOWN:
		SetCapture(hWnd);
		devCamera.LButtonDown(LOWORD(lParam) / (float)screenSize.x, HIWORD(lParam) / (float)screenSize.y);
		systemMisc.mouseDown = true;
		break;
	case WM_LBUTTONUP:
		ReleaseCapture();
		devCamera.LButtonUp(LOWORD(lParam) / (float)screenSize.x, HIWORD(lParam) / (float)screenSize.y);
		systemMisc.mouseDown = false;
		break;
	case WM_MOUSEMOVE:
		systemMisc.SetMousePos(IVec2(MAKEPOINTS(lParam).x, MAKEPOINTS(lParam).y));
		devCamera.MouseMove(MAKEPOINTS(lParam).x / (float)screenSize.x, MAKEPOINTS(lParam).y / (float)screenSize.y);
		break;
	case WM_MOUSEWHEEL:
		devCamera.MouseWheel((short)HIWORD(wParam) / (float)WHEEL_DELTA);
		break;
	case WM_SIZE:
		systemMisc.SetScreenSize(IVec2(LOWORD(lParam), HIWORD(lParam)));
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
