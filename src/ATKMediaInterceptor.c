#include <windows.h>
#include <stdio.h>

/**
 * 2011, zaak404@gmail.com
 */

#define MAX_CLASS_NAME 128

HINSTANCE hInst;
TCHAR szTitle[] = "ATKMEDIA";
TCHAR szWindowClass[] = "ATKMEDIA";
TCHAR szSettingsFileName[] = "icpt.conf";
TCHAR szSettingsPathKey[] = "application_path";
TCHAR szSettingsClassKey[] = "window_class";
TCHAR szAppPath[MAX_PATH];
TCHAR szAppWindowClass[MAX_CLASS_NAME];

ATOM MRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL LoadSettings();

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
//int main(int argc, char** argv)
//{
//	HINSTANCE hInstance = 0;
//    HINSTANCE hPrevInstance = 0;
//    LPTSTR    lpCmdLine = 0;
//    int       nCmdShow = SW_SHOWNORMAL;

	ZeroMemory(szAppPath, MAX_PATH);
	ZeroMemory(szAppWindowClass, MAX_CLASS_NAME);

	if(!LoadSettings())
	{
		MessageBox(0, "Can't load settings file\n", "ATKMEDIA by zaak404", MB_ICONERROR);
		return FALSE;
	}

	//printf("path: %s\n", szAppPath);
	//printf("class: %s\n", szAppWindowClass);

	MSG msg;

	MRegisterClass(hInstance);

	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int) msg.wParam;
}

ATOM MRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= 0;//LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= 0;//LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));

	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   //ShowWindow(hWnd, nCmdShow);
   //UpdateWindow(hWnd);

   return TRUE;
}

#define WM_APPCOMMAND                     0x0319
#define APPCOMMAND_MEDIA_NEXTTRACK        11
#define APPCOMMAND_MEDIA_PREVIOUSTRACK    12
#define APPCOMMAND_MEDIA_STOP             13
#define APPCOMMAND_MEDIA_PLAY_PAUSE       14

#define ATKMEDIA_MESSAGE				  0x0917
#define ATKMEDIA_PLAY                     0x0002
#define ATKMEDIA_STOP                     0x0003
#define ATKMEDIA_PREV                     0x0005
#define ATKMEDIA_NEXT                     0x0004

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		HWND hAppWindow;

		switch (wmId)
		{
		case WM_CLOSE:
			DestroyWindow(hWnd);
			break;

		case ATKMEDIA_MESSAGE:

			hAppWindow = FindWindow(szAppWindowClass, NULL);
			if(!hAppWindow)
			{
				ShellExecute(NULL, NULL, szAppPath, NULL, NULL, SW_SHOWNORMAL);
				break;
			}

			switch(wmEvent)
			{
			case ATKMEDIA_PLAY:
				PostMessage(hAppWindow, WM_APPCOMMAND, 0, MAKELPARAM(0, APPCOMMAND_MEDIA_PLAY_PAUSE));
				break;
			case ATKMEDIA_STOP:
				PostMessage(hAppWindow, WM_APPCOMMAND, 0, MAKELPARAM(0, APPCOMMAND_MEDIA_STOP));
				break;
			case ATKMEDIA_NEXT:
				PostMessage(hAppWindow, WM_APPCOMMAND, 0, MAKELPARAM(0, APPCOMMAND_MEDIA_NEXTTRACK));
				break;
			case ATKMEDIA_PREV:
				PostMessage(hAppWindow, WM_APPCOMMAND, 0, MAKELPARAM(0, APPCOMMAND_MEDIA_PREVIOUSTRACK));
				break;
			}

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

BOOL LoadSettings()
{
	TCHAR modulePath[MAX_PATH];
	TCHAR confFilePath[MAX_PATH];
	ZeroMemory(modulePath, MAX_PATH);
	ZeroMemory(confFilePath, MAX_PATH);
	
	GetModuleFileName(NULL /*current process*/, modulePath, MAX_PATH);

	char * c = strrchr(modulePath, '\\') + 1;
	*c= '\0';

	strcat(confFilePath, modulePath);
	strcat(confFilePath, szSettingsFileName);

	//MessageBox(0, confFilePath, "confFilePath", 0);

	FILE *f = fopen(confFilePath, "r");
	
	if(!f)
	{
		//puts("Can't see settings.ini");
		return 0;
	}

	fseek(f , 0 , SEEK_END);
	size_t fSize = ftell(f);
	rewind(f);

	char *buffer = (char*)malloc(fSize);
	ZeroMemory(buffer, fSize);
	if(buffer)
	{
		int result = fread(buffer, sizeof(char), fSize, f);

		if(result)
		{
			// Class name
			char* className = strstr(buffer, szSettingsClassKey) + strlen(szSettingsClassKey);
			while(*className == ' ' || *className == '=')
				++className;

			int bytesWritten = 0;
			while(*className != '\n' && *className != '\r' && *className != '\0' && bytesWritten < MAX_CLASS_NAME-1)
			{
				szAppWindowClass[bytesWritten++] = *(className++);
			}

			// Path
			char* path = strstr(buffer, szSettingsPathKey) + strlen(szSettingsPathKey);
			while(*path == ' ' || *path == '=')
				++path;

			bytesWritten = 0;
			while(*path != '\n' && *path != '\r' && *path != '\0' && bytesWritten < MAX_PATH-1)
			{
				szAppPath[bytesWritten++] = *(path++);
			}
			
		}
		free(buffer);
	}

	fclose(f);

	return strlen(szAppWindowClass) != 0;
}