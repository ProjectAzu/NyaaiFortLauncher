module;
#include "Utils/WindowsInclude.h"

module ReadFortniteLogActivity;

void NReadFortniteLogActivity::OnCreated()
{
    Super::OnCreated();

    if (!bPrintFortniteLog)
    {
        Log(Info, L"Printing fortnite log is off");
    }
    else if (bOnlyPrintLogWithColoredPrintPrefix && ColoredPrintPrefix.empty())
    {
        Log(Warning, L"bOnlyPrintLogWithColoredPrintPrefix is true but the ColoredPrintPrefix is not set, will not print anything");
    }

    GetCommandManager().RegisterConsoleCommand(
        this,
        L"fn",
        L"Forwards the command to fortnite",
        &NReadFortniteLogActivity::ForwardCommandToFortniteCommand);
}

void NReadFortniteLogActivity::Tick(double DeltaTime)
{
    Super::Tick(DeltaTime);

    HANDLE StdOutReadPipeHandle = GetLauncher()->GetFortniteStdOutReadPipeHandle();

    if (!StdOutReadPipeHandle || bPipeClosed)
    {
        return;
    }

    ReadAndProcessPipe(StdOutReadPipeHandle);

    if (!PendingOutput.empty())
    {
        LogRaw(PendingOutput);
        PendingOutput.clear();
    }
}

void NReadFortniteLogActivity::ReadAndProcessPipe(void* StdOutReadPipeHandle)
{
    while (true)
    {
        DWORD BytesAvailable = 0;

        if (!::PeekNamedPipe(StdOutReadPipeHandle, nullptr, 0, nullptr, &BytesAvailable, nullptr))
        {
            const DWORD ErrorCode = GetLastError();
            bPipeClosed = true;
            FlushCurrentLine(true);

            if (ErrorCode == ERROR_BROKEN_PIPE || ErrorCode == ERROR_PIPE_NOT_CONNECTED)
            {
                return;
            }

            Log(
                Error,
                L"NReadFortniteLogActivity: PeekNamedPipe failed with error: {}. This is most likely caused be a dll doing AllocConsole or redirecting stdout",
                ErrorCode);

            return;
        }

        if (BytesAvailable == 0)
        {
            return;
        }

        constexpr DWORD MaxChunk = 4096;
        const DWORD BytesToRead = (BytesAvailable < MaxChunk) ? BytesAvailable : MaxChunk;

        char Buffer[MaxChunk];
        DWORD BytesRead = 0;

        if (!ReadFile(StdOutReadPipeHandle, Buffer, BytesToRead, &BytesRead, nullptr) || BytesRead == 0)
        {
            const DWORD ErrorCode = GetLastError();
            bPipeClosed = true;
            FlushCurrentLine(true);

            if (ErrorCode == ERROR_BROKEN_PIPE || ErrorCode == ERROR_PIPE_NOT_CONNECTED)
            {
                return;
            }

            Log(
                Error,
                L"NReadFortniteLogActivity: ReadFile failed with error: {}. This is most likely caused be a dll doing AllocConsole or redirecting stdout",
                ErrorCode);

            return;
        }

        std::wstring Decoded{};

        if (bReadUtf16)
        {
            Utf16StreamDecoder.Append(Buffer, BytesRead);
            Decoded = Utf16StreamDecoder.ConsumeAll();
        }
        else
        {
            Utf8StreamDecoder.Append(Buffer, BytesRead);
            Decoded = Utf8StreamDecoder.ConsumeAll();
        }

        if (!Decoded.empty())
        {
            HandleDecodedText(Decoded);
        }
    }
}

void NReadFortniteLogActivity::HandleDecodedText(const std::wstring& Text)
{
    for (wchar_t Ch : Text)
    {
        if (Ch == L'\r')
        {
            continue;
        }

        ProcessLogTriggeredActions(Ch);

        if (!bPrintFortniteLog)
        {
            continue;
        }

        AppendToCurrentLine(Ch);
    }
}

void NReadFortniteLogActivity::AppendToCurrentLine(wchar_t Ch)
{
    CurrentLine.push_back(Ch);

    static constexpr uint32 MaxLineLength = 3000;

    if (Ch == L'\n' || CurrentLine.size() >= MaxLineLength)
    {
        FlushCurrentLine(true);
    }
}

void NReadFortniteLogActivity::FlushCurrentLine(bool bForce)
{
    if (CurrentLine.empty())
    {
        return;
    }

    if (!bForce && CurrentLine.back() != L'\n')
    {
        return;
    }

    const bool bIsColored = IsCurrentLineColored();

    if (ShouldPrintLine(bIsColored))
    {
        PendingOutput += BuildColoredLine(CurrentLine, bIsColored);
    }

    CurrentLine.clear();

    static constexpr size_t MaxPendingOutputLength = 32 * 1024;

    if (PendingOutput.size() >= MaxPendingOutputLength)
    {
        LogRaw(PendingOutput);
        PendingOutput.clear();
    }
}

