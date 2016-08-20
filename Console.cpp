#include <Windows.h>
#include "console.h"

Console::Console() : m_OwnConsole(false)
{
	strcpy_s(m_szName, "dbg");
}

Console::Console(const char* name) : m_OwnConsole(false)
{
	strcpy_s(m_szName, name);
}

void Console::Create()
{
	if (!m_OwnConsole)
	{
		if (!AllocConsole())
			return;

		SetConsoleTitleA(m_szName);

		const int in = _open_osfhandle((int)GetStdHandle(STD_INPUT_HANDLE), 0x4000);
		const int out = _open_osfhandle((int)GetStdHandle(STD_OUTPUT_HANDLE), 0x4000);
		m_OldStdin = *stdin;
		m_OldStdout = *stdout;

		*stdin = *_fdopen(in, "r");
		*stdout = *_fdopen(out, "w");

		m_OwnConsole = true;
	}
}

void Console::Release()
{
	if (m_OwnConsole)
	{
		fclose(stdout);
		fclose(stdin);
		*stdout = m_OldStdout;
		*stdin = m_OldStdin;
		FreeConsole();
	}
}