#include <windows.h>
#include <stdio.h>

/**
 * 2011, zaak404@gmail.com
 */

#define MAX_COMMAND 2500

HINSTANCE hInst;
TCHAR szTitle[] = "ATKMEDIA";
TCHAR szWindowClass[] = "ATKMEDIA";
TCHAR CONFIG_FILE_NAME[] = "ATKMediaInterceptor.conf";
TCHAR CONFIG_FILE_PATH[MAX_PATH];
TCHAR CONFIG_KEY_GenericCommand[] = "generic_button_command";
TCHAR genericCommand[MAX_COMMAND];

ATOM MRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void SendKeyPress(WORD vkKeyCode);
void RunCommand(TCHAR* command);
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

#define ATKMEDIA_MESSAGE				  0x0917
#define ATKMEDIA_GENERIC                  0x0001
#define ATKMEDIA_PLAY                     0x0002
#define ATKMEDIA_STOP                     0x0003
#define ATKMEDIA_PREV                     0x0005
#define ATKMEDIA_NEXT                     0x0004

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId    = LOWORD(wParam);
	int wmEvent = HIWORD(wParam);

	if(message == WM_COMMAND && wmId == ATKMEDIA_MESSAGE)
	{
		switch(wmEvent)
		{
		case ATKMEDIA_GENERIC:
			RunCommand(genericCommand);
			return 0;
		case ATKMEDIA_PLAY:
			SendKeyPress(VK_MEDIA_PLAY_PAUSE);
			return 0;
		case ATKMEDIA_STOP:
			SendKeyPress(VK_MEDIA_STOP);
			return 0;
		case ATKMEDIA_NEXT:
			SendKeyPress(VK_MEDIA_NEXT_TRACK);
			return 0;
		case ATKMEDIA_PREV:
			SendKeyPress(VK_MEDIA_PREV_TRACK);
			return 0;
		}
	}
	// Let the event go so it can be handled by the default
	return DefWindowProc(hWnd, message, wParam, lParam);
}

// Based on https://batchloaf.wordpress.com/2012/04/17/simulating-a-keystroke-in-win32-c-or-c-using-sendinput/
void SendKeyPress(WORD vkKeyCode)
{
	// Create a keyboard event
	INPUT ip;
	ip.type = INPUT_KEYBOARD;
	ip.ki.time = 0;
	ip.ki.wScan = 0;
	ip.ki.dwExtraInfo = 0;
	ip.ki.wVk = vkKeyCode;

	// Send key down
	ip.ki.dwFlags = 0;
	SendInput(1, &ip, sizeof(INPUT));

	// Send key again but with keyup event
	ip.ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(1, &ip, sizeof(INPUT));
}

void RunCommand(TCHAR* command)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	ZeroMemory( &pi, sizeof(pi) );

	// Start the child process.
	if(!CreateProcess(
		NULL,           // Program to run
		command,        // Command to run
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		0,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory
		&si,            // Pointer to STARTUPINFO structure
		&pi )           // Pointer to PROCESS_INFORMATION structure
		)
	{
		char format[] = "CreateProcess failed with error: %d, for command: %s";
		char message[strlen(format) + 3 + strlen(command)];
		sprintf(message, format, GetLastError(), command);
		AlertErrorAndExit(message);
	}
}

BOOL LoadSettings()
{
	TCHAR modulePath[MAX_PATH];
	ZeroMemory(modulePath, MAX_PATH);
	ZeroMemory(CONFIG_FILE_PATH, MAX_PATH);
	ZeroMemory(genericCommand, MAX_COMMAND);

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
	passed = passed && GetAndSetSetting(buffer, CONFIG_KEY_GenericCommand, genericCommand);

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
	while(*value != '\n' && *value != '\r' && *value != '\0' && bytesWritten < MAX_COMMAND - 1)
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
