// Log.cpp

#include "Log.h"

#include "Utils/WindowsInclude.h"
#include "Utils/Utf8.h"

#include "replxx/replxx.hxx"

#include <atomic>
#include <format>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
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

	std::unique_ptr<replxx::Replxx> GReplxx = nullptr;

	std::mutex               GCommandMutex{};
	std::queue<std::wstring> GPendingCommands{};

	std::jthread      GInputThread{};
	std::atomic<bool> bInitialized = false;

	constexpr char Prompt[] = "> ";

	// Internal key used only to bail out of input() during shutdown.
	constexpr char32_t ShutdownReplKey = replxx::Replxx::KEY::F11;

	void EnqueueConsoleCommand(std::wstring&& Command)
	{
		std::scoped_lock Lock(GCommandMutex);
		GPendingCommands.push(std::move(Command));
	}

	replxx::Replxx::ACTION_RESULT ShutdownHandler(char32_t)
	{
		// Remove prompt + current input line so we don't leave "> " behind on exit.
		if (GReplxx)
		{
			GReplxx->invoke(replxx::Replxx::ACTION::CLEAR_SELF, 0);
		}
		return replxx::Replxx::ACTION_RESULT::BAIL;
	}

	void InputWorker(std::stop_token StopToken)
	{
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

			// nullptr means EOF / abort / etc.
			if (Buf == nullptr)
			{
				break;
			}

			std::string LineUtf8(Buf);

			// Up/Down arrow history.
			if (GReplxx)
			{
				GReplxx->history_add(LineUtf8);
			}

			EnqueueConsoleCommand(Utf8ToWide(LineUtf8));
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

void EnqueueConsoleCommand(const std::wstring& Command)
{
	std::scoped_lock Lock(GCommandMutex);
	GPendingCommands.push(Command);
}

void InitializeLogging()
{
	EnableVirtualTerminalProcessing();

	// Keep console IO consistent with replxx's UTF-8 expectations.
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);

	GReplxx = std::make_unique<replxx::Replxx>();
	GReplxx->install_window_change_handler();

	// Internal shutdown key: always bails out of input() regardless of line state.
	GReplxx->bind_key(ShutdownReplKey, &ShutdownHandler);

	// Optional: keep Ctrl+D as EOF behavior if you want it.
	GReplxx->bind_key_internal(replxx::Replxx::KEY::control('D'), "send_eof");

	bInitialized = true;

	// Start input thread (reads user commands continuously).
	GInputThread = std::jthread(InputWorker);
}

void CleanupLogging()
{
	if (!bInitialized.exchange(false))
	{
		return;
	}

	if (GInputThread.joinable())
	{
		GInputThread.request_stop();

		// Wake replxx::input() and bail cleanly (does not require user input).
		if (GReplxx)
		{
			GReplxx->emulate_key_press(ShutdownReplKey);
		}

		GInputThread.join();
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

void LogRaw(const std::wstring& Msg)
{
	if (!bInitialized || !GReplxx)
	{
		// Fallback if logging is used before InitializeLogging() or after CleanupLogging().
		std::wcout << Msg;
		return;
	}

	// Correct replxx usage: print() from any thread.
	// With the fixed Windows event signaling, this should display immediately.
	const std::string Utf8 = WideToUtf8(Msg);
	GReplxx->print("%s", Utf8.c_str());
}