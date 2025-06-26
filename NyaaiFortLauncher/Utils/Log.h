#pragma once

#include <format>
#include <string>
#include <optional>

#include "Object/IntegerTypes.h"

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

std::optional<std::string> GetPendingConsoleCommand();

void InitializeLogging();
void CleanupLogging();

void LogRaw(const std::wstring& Message);
inline void LogRaw(const std::string& Message)
{
    LogRaw(std::wstring(Message.begin(), Message.end()));
}

void LogImplementation(ELogLevel Level, const std::wstring& Message);

template<typename... Args>
inline void Log(ELogLevel Level, const char* Format, Args&&... Arguments)
{
    std::string MessageString(std::vformat(Format, std::make_format_args(Arguments...)));
    LogImplementation(Level, std::wstring(MessageString.begin(), MessageString.end()));
}

template<typename... Args>
inline void Log(ELogLevel Level, const wchar_t* Format, Args&&... Arguments)
{
    LogImplementation(Level, std::vformat(Format, std::make_wformat_args(Arguments...)));
}
