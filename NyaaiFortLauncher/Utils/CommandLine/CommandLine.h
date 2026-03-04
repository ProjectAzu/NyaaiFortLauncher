#pragma once

#include "Object/IntegerTypes.h"

#include <optional>
#include <format>
#include <string>

class NCommandLine;

enum ELogLevel : uint8
{
    Temp,
    Info,
    Warning,
    Error
};

constexpr const wchar_t* LogLevelToString(ELogLevel Level)
{
    switch (Level)
    {
    case Temp: return L"Temp";
    case Info: return L"Info";  
    case Warning: return L"Warning";
    case Error: return L"Error";
    default: return L"Unknown";
    }
}

constexpr const wchar_t* LogLevelToColorCode(ELogLevel Level)
{
    switch (Level)
    {
    case Temp: return L"\x1B[34m";
    case Info: return L"\x1B[0m";  
    case Warning: return L"\x1B[33m";
    case Error: return L"\x1B[31m";
    default: return L"\x1B[0m";
    }
}

void InitCommandLineArgs(int32 ArgsNum, wchar_t* ArgsArrayPtr[]);
bool HasCommandLineArg(const wchar_t* Arg);
std::optional<std::wstring> GetCommandLineArgValue(const wchar_t* Arg);
std::optional<std::wstring> GetCommandLinePositionalArg(uint32 Index);

void LogRaw(const std::wstring& Message);
void LogImplementation(ELogLevel Level, const std::wstring& Message);
void EnqueueCommand(const std::wstring& Command);
std::optional<std::wstring> GetPendingCommand();
bool IsCommandLineRequestingProgramExit();

template<typename... Args>
inline void Log(ELogLevel Level, const wchar_t* Format, Args&&... Arguments)
{
    LogImplementation(Level, std::vformat(Format, std::make_wformat_args(Arguments...)));
}
