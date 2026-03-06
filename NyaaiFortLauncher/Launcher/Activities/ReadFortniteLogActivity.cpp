#include "ReadFortniteLogActivity.h"

#include "Launcher/CommandManager.h"
#include "Launcher/FortLauncher.h"

#include "Utils/WindowsInclude.h"

GENERATE_BASE_CPP(NReadFortniteLogActivity)

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
        &NReadFortniteLogActivity::ForwardCommandToFortniteCommand
    );
}

void NReadFortniteLogActivity::Tick(double DeltaTime)
{
    Super::Tick(DeltaTime);

    HANDLE StdOutReadPipeHandle = GetLauncher()->GetFortniteStdOutReadPipeHandle();
    if (!StdOutReadPipeHandle)
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
            Log(
                Error, 
                L"NReadFortniteLogActivity: PeekNamedPipe failed with error: {}. This is most likely caused be a dll doing AllocConsole or redirecting stdout",
                GetLastError()
                );
            
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
            Log(
                Error, 
                L"NReadFortniteLogActivity: ReadFile failed with error: {}. This is most likely caused be a dll doing AllocConsole or redirecting stdout",
                GetLastError()
                );
            
            return;
        }

        Utf8StreamDecoder.Append(Buffer, BytesRead);

        const std::wstring Decoded = Utf8StreamDecoder.ConsumeAll();
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

    const bool bIsGreen = IsCurrentLineGreen();

    if (ShouldPrintLine(bIsGreen))
    {
        PendingOutput += BuildColoredLine(CurrentLine, bIsGreen);
    }

    CurrentLine.clear();

    static constexpr size_t MaxPendingOutputLength = 32 * 1024;
    if (PendingOutput.size() >= MaxPendingOutputLength)
    {
        LogRaw(PendingOutput);
        PendingOutput.clear();
    }
}

bool NReadFortniteLogActivity::IsCurrentLineGreen() const
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

std::wstring NReadFortniteLogActivity::BuildColoredLine(const std::wstring& Line, bool bIsGreen) const
{
    const wchar_t* Color = bIsGreen ? L"\x1B[32m" : L"\x1B[90m";
    std::wstring Out;
    Out.reserve(Line.size() + 16);
    Out += Color;
    Out += Line;
    Out += L"\x1B[0m";
    return Out;
}

void NReadFortniteLogActivity::ProcessLogTriggeredActions(wchar_t Char)
{
    std::vector<int32> ToRemove{};

    for (int32 i = 0; i < static_cast<int32>(LogTriggeredActions.size()); i++)
    {
        auto& LogTriggeredAction = LogTriggeredActions[i];

        const bool bInRange =
            !LogTriggeredAction.TriggerString.empty() &&
            LogTriggeredAction.TriggerStringCharsFound < static_cast<uint32>(LogTriggeredAction.TriggerString.size());

        if (bInRange && Char == LogTriggeredAction.TriggerString[LogTriggeredAction.TriggerStringCharsFound])
        {
            LogTriggeredAction.TriggerStringCharsFound++;
        }
        else
        {
            LogTriggeredAction.TriggerStringCharsFound = 0;
        }

        if (LogTriggeredAction.TriggerStringCharsFound >= static_cast<uint32>(LogTriggeredAction.TriggerString.size()))
        {
            LogTriggeredAction.TriggerStringCharsFound = 0;
            
            if (LogTriggeredAction.Action)
            {
                NUniquePtr<NAction> Action = LogTriggeredAction.Action.NewObject(this);
            }
            
            for (const auto& ActionTemplate : LogTriggeredAction.Actions)
            {
                NUniquePtr<NAction> Action = ActionTemplate.NewObject(this);
            }

            if (LogTriggeredAction.bTriggerOnlyOnce)
            {
                ToRemove.push_back(i);
            }
        }
    }

    for (int32 i = static_cast<int32>(ToRemove.size()) - 1; i >= 0; --i)
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
        static_cast<uint32>(CommandBytes.size()),
        &BytesWritten,
        nullptr))
    {
        Log(Error, L"ForwardCommandToFortniteCommand: WriteFile failed: {}", GetLastError());
    }
}
