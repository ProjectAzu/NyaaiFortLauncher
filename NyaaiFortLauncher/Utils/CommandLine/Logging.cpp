module Logging;

static FCommandLineBackend* CommandLine = nullptr;
static std::mutex CommandLineMutex{};

void InitCommandLineBackend(FCommandLineBackend* Backend)
{
    std::scoped_lock Lock{ CommandLineMutex };
    CommandLine = Backend;
}

void CleanupCommandLine()
{
    FCommandLineBackend* Previous = nullptr;
    {
        std::scoped_lock Lock{ CommandLineMutex };
        Previous = std::exchange(CommandLine, nullptr);
    }

    if (Previous)
    {
        Previous->DestroyBackend();
    }
}

static std::vector<std::wstring> PositionalCommandLineArgs{};
static std::vector<std::pair<std::wstring, std::wstring>> CommandLineArgs{};

void InitCommandLineArgs(int32 ArgsNum, wchar_t* ArgsArrayPtr[])
{
    bool bIsExpectingValue = false;

    for (int32 i = 1; i < ArgsNum; i++)
    {
        std::wstring Arg{ ArgsArrayPtr[i] };

        if (Arg.empty())
        {
            continue;
        }

        if (Arg[0] == L'-')
        {
            bIsExpectingValue = true;
            CommandLineArgs.emplace_back().first = Arg;

            continue;
        }

        if (bIsExpectingValue)
        {
            bIsExpectingValue = false;
            CommandLineArgs.back().second = Arg;
        }
        else
        {
            PositionalCommandLineArgs.push_back(Arg);
        }
    }
}

bool HasCommandLineArg(const wchar_t* Arg)
{
    for (const auto& Elem : CommandLineArgs)
    {
        if (Elem.first == Arg)
        {
            return true;
        }
    }

    return false;
}

std::optional<std::wstring> GetCommandLineArgValue(const wchar_t* Arg)
{
    for (const auto& Elem : CommandLineArgs)
    {
        if (Elem.first != Arg)
        {
            continue;
        }

        if (Elem.second.empty())
        {
            Log(Error, L"Command line arg '{}' requires a value to be specified", Arg);

            return std::nullopt;
        }

        return Elem.second;
    }

    return std::nullopt;
}

std::optional<std::wstring> GetCommandLinePositionalArg(uint32 Index)
{
    if (Index < scast<uint32>(PositionalCommandLineArgs.size()))
    {
        return PositionalCommandLineArgs[Index];
    }

    return std::nullopt;
}

void LogRaw(const std::wstring& Message)
{
    std::scoped_lock Lock{ CommandLineMutex };

    if (!CommandLine)
    {
        return;
    }

    CommandLine->Log(Message);
}

void LogImplementation(ELogLevel Level, const std::wstring& Message)
{
    const std::wstring Decorated = std::format(
        L"[NyaaiFortLauncher] {}{}{}: {}\n",
        LogLevelToColorCode(Level),
        LogLevelToString(Level),
        L"\x1B[0m",
        Message);

    LogRaw(Decorated);
}

void EnqueueCommand(const std::wstring& Command)
{
    std::scoped_lock Lock{ CommandLineMutex };

    if (!CommandLine)
    {
        return;
    }

    CommandLine->EnqueueCommand(Command);
}

std::optional<std::wstring> GetPendingCommand()
{
    std::scoped_lock Lock{ CommandLineMutex };

    if (!CommandLine)
    {
        return std::nullopt;
    }

    return CommandLine->GetPendingCommand();
}

bool IsCommandLineRequestingProgramExit()
{
    std::scoped_lock Lock{ CommandLineMutex };

    if (!CommandLine)
    {
        return false;
    }

    return CommandLine->ShouldProgramExit();
}
