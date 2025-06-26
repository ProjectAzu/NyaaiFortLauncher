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
#include <vector>
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

    /* history ---------------------------------------------------------------- */
    std::vector<std::wstring> History;          // all previous commands
    std::size_t               HistIndex = 0;    // 0 ... History.size()
    std::wstring              DraftLine;        // what the user had typed before
    // valid only when browsing history

    /* live-editing ----------------------------------------------------------- */
    std::size_t CursorPos = 0;                  // index inside CurrentLine

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
        // snapshot the line & cursor so we don't hold InputMutex while printing
        std::wstring line;
        std::size_t  pos;
        {
            std::scoped_lock lk(InputMutex);
            line = CurrentLine;
            pos  = CursorPos;
        }

        /* --------- atomic console update --------- */
        std::lock_guard out(ConsoleMutex);
        std::wcout.clear();

        if (ClearLine)
            std::wcout << L"\r\x1B[2K";                 // erase line

        std::wcout << L"\r" << Prompt << line;

        /* place caret: "nD" moves n columns left */
        const std::size_t tail = line.size() - pos;
        if (tail)
            std::wcout << L"\x1B[" << tail << L"D";

        std::wcout << std::flush;
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
        constexpr DWORD  MAX_BATCH = 256;
        INPUT_RECORD     recs[MAX_BATCH];
        wchar_t          pendingHigh = 0;
    
        RedrawInput(true);        // initial prompt
    
        while (!st.stop_requested())
        {
            DWORD avail = 0;
            if (!GetNumberOfConsoleInputEvents(StdIn(), &avail) || avail == 0)
            {
                Sleep(1);
                continue;
            }
    
            DWORD toRead = std::min<DWORD>(avail, MAX_BATCH);
            DWORD read   = 0;
            if (!ReadConsoleInputW(StdIn(), recs, toRead, &read) || read == 0)
                continue;
    
            bool lineDirty = false;      // did we change the line?
            bool clearLine = false;      // need full clear? (arrow / backspace)
    
            for (DWORD i = 0; i < read; ++i)
            {
                const auto& ev = recs[i].Event.KeyEvent;
                if (recs[i].EventType != KEY_EVENT || !ev.bKeyDown)
                    continue;
    
                const WORD vk  = ev.wVirtualKeyCode;
                const wchar_t ch = ev.uChar.UnicodeChar;
    
                /* ---------- navigation keys (virtual key codes) ---------- */
                if (vk == VK_LEFT)
                {
                    std::scoped_lock lk(InputMutex);
                    if (CursorPos > 0) { --CursorPos; lineDirty = true; }
                    continue;
                }
                if (vk == VK_RIGHT)
                {
                    std::scoped_lock lk(InputMutex);
                    if (CursorPos < CurrentLine.size()) { ++CursorPos; lineDirty = true; }
                    continue;
                }
                if (vk == VK_UP)
                {
                    std::scoped_lock lk(InputMutex);
                    if (!History.empty() && HistIndex > 0)
                    {
                        if (HistIndex == History.size())
                            DraftLine = CurrentLine;      // remember current edit
    
                        --HistIndex;
                        CurrentLine = History[HistIndex];
                        CursorPos   = CurrentLine.size();
                        clearLine   = lineDirty = true;
                    }
                    continue;
                }
                if (vk == VK_DOWN)
                {
                    std::scoped_lock lk(InputMutex);
                    if (HistIndex < History.size())
                    {
                        ++HistIndex;
                        if (HistIndex == History.size())
                            CurrentLine = DraftLine;
                        else
                            CurrentLine = History[HistIndex];
    
                        CursorPos = CurrentLine.size();
                        clearLine = lineDirty = true;
                    }
                    continue;
                }
    
                /* ---------- printable characters / control ---------- */
                switch (ch)
                {
                    case L'\r':                                     /* ENTER */
                    {
                        std::wstring committed;
                        {
                            std::scoped_lock lk(InputMutex);
                            if (pendingHigh) {                      // orphan high surrogate
                                CurrentLine.insert(CursorPos++, 1, pendingHigh);
                                pendingHigh = 0;
                            }
                            committed.swap(CurrentLine);
                            CursorPos = 0;
                        }
    
                        if (!committed.empty())
                        {
                            std::scoped_lock lk(InputMutex);
                            History.push_back(committed);
                            HistIndex = History.size();
                        }
    
                        {
                            std::scoped_lock lk(InputMutex);
                            InputQueue.push(committed);
                        }
    
                        ConsoleWrite(L"");
                        clearLine = true;
                    } break;
    
                    case L'\b':                                     /* BACKSPACE */
                    {
                        std::scoped_lock lk(InputMutex);
                        if (pendingHigh) {
                            pendingHigh = 0;
                        } else if (CursorPos) {
                            CurrentLine.erase(--CursorPos, 1);
                        }
                        clearLine = lineDirty = true;
                    } break;
    
                    default:                                        /* text input */
                    {
                        if (ch < L' ') break;                       // ignore controls
    
                        std::scoped_lock lk(InputMutex);
    
                        if (ch >= 0xD800 && ch <= 0xDBFF)           // high surrogate
                        {
                            pendingHigh = ch;
                            break;
                        }
                        if (ch >= 0xDC00 && ch <= 0xDFFF)           // low surrogate
                        {
                            if (pendingHigh)
                            {
                                CurrentLine.insert(CursorPos++, 1, pendingHigh);
                                pendingHigh = 0;
                                CurrentLine.insert(CursorPos++, 1, ch);
                                lineDirty = true;
                            }
                            break;
                        }
    
                        if (pendingHigh)                            // orphan high
                        {
                            CurrentLine.insert(CursorPos++, 1, pendingHigh);
                            pendingHigh = 0;
                        }
                        CurrentLine.insert(CursorPos++, 1, ch);
                        lineDirty = true;
                    } break;
                } // switch
            }     // for each record
    
            if (lineDirty)
                RedrawInput(clearLine);
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
    
    _setmode(_fileno(stdout), _O_TEXT);
    _setmode(_fileno(stderr), _O_TEXT);
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
