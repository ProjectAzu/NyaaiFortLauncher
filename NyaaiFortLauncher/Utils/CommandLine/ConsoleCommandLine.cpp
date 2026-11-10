module;
#include "Utils/WindowsInclude.h"

module ConsoleCommandLine;

void NConsoleCommandLine::Write(std::wstring_view Text)
{
    if (bConsoleOutput)
    {
        while (!Text.empty())
        {
            DWORD Written = 0;

            if (!WriteConsoleW(OutputHandle, Text.data(), scast<DWORD>(std::min<size_t>(Text.size(), 16384)), &Written, nullptr) || !Written)
            {
                break;
            }

            Text.remove_prefix(Written);
        }

        return;
    }

    std::string Bytes = WideToUtf8(std::wstring(Text));
    size_t Offset = 0;

    while (Offset < Bytes.size())
    {
        DWORD Written = 0;

        if (!WriteFile(OutputHandle, Bytes.data() + Offset, scast<DWORD>(std::min<size_t>(Bytes.size() - Offset, 16384)), &Written, nullptr) || !Written)
        {
            break;
        }

        Offset += Written;
    }
}

void NConsoleCommandLine::ClearInput()
{
    if (!bPromptVisible)
    {
        return;
    }

    Write(L"\r\x1b[2K");
    bPromptVisible = false;
}

void NConsoleCommandLine::RedrawInput()
{
    if (!bConsoleInput || !bConsoleOutput)
    {
        return;
    }

    ClearInput();
    CONSOLE_SCREEN_BUFFER_INFO Info{};

    if (!GetConsoleScreenBufferInfo(OutputHandle, &Info))
    {
        return;
    }

    size_t Width = scast<size_t>(std::max<int>(Info.dwSize.X - 3, 1));
    size_t Start = Cursor > Width ? Cursor - Width : 0;
    std::wstring_view Visible(Input.data() + Start, std::min(Width, Input.size() - Start));
    Write(L"\x1b[0m> ");
    Write(Visible);
    const size_t DisplayCursor = Cursor - Start;

    if (Visible.size() > DisplayCursor)
    {
        Write(std::format(L"\x1b[{}D", Visible.size() - DisplayCursor));
    }

    bPromptVisible = true;
}

void NConsoleCommandLine::OnCreated()
{
    Super::OnCreated();
    InputHandle = GetStdHandle(STD_INPUT_HANDLE);
    OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    bConsoleInput = GetConsoleMode(InputHandle, &OriginalInputMode) != FALSE;
    bConsoleOutput = GetConsoleMode(OutputHandle, &OriginalOutputMode) != FALSE;

    if (bConsoleOutput)
    {
        SetConsoleMode(OutputHandle, OriginalOutputMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }

    if (bConsoleInput)
    {
        SetConsoleMode(InputHandle, (OriginalInputMode | ENABLE_WINDOW_INPUT | ENABLE_EXTENDED_FLAGS) & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_QUICK_EDIT_MODE | ENABLE_MOUSE_INPUT));
        RedrawInput();
    }
    InputThread = std::jthread([this](std::stop_token StopToken)
    {
        InputWorker(StopToken);
    });
}

void NConsoleCommandLine::OnDestroyed()
{
    InputThread.request_stop();

    if (InputThread.joinable())
    {
        InputThread.join();
    }
    {
        std::scoped_lock Lock{ OutputMutex };
        ClearInput();
        Write(PendingLog);
        PendingLog.clear();

        if (bConsoleInput)
        {
            SetConsoleMode(InputHandle, OriginalInputMode);
        }

        if (bConsoleOutput)
        {
            SetConsoleMode(OutputHandle, OriginalOutputMode);
        }
    }

    Super::OnDestroyed();
}

void NConsoleCommandLine::Log(const std::wstring& Message)
{
    std::scoped_lock Lock{ OutputMutex };

    if (!bConsoleInput || !bConsoleOutput)
    {
        Write(Message);

        return;
    }

    PendingLog += Message;
    const size_t LastNewline = PendingLog.rfind(L'\n');

    if (LastNewline == std::wstring::npos)
    {
        return;
    }

    ClearInput();
    Write(std::wstring_view(PendingLog).substr(0, LastNewline + 1));
    PendingLog.erase(0, LastNewline + 1);
    RedrawInput();
}

