#include <mutex>
#include <utility>

#include "CommandLineImplementation.h"

static std::vector<std::wstring> ProgramLaunchArgs{};

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
	{
		std::scoped_lock lock{CommandLineMutex};

		if (!CommandLine)
		{
			CommandLine = CommandLineTemplate.NewObjectRaw();
			return;
		}
	}

	Log(Error, L"Cannot init command line as it is already init");
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

static std::vector<std::wstring> PositionalCommandLineArgs{};
static std::vector<std::pair<std::wstring, std::wstring>> CommandLineArgs{};

void InitCommandLineArgs(int32 ArgsNum, wchar_t* ArgsArrayPtr[])
{
	bool bIsExpectingValue = false;
	
	for (int32 i = 1; i < ArgsNum; i++)
	{
		std::wstring Arg{ArgsArrayPtr[i]};
		
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
	if (Index < static_cast<uint32>(PositionalCommandLineArgs.size()))
	{
		return PositionalCommandLineArgs[Index];
	}
	
	return std::nullopt;
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

bool IsCommandLineRequestingProgramExit()
{
	std::scoped_lock Lock{CommandLineMutex};
	
	if (!CommandLine)
	{
		return false;
	}
	
	return CommandLine->ShouldProgramExit();
}