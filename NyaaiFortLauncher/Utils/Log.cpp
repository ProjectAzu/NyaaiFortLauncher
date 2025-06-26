// Log.cpp  —  engine-wide logging + interactive command line (replxx edition)

#include "Log.h"

#include <replxx/replxx.hxx>                     // ▸ vcpkg install replxx
#include <Windows.h>
#include <format>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>

// ugly chat-gpt generated code

/* ───────────────────────────── helpers ───────────────────────────── */

namespace
{
    /* ------------- shared state ------------- */
    replxx::Replxx              Rx;               // single replxx instance
    std::queue<std::string>     InputQueue;       // commands ready for engine
    std::mutex                  InputMutex;       // protects InputQueue
    std::mutex                  ConsoleMutex;     // serialises LogRaw / Rx.print

    std::unique_ptr<std::jthread> ReaderThread;   // background input loop

    /* console mode that shows colours inside replxx-managed prompt */
    void EnableVirtualTerminalProcessing()
    {
        HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD  mode = 0;
        if (GetConsoleMode(out, &mode))
            SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);  // :contentReference[oaicite:0]{index=0}
    }

    /* wchar->UTF-8 (for printing wide engine logs through replxx) */
    std::string NarrowUTF8(std::wstring_view w)
    {
        if (w.empty()) return {};
        int bytes = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()),
                                        nullptr, 0, nullptr, nullptr);
        std::string out(bytes, '\0');
        WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()),
                            out.data(), bytes, nullptr, nullptr);
        return out;
    }

    /* safe printing that never mangles the user’s currently edited line */
    void ConsoleWrite(std::wstring_view msg)
    {
        std::string utf8 = NarrowUTF8(msg);
        std::lock_guard lk(ConsoleMutex);          // serialise with other writers
        Rx.print("%s", utf8.c_str());              // replxx keeps the prompt intact :contentReference[oaicite:1]{index=1}
    }

    /* ------------------ background stdin reader ------------------ */
    void InputLoop(std::stop_token st)
    {
        while (!st.stop_requested())
        {
            /* blocking call – replxx handles editing, history, UTF-8, etc. */
            const char* line = Rx.input("> ");
            if (!line)                            // nullptr on Ctrl-D / Ctrl-C
                if (st.stop_requested()) break;
                else continue;

            std::string cmd{line};
            if (cmd.empty())                      // empty line → ignore
                continue;

            Rx.history_add(cmd);                  // in-memory; saved on shutdown

            {
                std::lock_guard lk(InputMutex);
                InputQueue.push(std::move(cmd));
            }
        }
    }
} // anon ns

/* ───────────────────────── public API ───────────────────────── */

std::optional<std::string> GetPendingConsoleCommand()
{
    std::lock_guard lk(InputMutex);
    if (InputQueue.empty())
        return std::nullopt;

    std::string cmd = std::move(InputQueue.front());
    InputQueue.pop();
    return cmd;
}

void InitializeLogging()
{
    EnableVirtualTerminalProcessing();

    /* optional: load persisted history */
    //Rx.history_load(".nyaai_engine_history");      // silently ignored if file absent

    /* launch reader thread (replxx blocks inside) */
    ReaderThread = std::make_unique<std::jthread>(InputLoop);
}

void CleanupLogging()
{
    if (ReaderThread && ReaderThread->joinable())
    {
        /* wake the blocking input() so the loop can observe stop_token */
        Rx.emulate_key_press(replxx::Replxx::KEY::control('D'));      // synthetic Ctrl-D :contentReference[oaicite:2]{index=2}
        ReaderThread->request_stop();
        ReaderThread->join();
        ReaderThread.reset();
    }

    //Rx.history_save(".nyaai_engine_history");      // persist across runs
    /* Rx destructor restores console modes automatically */
}

void LogImplementation(ELogLevel Level, const std::wstring& Msg)
{
    const std::wstring decorated = std::format(
        L"[NyaaiFortLauncher] {}{}{}: {}\n",
        LogLevelToColorCode(Level), LogLevelToString(Level), L"\x1B[0m",
        Msg);

    LogRaw(decorated);
}

void LogRaw(const std::wstring& Msg)
{
    ConsoleWrite(Msg);
}
