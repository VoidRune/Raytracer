#pragma once

namespace Logger
{
	void Init();
	void Print(const char* str, ...);
	void PrintInfo(const char* str, ...);
	void PrintWarn(const char* str, ...);
	void PrintError(const char* str, ...);
	void PrintFatal(const char* str, ...);
	void PrintIf(bool condition, const char* str, ...);
	void PrintInfoIf(bool condition, const char* str, ...);
	void PrintWarnIf(bool condition, const char* str, ...);
	void PrintErrorIf(bool condition, const char* str, ...);
	void PrintFatalIf(bool condition, const char* str, ...);
}