#include <Windows.h>
#include <iostream>

#include "Utils.h"
#include "Console.h"

Console console;

DWORD WINAPI Init(LPVOID lpArguments)
{
	while (1)
	{
		printf("Hackidy hack!\n");
		Sleep(300);
	}	
	
	return 0;
}

int __stdcall DllMain(HINSTANCE hinstDLL, unsigned long fdwReason, void* lpReserved)
{
	static HANDLE hMainThread = INVALID_HANDLE_VALUE;
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		console.Create();

		Sleep(1000);

		DWORD threadId = 0;
		hMainThread = Utils::NtCreateThreadEx(GetCurrentProcess(), Init, NULL, &threadId);

		//hMainThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Init, NULL, 0, NULL);
		printf("hMainThread: 0x%X\n", hMainThread);
		printf("threadId: 0x%X\n", threadId);

		return (hMainThread != INVALID_HANDLE_VALUE);
	}
	else if (fdwReason == DLL_PROCESS_DETACH)
	{
		console.Release();
	}
	return 0;
}
