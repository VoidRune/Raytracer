#include "logger.h"
#include <iostream>

#ifdef _DEBUG
#define DEBUG_LOG
#include <windows.h>
#endif

#define MAX_PRINT_LENGTH 1024

#define PRINTCOLOR		0x000B
#define PRINTINFOCOLOR	0x000A
#define PRINTWARNCOLOR	0x000D
#define PRINTERRORCOLOR 0x000C
#define PRINTFATALCOLOR 0x00C0

namespace Logger
{
#ifdef DEBUG_LOG
	HANDLE g_ConsoleHandle;
#endif
	void Init()
	{
#ifdef DEBUG_LOG
		AllocConsole();
		SetWindowPos(GetConsoleWindow(), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
		AttachConsole(GetCurrentProcessId());
		freopen_s((FILE**)(stdout), "CON", "w", stdout);
		g_ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(g_ConsoleHandle, 0x000B);
#endif
	}

	void Print(const char* str, ...)
	{
#ifdef DEBUG_LOG
		SetConsoleTextAttribute(g_ConsoleHandle, PRINTCOLOR);
		va_list argList;
		va_start(argList, str);

		char buffer[MAX_PRINT_LENGTH];
		vsnprintf(buffer, MAX_PRINT_LENGTH, str, argList);
		printf("%s\n", buffer);

		va_end(argList);
#endif
	}

	void PrintInfo(const char* str, ...)
	{
#ifdef DEBUG_LOG
		SetConsoleTextAttribute(g_ConsoleHandle, PRINTINFOCOLOR);
		va_list argList;
		va_start(argList, str);

		char buffer[MAX_PRINT_LENGTH];
		vsnprintf(buffer, MAX_PRINT_LENGTH, str, argList);
		printf("[INFO] %s\n", buffer);

		va_end(argList);
#endif
	}

	void PrintWarn(const char* str, ...)
	{
#ifdef DEBUG_LOG
		SetConsoleTextAttribute(g_ConsoleHandle, PRINTWARNCOLOR);
		va_list argList;
		va_start(argList, str);

		char buffer[MAX_PRINT_LENGTH];
		vsnprintf(buffer, MAX_PRINT_LENGTH, str, argList);
		printf("[WARNING!] %s\n", buffer);

		va_end(argList);
#endif
	}

	void PrintError(const char* str, ...)
	{
#ifdef DEBUG_LOG
		SetConsoleTextAttribute(g_ConsoleHandle, PRINTERRORCOLOR);
		va_list argList;
		va_start(argList, str);

		char buffer[MAX_PRINT_LENGTH];
		vsnprintf(buffer, MAX_PRINT_LENGTH, str, argList);
		printf("[ERROR!] %s\n", buffer);

		va_end(argList);
#endif
	}

	void PrintFatal(const char* str, ...)
	{
#ifdef DEBUG_LOG
		SetConsoleTextAttribute(g_ConsoleHandle, PRINTFATALCOLOR);
		va_list argList;
		va_start(argList, str);

		char buffer[MAX_PRINT_LENGTH];
		vsnprintf(buffer, MAX_PRINT_LENGTH, str, argList);
		printf("[FATAL!] %s\n", buffer);

		va_end(argList);
#endif
	}

	void PrintIf(bool condition, const char* str, ...)
	{
#ifdef DEBUG_LOG
		if (condition)
		{
			SetConsoleTextAttribute(g_ConsoleHandle, PRINTCOLOR);
			va_list argList;
			va_start(argList, str);

			char buffer[MAX_PRINT_LENGTH];
			vsnprintf(buffer, MAX_PRINT_LENGTH, str, argList);
			printf("%s\n", buffer);

			va_end(argList);
		}
#endif
	}

	void PrintInfoIf(bool condition, const char* str, ...)
	{
#ifdef DEBUG_LOG
		if (condition)
		{
			SetConsoleTextAttribute(g_ConsoleHandle, PRINTINFOCOLOR);
			va_list argList;
			va_start(argList, str);

			char buffer[MAX_PRINT_LENGTH];
			vsnprintf(buffer, MAX_PRINT_LENGTH, str, argList);
			printf("[INFO] %s\n", buffer);

			va_end(argList);
		}
#endif
	}

	void PrintWarnIf(bool condition, const char* str, ...)
	{
#ifdef DEBUG_LOG
		if (condition)
		{
			SetConsoleTextAttribute(g_ConsoleHandle, PRINTWARNCOLOR);
			va_list argList;
			va_start(argList, str);

			char buffer[MAX_PRINT_LENGTH];
			vsnprintf(buffer, MAX_PRINT_LENGTH, str, argList);
			printf("[WARNING!] %s\n", buffer);

			va_end(argList);
		}
#endif
	}

	void PrintErrorIf(bool condition, const char* str, ...)
	{
#ifdef DEBUG_LOG
		if (condition)
		{
			SetConsoleTextAttribute(g_ConsoleHandle, PRINTERRORCOLOR);
			va_list argList;
			va_start(argList, str);

			char buffer[MAX_PRINT_LENGTH];
			vsnprintf(buffer, MAX_PRINT_LENGTH, str, argList);
			printf("[ERROR!] %s\n", buffer);

			va_end(argList);
		}
#endif
	}

	void PrintFatalIf(bool condition, const char* str, ...)
	{
#ifdef DEBUG_LOG
		if (condition)
		{
			SetConsoleTextAttribute(g_ConsoleHandle, PRINTFATALCOLOR);
			va_list argList;
			va_start(argList, str);

			char buffer[MAX_PRINT_LENGTH];
			vsnprintf(buffer, MAX_PRINT_LENGTH, str, argList);
			printf("[FATAL!] %s\n", buffer);

			va_end(argList);
		}
#endif
	}

}