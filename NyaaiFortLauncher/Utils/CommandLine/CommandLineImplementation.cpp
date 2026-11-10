module CommandLine;

void NCommandLine::Log(const std::wstring& Message)
{
}

bool NCommandLine::ShouldProgramExit() const
{
    return false;
}

void NCommandLine::EnqueueCommand(const std::wstring& Command)
{
    std::scoped_lock Lock{ QueueMutex };
    CommandQueue.push(Command);
}

std::optional<std::wstring> NCommandLine::GetPendingCommand()
{
    std::scoped_lock Lock{ QueueMutex };

    if (CommandQueue.empty())
    {
        return std::nullopt;
    }

    auto Result = std::move(CommandQueue.front());
    CommandQueue.pop();

    return Result;
}

void InitCommandLine(const TObjectTemplate<NCommandLine>& CommandLineTemplate)
{
    InitCommandLineBackend(CommandLineTemplate.NewObjectRaw());
}
