export module Logging;

export import CoreTypes;

export enum ELogLevel : uint8
{
    Temp,
    Info,
    Warning,
    Error
};

export constexpr const wchar_t* LogLevelToString(ELogLevel Level)
{
    switch (Level)
    {
    case Temp:
        return L"Temp";
    case Info:
        return L"Info";
    case Warning:
        return L"Warning";
    case Error:
        return L"Error";
    default:
        return L"Unknown";
    }
}

export constexpr const wchar_t* LogLevelToColorCode(ELogLevel Level)
{
    switch (Level)
    {
    case Temp:
        return L"\x1B[34m";
    case Info:
        return L"\x1B[0m";
    case Warning:
        return L"\x1B[33m";
    case Error:
        return L"\x1B[31m";
    default:
        return L"\x1B[0m";
    }
}

export void InitCommandLineArgs(int32 ArgsNum, wchar_t* ArgsArrayPtr[]);
export bool HasCommandLineArg(const wchar_t* Arg);
export std::optional<std::wstring> GetCommandLineArgValue(const wchar_t* Arg);
export std::optional<std::wstring> GetCommandLinePositionalArg(uint32 Index);

export void LogRaw(const std::wstring& Message);
export void LogImplementation(ELogLevel Level, const std::wstring& Message);
export void EnqueueCommand(const std::wstring& Command);
export std::optional<std::wstring> GetPendingCommand();
export bool IsCommandLineRequestingProgramExit();

export template <typename... Args>
inline void Log(ELogLevel Level, const wchar_t* Format, Args&&... Arguments)
{
    LogImplementation(Level, std::vformat(Format, std::make_wformat_args(Arguments...)));
}

export class FCommandLineBackend
{
public:
    virtual ~FCommandLineBackend() = default;
    virtual void Log(const std::wstring& Message) = 0;
    virtual bool ShouldProgramExit() const = 0;
    virtual void EnqueueCommand(const std::wstring& Command) = 0;
    virtual std::optional<std::wstring> GetPendingCommand() = 0;
    virtual void DestroyBackend() = 0;
};

export void InitCommandLineBackend(FCommandLineBackend* Backend);
export void CleanupCommandLine();
