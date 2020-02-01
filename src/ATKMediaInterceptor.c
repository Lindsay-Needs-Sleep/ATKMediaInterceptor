#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>

/**
 * 2011, zaak404@gmail.com
 */

#define MAX_CLASS_NAME 128

HINSTANCE hInst;
TCHAR szTitle[] = "ATKMEDIA";
TCHAR szWindowClass[] = "ATKMEDIA";
TCHAR CONFIG_FILE_NAME[] = "ATKMediaInterceptor.conf";
TCHAR CONFIG_FILE_PATH[MAX_PATH];
TCHAR CONFIG_KEY_MediaApplicationPath[] = "media_application_path";
TCHAR mediaApplicationPath[MAX_PATH];

ATOM MRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
int FindApplicationPid(TCHAR* appPath);
HWND FindApplicationWindow(TCHAR* appPath);
HWND EnsureApplicationIsRunning(TCHAR* appPath);
void OpenApp(TCHAR* path);
BOOL LoadSettings();
BOOL GetAndSetSetting(char* settings, TCHAR* key, TCHAR* valueStr);
void AlertErrorAndExit(char* errorMsg);

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
//int main(int argc, char** argv)
//{
//	HINSTANCE hInstance = 0;
//    HINSTANCE hPrevInstance = 0;
//    LPTSTR    lpCmdLine = 0;
//    int       nCmdShow = SW_SHOWNORMAL;

	LoadSettings();

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

   return TRUE;
}

#define WM_APPCOMMAND                     0x0319
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

			hAppWindow = EnsureApplicationIsRunning(mediaApplicationPath);

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

// Based on https://www.12ghast.com/code/c-process-name-to-pid/
int FindApplicationPid(TCHAR* appPath)
{
	// Create a snapshot of currently running processes
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	// Some error handling in case we failed to get a snapshot of running processes
	if(snap == INVALID_HANDLE_VALUE)
	{
		// Clean the snapshot object to prevent resource leakage
		CloseHandle(snap);
		AlertErrorAndExit(GetLastError());
	}

	// Declare a PROCESSENTRY32 variable
	PROCESSENTRY32 pe32;
	// Set the size of the structure before using it.
	pe32.dwSize = sizeof(PROCESSENTRY32);

	// Retrieve information about the first process and exit if unsuccessful
	if(!Process32First(snap, &pe32))
	{
		// Clean the snapshot object to prevent resource leakage
		CloseHandle(snap);
		AlertErrorAndExit(GetLastError());
	}

	int pid = 0;
	char* target = strrchr(appPath, '\\') + 1;
	// Cycle through Process List
	do {
		// Comparing two strings containing process names for 'equality'
		if(strcmp(pe32.szExeFile, target) == 0) {
			pid = pe32.th32ProcessID;
			break;
		}
	} while (Process32Next(snap, &pe32));
	// Clean the snapshot object to prevent resource leakage
	CloseHandle(snap);
	return pid;
}

HWND FindApplicationWindow(TCHAR* appPath)
{
	int pid = FindApplicationPid(appPath);
	if(!pid)
	{
		return 0;
	}
	HWND appWindow = NULL;
	BOOL CALLBACK EnumWindowsProcMy(HWND hwnd, LPARAM wantedPid)
	{
		DWORD pid;
		GetWindowThreadProcessId(hwnd, &pid);
		if(pid == wantedPid)
		{
			appWindow = hwnd;
			return FALSE;
		}
		return TRUE;
	}
	EnumWindows(EnumWindowsProcMy, pid);
	return appWindow;
}

HWND EnsureApplicationIsRunning(TCHAR* appPath)
{
	HWND win = FindApplicationWindow(appPath);
	// If can't find Application PID
	if(!win)
	{
		// Try to open the Application
		OpenApp(appPath);
		// Give the window a second to open
		sleep(1);
		// If we still can't find the Application PID
		win = FindApplicationWindow(appPath);
		if(!win)
		{
			// The appPath is probably wrong, so report it
			char format[] = "Can't find or start the application at path: %s. Ensure the config file is correct.";
			char message[strlen(format) + strlen(appPath)];
			sprintf(message, format, appPath);
			AlertErrorAndExit(message);
		}
	}
	return win;
}

void OpenApp(TCHAR* path)
{
	ShellExecute(NULL, NULL, path, NULL, NULL, SW_SHOW);
}

BOOL LoadSettings()
{
	TCHAR modulePath[MAX_PATH];
	ZeroMemory(modulePath, MAX_PATH);
	ZeroMemory(CONFIG_FILE_PATH, MAX_PATH);
	ZeroMemory(mediaApplicationPath, MAX_PATH);

	GetModuleFileName(NULL /*current process*/, modulePath, MAX_PATH);

	char * c = strrchr(modulePath, '\\') + 1;
	*c= '\0';

	strcat(CONFIG_FILE_PATH, modulePath);
	strcat(CONFIG_FILE_PATH, CONFIG_FILE_NAME);

//	printf("settings path %s \n" , CONFIG_FILE_PATH);

	FILE *f = fopen(CONFIG_FILE_PATH, "r");

	if(!f)
	{
		AlertErrorAndExit("Could not find config file.");
	}

	fseek(f , 0 , SEEK_END);
	size_t fSize = ftell(f);
	if(fSize == 0)
	{
		AlertErrorAndExit("Empty config file.");
	}
	rewind(f);

	int passed = 1;
	char *buffer = (char*)malloc(fSize);
	ZeroMemory(buffer, fSize);
	int result = fread(buffer, sizeof(char), fSize, f);
	if(!result)
	{
		AlertErrorAndExit("Could not read config file.");
	}
	// Retrieve each setting
	passed = passed && GetAndSetSetting(buffer, CONFIG_KEY_MediaApplicationPath, mediaApplicationPath);

	// Free memory
	free(buffer);
	fclose(f);

	return passed;
}

BOOL GetAndSetSetting(char* settings, TCHAR* key, TCHAR* valueStr)
{
	char* value = strstr(settings, key);
	if(!value)
	{
		char format[] = "Could not find config key: %s";
		char message[strlen(format) + strlen(key)];
		sprintf(message, format, key);
		AlertErrorAndExit(message);
	}
	value += strlen(key);
	while(*value == ' ' || *value == '=')
		++value;

	int bytesWritten = 0;
	while(*value != '\n' && *value != '\r' && *value != '\0' && bytesWritten < MAX_CLASS_NAME-1)
	{
		valueStr[bytesWritten++] = *(value++);
	}
//	printf("setting: %s \nvalue: %s\n", key, valueStr);

	if(strlen(valueStr) ==0)
	{
		char format[] = "No value found for config key: %s";
		char message[strlen(format) + strlen(key)];
		sprintf(message, format, key);
		AlertErrorAndExit(message);
	}
	return 1;
}

void AlertErrorAndExit(char* errorMsg)
{
	char format[] = "%s\nConfig path: %s";
	int size = strlen(errorMsg) + strlen(format) + MAX_PATH;
	char message[size];
	sprintf(message, format, errorMsg, CONFIG_FILE_PATH);
	MessageBox(0, message, "ATKMediaInterceptor", MB_ICONERROR);
	exit(0);
}
