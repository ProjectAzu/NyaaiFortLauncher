#include "CoutCommandLine.h"

#include <iostream>

GENERATE_BASE_CPP(NCoutCommandLine)

void NCoutCommandLine::Log(const std::wstring& Message)
{
    std::wcout << Message;
}
