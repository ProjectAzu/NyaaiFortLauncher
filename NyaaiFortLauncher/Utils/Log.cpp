// Log.cpp engine-wide logging + interactive command line
#include "Log.h"

#include <Windows.h>
#include <format>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <fcntl.h>      // _O_U16TEXT
#include <io.h>         // _setmode

// This is chat gpt generated, ugly code but it works

/* ---------------------------- helpers ---------------------------- */

namespace
{
    // - console handles
    HANDLE StdIn()  { return GetStdHandle(STD_INPUT_HANDLE); }
    HANDLE StdOut() { return GetStdHandle(STD_OUTPUT_HANDLE); }

    // - globals protected by the mutexes below
    std::queue<std::wstring> InputQueue;   // committed commands
    std::wstring             CurrentLine;  // text being typed right now

    // - thread & synchronisation
    std::unique_ptr<std::jthread> ReaderThread;
    std::mutex                    InputMutex;      // InputQueue + CurrentLine
    std::mutex                    ConsoleMutex;    // all writes to stdout

    // - original console modes (restored in CleanupLogging)
    DWORD OldInMode  = 0;
    DWORD OldOutMode = 0;

    constexpr std::wstring_view Prompt = L"> ";

    /* wide -> UTF-8 */
    std::string NarrowUTF8(const std::wstring& w)
    {
        if (w.empty()) return {};
        const int bytes = WideCharToMultiByte(CP_UTF8, 0, w.data(),
                                              static_cast<int>(w.size()),
                                              nullptr, 0, nullptr, nullptr);
        std::string out(bytes, '\0');
        WideCharToMultiByte(CP_UTF8, 0, w.data(),
                            static_cast<int>(w.size()),
                            out.data(), bytes, nullptr, nullptr);
        return out;
    }

    /* print "prompt + CurrentLine" (optionally clearing the whole line first) */
    void RedrawInput(bool ClearLine)
    {
        std::lock_guard lock(ConsoleMutex);
        std::wcout.clear();                               // <- added
        if (ClearLine)
            std::wcout << L"\r\x1B[2K";
        std::wcout << L"\r" << Prompt << CurrentLine << std::flush;
    }

    /* write a log entry, preserving whatever the user is typing *
     * (called internally by LogRaw and also by the reader thread on commit)  */
    void ConsoleWrite(std::wstring_view Msg)
    {
        {
            std::lock_guard lock(ConsoleMutex);
            std::wcout.clear();                           // <- added
            std::wcout << L"\r\x1B[2K" << Msg << std::flush;
        }
        RedrawInput(false);
    }

    /* reader thread - raw mode, key-by-key */
    void InputLoop(std::stop_token st)
    {
        INPUT_RECORD rec{};
        DWORD        read = 0;
        wchar_t      pendingHigh = 0;                 // stores an unmatched high surrogate
    
        RedrawInput(true);                            // initial "> "
    
        while (!st.stop_requested())
        {
            if (!ReadConsoleInputW(StdIn(), &rec, 1, &read))
                continue;
    
            if (rec.EventType != KEY_EVENT || !rec.Event.KeyEvent.bKeyDown)
                continue;
    
            const wchar_t wch = rec.Event.KeyEvent.uChar.UnicodeChar;
    
            switch (wch)
            {
                case L'\r':                                              // ENTER
                {
                    {
                        std::scoped_lock lk(InputMutex);
                        if (pendingHigh) {                               // flush stray high surrogate
                            CurrentLine.push_back(pendingHigh);
                            pendingHigh = 0;
                        }
                        InputQueue.push(CurrentLine);
                        CurrentLine.clear();
                    }
    
                    ConsoleWrite(L"");                                   // no extra blank line
                    RedrawInput(true);
                } break;
    
                case L'\b':                                              // BACKSPACE
                {
                    std::scoped_lock lk(InputMutex);
                    if (pendingHigh) {
                        pendingHigh = 0;                                 // just discard it
                    } else if (!CurrentLine.empty()) {
                        CurrentLine.pop_back();
                    }
                    RedrawInput(true);
                } break;
    
                default:                                                 // printable
                {
                    if (wch < L' ')                                      // control -> ignore
                        break;
    
                    if (wch >= 0xD800 && wch <= 0xDBFF) {                // high surrogate
                        pendingHigh = wch;
                        break;
                    }
    
                    if (wch >= 0xDC00 && wch <= 0xDFFF) {                // low surrogate
                        if (pendingHigh) {
                            std::scoped_lock lk(InputMutex);
                            CurrentLine.push_back(pendingHigh);
                            CurrentLine.push_back(wch);
                            pendingHigh = 0;
                            RedrawInput(false);
                        }
                        /* stray low surrogate -> drop */
                        break;
                    }
    
                    /* ordinary BMP char */
                    if (pendingHigh) {                                   // flush orphan high
                        std::scoped_lock lk(InputMutex);
                        CurrentLine.push_back(pendingHigh);
                        pendingHigh = 0;
                    }
                    {
                        std::scoped_lock lk(InputMutex);
                        CurrentLine.push_back(wch);
                    }
                    RedrawInput(false);
                } break;
            }
        }
    }
} // anonym-ns

/* ---------------- public API expected by Log.h ---------------- */

static void EnableVirtualTerminalProcessing()
{
    GetConsoleMode(StdOut(), &OldOutMode);
    SetConsoleMode(StdOut(), OldOutMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

std::optional<std::string> GetPendingConsoleCommand()
{
    std::scoped_lock lk(InputMutex);

    if (InputQueue.empty())
        return std::nullopt;

    std::wstring w = std::move(InputQueue.front());
    InputQueue.pop();
    return NarrowUTF8(w);
}

void InitializeLogging()
{
    EnableVirtualTerminalProcessing();

    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stderr), _O_U16TEXT);

    /* put stdin in "raw" -- no line buffering, no echo */
    GetConsoleMode(StdIn(), &OldInMode);
    DWORD inMode = OldInMode &
                   ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
    SetConsoleMode(StdIn(), inMode);

    /* launch background reader */
    ReaderThread = std::make_unique<std::jthread>(InputLoop);
}

void CleanupLogging()
{
    if (ReaderThread && ReaderThread->joinable())
    {
        CancelSynchronousIo(ReaderThread->native_handle());
        ReaderThread->request_stop();
        ReaderThread->join();
        ReaderThread.reset();
    }

    /* restore console modes */
    SetConsoleMode(StdIn(),  OldInMode);
    SetConsoleMode(StdOut(), OldOutMode);
}

void LogImplementation(ELogLevel Level, const std::wstring& Message)
{
    const std::wstring FullMessage = std::format(
        L"[NyaaiFortLauncher] {}{}{}: {}\n",
        LogLevelToColorCode(Level), LogLevelToString(Level), L"\x1B[0m",
        Message);

    LogRaw(FullMessage);  // delegate to the lower layer
}

void LogRaw(const std::wstring& Message)
{
    ConsoleWrite(Message);      // keep the prompt tidy
}
