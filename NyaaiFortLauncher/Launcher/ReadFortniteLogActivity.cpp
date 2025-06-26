#include "ReadFortniteLogActivity.h"

#include "FortLauncher.h"

#include <iostream>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

GENERATE_BASE_CPP(NReadFortniteLogActivity)

void NReadFortniteLogActivity::OnCreated()
{
    Super::OnCreated();

    if (bPrintFortniteLog)
    {
        if (bOnlyPrintLogWithColoredPrintPrefix && ColoredPrintPrefix.empty())
        {
            Log(Warning, "bOnlyPrintLogWithColoredPrintPrefix is true but the ColoredPrintPrefix is not set, will not print anything");
        }   
    }
    else
    {
        Log(Info, "Printing fortnite log is off");
    }

    GetLauncher()->RegisterConsoleCommand(
        this,
        "fn",
        "Forwards the command to the fortnite",
        &NReadFortniteLogActivity::ForwardCommandToFortniteCommand
        );
}

void NReadFortniteLogActivity::Tick(double DeltaTime)
{
    Super::Tick(DeltaTime);

    auto StdOutReadPipeHandle = GetLauncher()->GetFortniteStdOutReadPipeHandle();

    while (true)
    {
        DWORD BytesAvailable = 0;
        // PeekNamedPipe lets us see how many bytes are in the buffer without blocking
        BOOL bPeekOk = ::PeekNamedPipe(
            StdOutReadPipeHandle, // handle to the pipe
            nullptr,              // no need a local buffer to peek data
            0,                    // 0 bytes to read
            nullptr,              // bytes read
            &BytesAvailable,      // bytes available
            nullptr               // total bytes left in this message (not needed)
        );

        if (!bPeekOk)
        {
            DWORD ErrorCode = GetLastError();
            
            Log(Error, "PeekNamedPipe failed with error: {}", ErrorCode);
            
            break;
        }

        // If there are no bytes available, we're done for this tick
        if (BytesAvailable == 0)
        {
            break;
        }

        // Read only what's currently available (up to some chunk)
        constexpr DWORD MAX_CHUNK = 4096;  // arbitrary chunk size
        DWORD BytesToRead = (BytesAvailable < MAX_CHUNK) ? BytesAvailable : MAX_CHUNK;

        char Buffer[MAX_CHUNK];
        DWORD BytesRead = 0;
        BOOL bReadOk = ::ReadFile(
            StdOutReadPipeHandle,
            Buffer,
            BytesToRead,
            &BytesRead,
            nullptr
        );

        if (!bReadOk || BytesRead == 0)
        {
            DWORD ErrorCode = GetLastError();
            
            Log(Error, "ReadFile failed with error: {}", ErrorCode);
            
            break;
        }

        std::string ReadString = std::string{Buffer, BytesRead};
        
        for (const auto Char : ReadString)
        {
            std::vector<int32> LogTriggeredActionsToRemove{};
            
            for (int32 i = 0; i < static_cast<int32>(LogTriggeredActions.size()); i++)
            {
                auto& LogTriggeredAction = LogTriggeredActions[i];
                
                if (Char == LogTriggeredAction.TriggerString[LogTriggeredAction.TriggerStringCharsFound])
                {
                    LogTriggeredAction.TriggerStringCharsFound++;
                }
                else
                {
                    LogTriggeredAction.TriggerStringCharsFound = 0;
                }

                if (LogTriggeredAction.TriggerStringCharsFound >= static_cast<int32>(LogTriggeredAction.TriggerString.size()))
                {
                    LogTriggeredAction.TriggerStringCharsFound = 0;
                    NUniquePtr<NAction> Action = LogTriggeredAction.Action.NewObject(this);

                    if (LogTriggeredAction.bTriggerOnlyOnce)
                    {
                        LogTriggeredActionsToRemove.push_back(i);
                    }
                }
            }

            for (int32 i = static_cast<int32>(LogTriggeredActionsToRemove.size()) - 1; i >= 0; i--)
            {
                LogTriggeredActions[i] = LogTriggeredActions.back();
                LogTriggeredActions.pop_back();
            }

            if (bPrintFortniteLog)
            {
                AwaitingPrintString += Char;
            }
        }
    }

    if (!bPrintFortniteLog || AwaitingPrintString.empty())
    {
        return;
    }

    while (true)
    {
        auto NewLinePos = AwaitingPrintString.find('\n');
        
        if (NewLinePos == std::string::npos)
        {
            if (AwaitingPrintString.length() < MaxAwaitingPrintStringLength)
            {
                break;
            }
            
            NewLinePos = AwaitingPrintString.length() - 1;
        }

        NewLinePos++;

        std::string Line = AwaitingPrintString.substr(0, NewLinePos);
        AwaitingPrintString.erase(0, NewLinePos);

        if (!ColoredPrintPrefix.empty() && Line.contains(ColoredPrintPrefix))
        {
            LogRaw("\x1B[32m" + Line + "\x1B[0m");
        }
        else if (!bOnlyPrintLogWithColoredPrintPrefix)
        {
             LogRaw("\x1B[90m" + Line + "\x1B[0m");
        }
    }
}

void NReadFortniteLogActivity::ForwardCommandToFortniteCommand(const FCommandArguments& Args)
{
    std::string CommandRaw = Args.GetRawString();
    if (CommandRaw.empty())
    {
        return;
    }

    DWORD BytesWritten = 0;
    
    WriteFile(
        GetLauncher()->GetFortniteStdInWritePipeHandle(),
        CommandRaw.c_str(),
        static_cast<uint32>(CommandRaw.length()),
        &BytesWritten,
        nullptr
        );
}