void NConsoleCommandLine::InputWorker(std::stop_token StopToken)
{
    FUtf8StreamDecoder Decoder{};
    std::wstring RedirectedLine{};
    while (!StopToken.stop_requested())
    {
        if (!bConsoleInput)
        {
            DWORD Available = 0;

            if (GetFileType(InputHandle) == FILE_TYPE_PIPE)
            {
                if (!PeekNamedPipe(InputHandle, nullptr, 0, nullptr, &Available, nullptr))
                {
                    break;
                }

                if (!Available)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                    continue;
                }
            }
            else if (GetFileType(InputHandle) != FILE_TYPE_DISK)
            {
                break;
            }

            char Buffer[4096];
            DWORD Read = 0;

            if (!ReadFile(InputHandle, Buffer, sizeof(Buffer), &Read, nullptr) || !Read)
            {
                break;
            }

            Decoder.Append(Buffer, Read);

            for (wchar_t Character : Decoder.ConsumeAll())
            {
                if (Character == L'\n')
                {
                    EnqueueCommand(RedirectedLine);
                    RedirectedLine.clear();
                }
                else if (Character != L'\r')
                {
                    RedirectedLine += Character;
                }
            }

            continue;
        }

        if (WaitForSingleObject(InputHandle, 20) != WAIT_OBJECT_0)
        {
            continue;
        }

        INPUT_RECORD Record{};
        DWORD Read = 0;

        if (!ReadConsoleInputW(InputHandle, &Record, 1, &Read))
        {
            break;
        }

        std::unique_lock Lock{ OutputMutex };

        if (Record.EventType == WINDOW_BUFFER_SIZE_EVENT)
        {
            RedrawInput();
            continue;
        }

        if (Record.EventType != KEY_EVENT || !Record.Event.KeyEvent.bKeyDown)
        {
            continue;
        }

        const auto& Key = Record.Event.KeyEvent;

        for (WORD Repeat = 0; Repeat < Key.wRepeatCount; ++Repeat)
        {
            switch (Key.wVirtualKeyCode)
            {
            case VK_RETURN:
            {
                ClearInput();
                Write(L"> " + Input + L"\n");
                std::wstring Command = std::exchange(Input, {});
                Cursor = 0;

                if (!Command.empty() && (History.empty() || History.back() != Command))
                {
                    History.push_back(Command);
                }

                HistoryIndex = History.size();
                SavedInput.clear();
                Lock.unlock();
                EnqueueCommand(Command);
                Lock.lock();
                break;
            }
            case VK_LEFT:
                if (Cursor)
                {
                    --Cursor;
                }

                break;
            case VK_RIGHT:
                if (Cursor < Input.size())
                {
                    ++Cursor;
                }

                break;
            case VK_HOME:
                Cursor = 0;
                break;
            case VK_END:
                Cursor = Input.size();
                break;
            case VK_BACK:
                if (Cursor)
                {
                    Input.erase(--Cursor, 1);
                }

                break;
            case VK_DELETE:
                if (Cursor < Input.size())
                {
                    Input.erase(Cursor, 1);
                }

                break;
            case VK_UP:
                if (HistoryIndex == History.size())
                {
                    SavedInput = Input;
                }

                if (HistoryIndex)
                {
                    Input = History[--HistoryIndex];
                }

                Cursor = Input.size();
                break;
            case VK_DOWN:
                if (HistoryIndex < History.size())
                {
                    Input = ++HistoryIndex == History.size() ? SavedInput : History[HistoryIndex];
                }

                Cursor = Input.size();
                break;
            default:
                if (Key.uChar.UnicodeChar == 3)
                {
                    Input.clear();
                    Cursor = 0;
                }
                else if (Key.uChar.UnicodeChar >= L' ')
                {
                    Input.insert(Cursor++, 1, Key.uChar.UnicodeChar);
                }

                break;
            }
        }

        RedrawInput();
    }

    if (!RedirectedLine.empty())
    {
        EnqueueCommand(RedirectedLine);
    }
}
