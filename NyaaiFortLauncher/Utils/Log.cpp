#include "Log.h"

#include "Utils/WindowsInclude.h"
#include "Utils/Utf8.h"

#include "replxx/replxx.hxx"

#include <atomic>
#include <condition_variable>
#include <format>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <iostream>

namespace
{
	void EnableVirtualTerminalProcessing()
	{
		HANDLE Out = GetStdHandle(STD_OUTPUT_HANDLE);
		DWORD  Mode = 0;
		if (GetConsoleMode(Out, &Mode))
		{
			SetConsoleMode(Out, Mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
		}
	}

	// replxx expects UTF-8. We'll keep your public API as std::wstring and convert at the boundary.
	std::unique_ptr<replxx::Replxx> GReplxx;

	std::mutex                 GCommandMutex;
	std::queue<std::wstring>   GPendingCommands;
	
	std::mutex GPendingPrintMutex{};
	std::wstring GPendingPrint{};
	std::jthread GPrintingThread{};

	std::jthread               GInputThread;
	std::atomic<bool>          bInitialized = false;

	constexpr char Prompt[] = "> ";

	void InputWorker(std::stop_token StopToken)
	{
		// Typical replxx usage: retry on EAGAIN.
		while (!StopToken.stop_requested())
		{
			errno = 0;
			char const* Buf = nullptr;

			do
			{
				Buf = GReplxx ? GReplxx->input(Prompt) : nullptr;
			} while ((Buf == nullptr) && (errno == EAGAIN) && !StopToken.stop_requested());

			if (StopToken.stop_requested())
			{
				break;
			}

			// nullptr means EOF / abort / etc (depending on key bindings / platform).
			if (Buf == nullptr)
			{
				break;
			}

			std::string LineUtf8(Buf);

			// Add to replxx history (Up/Down arrow navigation).
			GReplxx->history_add(LineUtf8);

			EnqueueConsoleCommand(Utf8ToWide(LineUtf8));
		}
	}
	
	void FlushPendingPrint()
	{
		std::scoped_lock Lock{GPendingPrintMutex};
		
		if (GPendingPrint.empty())
		{
			return;
		}
			
		if (GReplxx)
		{
			const std::string Utf8 = WideToUtf8(GPendingPrint);
			GReplxx->print("%s", Utf8.c_str());
		}
		else
		{
			// Fallback if logging is used before InitializeLogging().
			std::wcout << GPendingPrint;	
		}
			
		GPendingPrint.clear();
	}
	
	// this logging lib goes to shit when you print too frequently and a different lib doesnt exist
	void PrintingWorker(std::stop_token StopToken)
	{
		while (!StopToken.stop_requested())
		{
			FlushPendingPrint();
			
			Sleep(25);
		}
	}
}

std::optional<std::wstring> GetPendingConsoleCommand()
{
	std::scoped_lock Lock(GCommandMutex);
	if (GPendingCommands.empty())
	{
		return std::nullopt;
	}

	std::wstring Cmd = std::move(GPendingCommands.front());
	GPendingCommands.pop();
	return Cmd;
}

void InitializeLogging()
{
	EnableVirtualTerminalProcessing();

	// Keep console IO consistent with replxx's UTF-8 expectations.
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);

	GReplxx = std::make_unique<replxx::Replxx>();

	// Make sure Ctrl+D sends EOF (so we can also emulate it during shutdown).
	// Example of binding ctrl-D to "send_eof" in a real replxx app.
	GReplxx->bind_key_internal(replxx::Replxx::KEY::control('D'), "send_eof");

	bInitialized = true;

	// Start input thread (reads user commands continuously).
	GInputThread = std::jthread(InputWorker);
	GPrintingThread = std::jthread(PrintingWorker);
}

void CleanupLogging()
{
	FlushPendingPrint();
	
	if (!bInitialized.exchange(false))
	{
		return;
	}

	// Ask the input thread to stop, then poke replxx so a blocking input() returns.
	if (GInputThread.joinable())
	{
		GInputThread.request_stop();

		if (GReplxx)
		{
			// emulate_key_press is used to break replxx input from another thread.
			GReplxx->emulate_key_press(replxx::Replxx::KEY::control('D'));
		}

		GInputThread.join();
	}
	
	if (GPrintingThread.joinable())
	{
		GPrintingThread.request_stop();
		GPrintingThread.join();
	}

	GReplxx.reset();
}

void LogImplementation(ELogLevel Level, const std::wstring& Msg)
{
	const std::wstring Decorated = std::format(
		L"[NyaaiFortLauncher] {}{}{}: {}\n",
		LogLevelToColorCode(Level), LogLevelToString(Level), L"\x1B[0m",
		Msg);

	LogRaw(Decorated);
}

void EnqueueConsoleCommand(const std::wstring& Command)
{
	std::scoped_lock Lock(GCommandMutex);
	GPendingCommands.push(Command);
}

void LogRaw(const std::wstring& Msg)
{
	if (bInitialized)
	{
		std::scoped_lock Lock{GPendingPrintMutex};
		GPendingPrint += Msg;
		return;
	}

	// Fallback if logging is used before InitializeLogging().
	std::wcout << Msg;
}
