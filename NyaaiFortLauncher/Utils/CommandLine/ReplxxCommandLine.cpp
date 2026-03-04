#include "ReplxxCommandLine.h"

#include <mutex>
#include <thread>

#include <replxx.hxx>

#include "Utils/Utf8.h"
#include "Utils/WindowsInclude.h"

GENERATE_BASE_CPP(NReplxxCommandLine)

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

	std::jthread      GInputThread{};
	std::atomic<bool> bInitialized = false;
	std::atomic<bool> bProgramWantsToExit = false;

	constexpr char Prompt[] = "> ";

	// Internal key used only to bail out of input() during shutdown.
	constexpr char32_t ShutdownReplKey = replxx::Replxx::KEY::F11;

	replxx::Replxx::ACTION_RESULT ShutdownHandler(char32_t)
	{
		// Remove prompt + current input line so we don't leave "> " behind on exit.
		if (GReplxx)
		{
			GReplxx->invoke(replxx::Replxx::ACTION::CLEAR_SELF, 0);
		}
		return replxx::Replxx::ACTION_RESULT::BAIL;
	}
	
	replxx::Replxx::ACTION_RESULT CtrlCHandler(char32_t)
	{
		bProgramWantsToExit.exchange(true);
		
		return replxx::Replxx::ACTION_RESULT::CONTINUE;
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

			EnqueueCommand(Utf8ToWide(LineUtf8));
		}
	}
}

void NReplxxCommandLine::OnCreated()
{
    Super::OnCreated();
    
	EnableVirtualTerminalProcessing();
	
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);

	GReplxx = std::make_unique<replxx::Replxx>();
	GReplxx->install_window_change_handler();
	
	GReplxx->bind_key(ShutdownReplKey, &ShutdownHandler);
	
	// actually ctrl c is annoying so fuck this
	//GReplxx->bind_key(replxx::Replxx::KEY::control('C'), &CtrlCHandler);
	
	bInitialized = true;
	
	GInputThread = std::jthread(InputWorker);
}

void NReplxxCommandLine::OnDestroyed()
{
    Super::OnDestroyed();
	
	if (!bInitialized.exchange(false))
	{
		return;
	}

	if (GInputThread.joinable())
	{
		GInputThread.request_stop();
		
		if (GReplxx)
		{
			GReplxx->emulate_key_press(ShutdownReplKey);
		}

		GInputThread.join();
	}

	GReplxx.reset();
}

void NReplxxCommandLine::Log(const std::wstring& Message)
{
    Super::Log(Message);
	
	if (!GReplxx)
	{
		return;
	}
	
	const std::string Utf8 = WideToUtf8(Message);
	GReplxx->print("%s", Utf8.c_str());
}

bool NReplxxCommandLine::ShouldProgramExit() const
{
    return bProgramWantsToExit.load();
}
