#include "ReadFortniteLogActivity.h"

#include "FortLauncher.h"

#include <iostream>
#include <string>
#include <vector>

#include "Utils/WindowsInclude.h"

GENERATE_BASE_CPP(NReadFortniteLogActivity)

static void FlushIfBig(std::wstring& out, size_t threshold = 4096)
{
    if (out.size() >= threshold)
    {
        LogRaw(out);
        out.clear();
    }
}

void NReadFortniteLogActivity::OnCreated()
{
    Super::OnCreated();

    if (bPrintFortniteLog)
    {
        if (bOnlyPrintLogWithColoredPrintPrefix && ColoredPrintPrefix.empty())
        {
            Log(Warning, L"bOnlyPrintLogWithColoredPrintPrefix is true but the ColoredPrintPrefix is not set, will not print anything");
        }
    }
    else
    {
        Log(Info, L"Printing fortnite log is off");
    }

    GetLauncher()->RegisterConsoleCommand(
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
        return;

    std::wstring Out{};
    Out.reserve(4096);

    const std::wstring& Prefix = ColoredPrintPrefix;
    const size_t PrefixLen = Prefix.size();

    auto ResetForNewLine = [&]()
    {
        bAtLineStart = true;
        bDecidingPrefix = true;
        bLineIsGreen = false;
        PrefixIndex = 0;
        LineStartBuffer.clear();
    };

    auto CommitDecisionAndFlushBuffered = [&](bool isGreen)
    {
        bLineIsGreen = isGreen;
        bDecidingPrefix = false;

        if (!bOnlyPrintLogWithColoredPrintPrefix || bLineIsGreen)
        {
            Out += bLineIsGreen ? L"\x1B[32m" : L"\x1B[90m";
            Out += LineStartBuffer;
        }

        LineStartBuffer.clear();
    };

    auto EmitCharIfAllowed = [&](wchar_t ch)
    {
        if (!bOnlyPrintLogWithColoredPrintPrefix || bLineIsGreen)
        {
            Out.push_back(ch);
        }
    };

    auto EndLineIfNeeded = [&]()
    {
        if (!bOnlyPrintLogWithColoredPrintPrefix || bLineIsGreen)
        {
            Out += L"\x1B[0m";
        }
        ResetForNewLine();
    };

    while (true)
    {
        DWORD BytesAvailable = 0;
        if (!::PeekNamedPipe(StdOutReadPipeHandle, nullptr, 0, nullptr, &BytesAvailable, nullptr))
        {
            Log(Error, L"PeekNamedPipe failed with error: {}", GetLastError());
            break;
        }

        if (BytesAvailable == 0)
            break;

        constexpr DWORD MAX_CHUNK = 4096;
        const DWORD BytesToRead = (BytesAvailable < MAX_CHUNK) ? BytesAvailable : MAX_CHUNK;

        char Buffer[MAX_CHUNK];
        DWORD BytesRead = 0;
        if (!::ReadFile(StdOutReadPipeHandle, Buffer, BytesToRead, &BytesRead, nullptr) || BytesRead == 0)
        {
            Log(Error, L"ReadFile failed with error: {}", GetLastError());
            break;
        }

        Utf8StreamDecoder.Append(Buffer, BytesRead);
        const std::wstring ReadString = Utf8StreamDecoder.ConsumeAll();

        for (wchar_t ch : ReadString)
        {
            // Optional but usually correct for piped text:
            if (ch == L'\r') continue;

            ProcessTriggeredActions(ch);

            if (!bPrintFortniteLog)
                continue;

            if (bAtLineStart)
            {
                bAtLineStart = false;
                bDecidingPrefix = (PrefixLen > 0);
                bLineIsGreen = false;
                PrefixIndex = 0;
                LineStartBuffer.clear();

                // If no prefix (or only-colored with empty prefix), we effectively won’t print lines.
                if (PrefixLen == 0)
                {
                    if (!bOnlyPrintLogWithColoredPrintPrefix)
                    {
                        Out += L"\x1B[90m"; // gray line
                        bDecidingPrefix = false;
                    }
                }
            }

            // If we still need to decide whether this line is colored:
            if (bDecidingPrefix)
            {
                LineStartBuffer.push_back(ch);

                // Newline before decision => not colored
                if (ch == L'\n')
                {
                    CommitDecisionAndFlushBuffered(false);
                    EndLineIfNeeded();
                    FlushIfBig(Out);
                    continue;
                }

                // Mismatch? decide immediately (this is the key improvement)
                if (PrefixIndex >= PrefixLen || ch != Prefix[PrefixIndex])
                {
                    CommitDecisionAndFlushBuffered(false);
                    FlushIfBig(Out);
                    continue;
                }

                // Matched one more char
                ++PrefixIndex;

                // Full prefix matched => colored line, flush immediately
                if (PrefixIndex == PrefixLen)
                {
                    CommitDecisionAndFlushBuffered(true);
                }

                FlushIfBig(Out);
                continue;
            }

            // Normal streaming after decision
            EmitCharIfAllowed(ch);

            if (ch == L'\n')
            {
                EndLineIfNeeded();
            }

            FlushIfBig(Out);
        }
    }

    if (!Out.empty())
    {
        LogRaw(Out);
    }
}

void NReadFortniteLogActivity::ProcessTriggeredActions(wchar_t ch)
{
    std::vector<int32> ToRemove{};

    for (int32 i = 0; i < static_cast<int32>(LogTriggeredActions.size()); i++)
    {
        auto& LogTriggeredAction = LogTriggeredActions[i];

        const bool bInRange =
            !LogTriggeredAction.TriggerString.empty() &&
            LogTriggeredAction.TriggerStringCharsFound < static_cast<uint32>(LogTriggeredAction.TriggerString.size());

        if (bInRange && ch == LogTriggeredAction.TriggerString[LogTriggeredAction.TriggerStringCharsFound])
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
            NUniquePtr<NAction> Action = LogTriggeredAction.Action.NewObject(this);

            if (LogTriggeredAction.bTriggerOnlyOnce)
            {
                ToRemove.push_back(i);
            }
        }
    }

    for (int32 i = static_cast<int32>(ToRemove.size()) - 1; i >= 0; --i)
    {
        const int32 idx = ToRemove[i];
        LogTriggeredActions[idx] = LogTriggeredActions.back();
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

    auto PipeHandle = GetLauncher()->GetFortniteStdInWritePipeHandle();
    if (!PipeHandle)
    {
        Log(Error, L"ForwardCommandToFortniteCommand: No pipe handle.");
        return;
    }

    const std::string CommandBytes = WideToUtf8(CommandRaw);

    DWORD BytesWritten = 0;
    if (!WriteFile(
        PipeHandle,
        CommandBytes.data(),
        static_cast<uint32>(CommandBytes.size()),
        &BytesWritten,
        nullptr
    ))
    {
        Log(Error, L"ForwardCommandToFortniteCommand: WriteFile failed: {}.", GetLastError());
    }
}
