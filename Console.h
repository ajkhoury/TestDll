#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include <stdio.h>
#include <io.h>

class Console
{
public:
	Console();
	Console(const char* name);

	void Create();
	void Release();

private:
	char m_szName[256];
	FILE m_OldStdin, m_OldStdout;
	bool m_OwnConsole;
};

#endif