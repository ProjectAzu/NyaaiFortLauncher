#include "Log.h"

#include <iostream>
#include <Windows.h>

void LogImplementation(const std::string& Message)
{
	std::cout << Message + '\n';
	OutputDebugStringA(Message.c_str());
}

void LogImplementation(const std::wstring& Message)
{
	std::wcout << Message + L'\n';
	OutputDebugStringW(Message.c_str());
}
