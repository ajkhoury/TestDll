#pragma once

#include "nt_ddk.h"

namespace Utils
{

	static HMODULE GetLocalModuleHandle(const char* moduleName)
	{
		void* dwModuleHandle = 0;

		_TEB* teb = (_TEB*)NtCurrentTeb();
		_PEB* peb = (_PEB*)teb->ProcessEnvironmentBlock;
		PPEB_LDR_DATA ldrData = peb->Ldr;
		PLDR_DATA_ENTRY cursor = (PLDR_DATA_ENTRY)ldrData->InInitializationOrderModuleList.Flink;

		while (cursor->BaseAddress)
		{
			char strBaseDllName[MAX_PATH] = { 0 };
			size_t bytesCopied = 0;
			wcstombs_s(&bytesCopied, strBaseDllName, cursor->BaseDllName.Buffer, MAX_PATH);
			if (_stricmp(strBaseDllName, moduleName) == 0) {
				dwModuleHandle = cursor->BaseAddress;
				break;
			}
			cursor = (PLDR_DATA_ENTRY)cursor->InMemoryOrderModuleList.Flink;
		}
		return (HMODULE)dwModuleHandle;
	}

	static void* GetProcAddress(HMODULE module, const char *proc_name)
	{
		char *modb = (char *)module;

		IMAGE_DOS_HEADER *dos_header = (IMAGE_DOS_HEADER *)modb;
		IMAGE_NT_HEADERS *nt_headers = (IMAGE_NT_HEADERS *)((size_t)modb + dos_header->e_lfanew);

		IMAGE_OPTIONAL_HEADER *opt_header = &nt_headers->OptionalHeader;
		IMAGE_DATA_DIRECTORY *exp_entry = (IMAGE_DATA_DIRECTORY *)(&opt_header->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]);
		IMAGE_EXPORT_DIRECTORY *exp_dir = (IMAGE_EXPORT_DIRECTORY *)((size_t)modb + exp_entry->VirtualAddress);

		DWORD* func_table = (DWORD*)((size_t)modb + exp_dir->AddressOfFunctions);
		WORD* ord_table = (WORD *)((size_t)modb + exp_dir->AddressOfNameOrdinals);
		DWORD* name_table = (DWORD*)((size_t)modb + exp_dir->AddressOfNames);

		void *address = NULL;
		DWORD i;

		/* is ordinal? */
		if (((ULONG_PTR)proc_name >> 16) == 0)
		{
			WORD ordinal = LOWORD(proc_name);
			ULONG_PTR ord_base = exp_dir->Base;
			/* is valid ordinal? */
			if (ordinal < ord_base || ordinal > ord_base + exp_dir->NumberOfFunctions)
				return NULL;

			/* taking ordinal base into consideration */
			address = (void*)((size_t)modb + func_table[ordinal - ord_base]);
		}
		else
		{
			/* import by name */
			for (i = 0; i < exp_dir->NumberOfNames; i++)
			{
				/* name table pointers are rvas */
				char* procEntryName = (char*)((size_t)modb + name_table[i]);
				if (_stricmp(proc_name, procEntryName) == 0)
				{
					address = (void*)((size_t)modb + func_table[ord_table[i]]);
					break;
				}
			}
		}
		/* is forwarded? */
		if ((char *)address >= (char*)exp_dir && (char*)address < (char*)exp_dir + exp_entry->Size)
		{
			HMODULE frwd_module = 0;

			char* dll_name = _strdup((char*)address);
			if (!dll_name)
				return NULL;
			char* func_name = strchr(dll_name, '.');
			*func_name++ = 0;

			address = NULL;

			char dllName[256];
			strcpy_s(dllName, dll_name);
			strcat_s(dllName, strlen(dll_name) + 4 + 1, ".dll");

			/* is already loaded? */
			frwd_module = (HMODULE)Utils::GetLocalModuleHandle(dllName);
			if (!frwd_module)
				frwd_module = LoadLibraryA(dllName);
			if (!frwd_module)
			{
				MessageBox(0, L"GetProcAddress failed to load module using GetModuleHandle and LoadLibrary!", L"client", MB_ICONERROR);
				return NULL;
			}

			bool forwardByOrd = strchr(func_name, '#') == 0 ? false : true;
			if (forwardByOrd) // forwarded by ordinal
			{
				WORD func_ord = atoi(func_name + 1);
				address = Utils::GetProcAddress(frwd_module, (const char*)func_ord);
			}
			else
			{
				address = Utils::GetProcAddress(frwd_module, func_name);
			}

			free(dll_name);
		}
		return address;
	}

	static HANDLE NtCreateThreadEx(HANDLE hProcess, LPVOID lpRemoteThreadStart, LPVOID lpParam, DWORD* threadId)
	{
		tNtCreateThreadEx NtCreateThreadEx = (tNtCreateThreadEx)Utils::GetProcAddress(Utils::GetLocalModuleHandle("ntdll.dll"), "NtCreateThreadEx");
		if (NtCreateThreadEx == NULL)
			return NULL;

		PS_ATTRIBUTE_LIST attrList;
		ZeroMemory(&attrList, sizeof(PS_ATTRIBUTE_LIST));
		CLIENT_ID cid;
		ZeroMemory(&cid, sizeof(CLIENT_ID));
		OBJECT_ATTRIBUTES64 thrdAttr;
		InitializeObjectAttributes64(&thrdAttr, NULL, 0, NULL, NULL);

		attrList.Attributes[0].Attribute = PsAttributeValue(PsAttributeClientId, TRUE, FALSE, FALSE);
		attrList.Attributes[0].Size = sizeof(CLIENT_ID);
		attrList.Attributes[0].ValuePtr = (ULONG_PTR*)&cid;

		attrList.TotalLength = sizeof(PS_ATTRIBUTE_LIST);

		HANDLE hRemoteThread = NULL;
		HRESULT hRes = 0;

		if (!NT_SUCCESS(NtCreateThreadEx(&hRemoteThread, THREAD_ALL_ACCESS, NULL, hProcess, lpRemoteThreadStart, lpParam, 0, 0, 0x1000, 0x100000, &attrList)))
			return NULL;

		if (threadId)
			*threadId = (DWORD)cid.UniqueThread;

		return hRemoteThread;
	}

}