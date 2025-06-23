#pragma once

#include <format>
#include <string>

#include "Object/IntegerTypes.h"

constexpr std::wstring ToWide(const char* input)
{
    std::wstring Result;

    for (std::size_t i = 0; input[i] != '\0'; ++i)
    {
        Result.push_back(static_cast<wchar_t>(static_cast<unsigned char>(input[i])));
    }

    return Result;
}

enum ELogLevel : uint8
{
    Temp,
    Info,
    Warning,
    Error
};

constexpr const char* LogLevelToString(ELogLevel Level)
{
    switch (Level)
    {
    case Temp: return "Temp";
    case Info: return "Info";  
    case Warning: return "Warning";
    case Error: return "Error";
    default: return "Unknown";
    }
}

constexpr const char* LogLevelToColorCode(ELogLevel Level)
{
    switch (Level)
    {
    case Temp: return "\x1B[34m";
    case Info: return "\x1B[0m";  
    case Warning: return "\x1B[33m";
    case Error: return "\x1B[31m";
    default: return "\x1B[0m";
    }
}

void LogImplementation(const std::string& Message);
void LogImplementation(const std::wstring& Message);

template<typename... Args>
__forceinline void Log(ELogLevel Level, const char* Format, Args&&... Arguments)
{
    const std::string Message = std::format(
        "[NyaaiFortLauncher] {}{}{}: {}",
        LogLevelToColorCode(Level), LogLevelToString(Level), "\x1B[0m",
        std::vformat(Format, std::make_format_args(Arguments...))
    );

    LogImplementation(Message);
}

template<typename... Args>
__forceinline void Log(ELogLevel Level, const wchar_t* Format, Args&&... Arguments)
{
    const std::wstring Message = std::format(
        L"[NyaaiFortLauncher] {}{}{}: {}",
        ToWide(LogLevelToColorCode(Level)), ToWide(LogLevelToString(Level)), L"\x1B[0m",
        std::vformat(Format, std::make_wformat_args(Arguments...))
    );

    LogImplementation(Message);
}