bool NReadFortniteLogActivity::IsCurrentLineColored() const
{
    if (ColoredPrintPrefix.empty())
    {
        return false;
    }

    if (CurrentLine.size() < ColoredPrintPrefix.size())
    {
        return false;
    }

    return CurrentLine.starts_with(ColoredPrintPrefix);
}

bool NReadFortniteLogActivity::ShouldPrintLine(bool bIsGreen) const
{
    if (!bOnlyPrintLogWithColoredPrintPrefix)
    {
        return true;
    }

    if (ColoredPrintPrefix.empty())
    {
        return false;
    }

    return bIsGreen;
}

static bool StringContainsCaseInsensitive(std::wstring_view haystack, std::wstring_view needle)
{
    if (needle.empty())
    {
        return true;
    }

    auto it = std::ranges::search(
        haystack,
        needle,
        {},
        [](wchar_t ch)
    {
        return std::towlower(ch);
    },
        [](wchar_t ch)
    {
        return std::towlower(ch);
    });

    return !it.empty();
}

std::wstring NReadFortniteLogActivity::BuildColoredLine(const std::wstring& Line, bool bIsColored) const
{
    const wchar_t* Color = L"\x1B[90m"; // gray

    if (bIsColored)
    {
        if (StringContainsCaseInsensitive(Line, L"error"))
        {
            Color = L"\x1B[31m"; // red
        }
        else if (StringContainsCaseInsensitive(Line, L"warning"))
        {
            Color = L"\x1B[33m"; // yellow
        }
        else
        {
            Color = L"\x1B[32m"; // green
        }
    }

    std::wstring Out{};

    Out.reserve(Line.size() + 16);

    Out += Color;
    Out += Line;
    Out += L"\x1B[0m";

    return Out;
}

void NReadFortniteLogActivity::ProcessLogTriggeredActions(wchar_t Char)
{
    std::vector<int32> ToRemove{};

    for (int32 i = 0; i < scast<int32>(LogTriggeredActions.size()); i++)
    {
        auto& LogTriggeredAction = LogTriggeredActions[i];

        if (LogTriggeredAction.TriggerString.empty())
        {
            continue;
        }

        const auto& Trigger = LogTriggeredAction.TriggerString;
        auto& Matched = LogTriggeredAction.TriggerStringCharsFound;

        if (Matched >= Trigger.size())
        {
            Matched = 0;
        }

        while (Matched && Char != Trigger[Matched])
        {
            uint32 PrefixLength = Matched - 1;

            while (PrefixLength && Trigger.compare(0, PrefixLength, Trigger, Matched - PrefixLength, PrefixLength) != 0)
            {
                --PrefixLength;
            }

            Matched = PrefixLength;
        }

        if (Char == Trigger[Matched])
        {
            ++Matched;
        }

        if (LogTriggeredAction.TriggerStringCharsFound >= scast<uint32>(LogTriggeredAction.TriggerString.size()))
        {
            LogTriggeredAction.TriggerStringCharsFound = 0;

            if (LogTriggeredAction.Action)
            {
                TUniqueObjectPtr<NAction> Action = LogTriggeredAction.Action.NewObject(this);
            }

            for (const auto& ActionTemplate : LogTriggeredAction.Actions)
            {
                TUniqueObjectPtr<NAction> Action = ActionTemplate.NewObject(this);
            }

            if (LogTriggeredAction.bTriggerOnlyOnce)
            {
                ToRemove.push_back(i);
            }
        }
    }

    for (int32 i = scast<int32>(ToRemove.size()) - 1; i >= 0; --i)
    {
        const int32 Idx = ToRemove[i];
        LogTriggeredActions[Idx] = LogTriggeredActions.back();
        LogTriggeredActions.pop_back();
    }
}

void NReadFortniteLogActivity::ForwardCommandToFortniteCommand(const FCommandArguments& Args)
{
    std::wstring CommandRaw = Args.GetRawString();

    if (CommandRaw.empty())
    {
        return;
    }

    CommandRaw += L'\n';

    HANDLE PipeHandle = GetLauncher()->GetFortniteStdInWritePipeHandle();

    if (!PipeHandle)
    {
        Log(Error, L"ForwardCommandToFortniteCommand: No pipe handle");

        return;
    }

    const std::string CommandBytes = WideToUtf8(CommandRaw);

    DWORD BytesWritten = 0;

    if (!WriteFile(
            PipeHandle,
            CommandBytes.data(),
            scast<uint32>(CommandBytes.size()),
            &BytesWritten,
            nullptr))
    {
        Log(Error, L"ForwardCommandToFortniteCommand: WriteFile failed: {}", GetLastError());
    }
}

void NReadFortniteLogActivity::OnDestroyed()
{
    if (auto Pipe = GetLauncher()->GetFortniteStdOutReadPipeHandle(); Pipe && !bPipeClosed)
    {
        ReadAndProcessPipe(Pipe);
    }

    FlushCurrentLine(true);

    if (!PendingOutput.empty())
    {
        LogRaw(PendingOutput);
        PendingOutput.clear();
    }

    Super::OnDestroyed();
}
