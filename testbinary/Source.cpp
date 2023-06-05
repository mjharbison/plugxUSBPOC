#include <windows.h>
#include <stdio.h>

int wmain(int argc, wchar_t* argv[])
{
	STARTUPINFOW info = { sizeof(info) };
	PROCESS_INFORMATION  processInfo;
	wchar_t cmdLineArgs[MAX_PATH] = L"";
	WCHAR *CmdLineArgs;
	WCHAR ModName[MAX_PATH];
	wchar_t* DriveLetter;
	//__debugbreak();
	CmdLineArgs = GetCommandLine();
	
	//printf("[+] Command line args are %ls\n", CmdLineArgs);
	DWORD LenHandle = GetModuleFileName(NULL, ModName, MAX_PATH);
	wcstok_s((wchar_t*)ModName, L":", &DriveLetter); //We need just the drive letter of the USB device
	wsprintfW(cmdLineArgs, L"Explorer.exe \"%ls:\\%c\\%c\"", ModName,160,160);
	if (!CreateProcess(NULL, cmdLineArgs, NULL, NULL, FALSE, 0, NULL, NULL, &info, &processInfo))
	{
		printf("[!] Error starting explorer.exe. Error code %08x\n", GetLastError());
	}
	system("calc.exe");
}