#include <mutex>

#include "CommandLineImplementation.h"

std::vector<std::wstring> ProgramLaunchArgs{};

static NCommandLine* CommandLine = nullptr;
static std::mutex CommandLineMutex{};

GENERATE_BASE_CPP(NCommandLine)

void NCommandLine::Log(const std::wstring& Message)
{
}

bool NCommandLine::ShouldProgramExit() const
{
	return false;
}

void NCommandLine::EnqueueCommand(const std::wstring& Command)
{
	CommandQueue.push(Command);
}

std::optional<std::wstring> NCommandLine::GetPendingCommand()
{
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
	std::scoped_lock Lock{CommandLineMutex};
	
	if (CommandLine)
	{
		Log(Error, L"Cannot init command line as it is already init");
		return;
	}
	
	CommandLine = CommandLineTemplate.NewObjectRaw();
}

void CleanupCommandLine()
{
	std::scoped_lock Lock{CommandLineMutex};
	
	if (CommandLine)
	{
		CommandLine->Destroy();
		CommandLine = nullptr;
	}
}

void LogRaw(const std::wstring& Message)
{
	std::scoped_lock Lock{CommandLineMutex};
	
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
		LogLevelToColorCode(Level), LogLevelToString(Level), L"\x1B[0m",
		Message
		);

	LogRaw(Decorated);
}

void EnqueueCommand(const std::wstring& Command)
{
	std::scoped_lock Lock{CommandLineMutex};
	
	if (!CommandLine)
	{
		return;
	}
	
	CommandLine->EnqueueCommand(Command);
}

std::optional<std::wstring> GetPendingCommand()
{
	std::scoped_lock Lock{CommandLineMutex};
	
	if (!CommandLine)
	{
		return std::nullopt;
	}
	
	return CommandLine->GetPendingCommand();
}

bool ShouldProgramExit()
{
	std::scoped_lock Lock{CommandLineMutex};
	
	if (!CommandLine)
	{
		return false;
	}
	
	return CommandLine->ShouldProgramExit();
}