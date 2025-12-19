#pragma once

#include "Activity.h"
#include "Action.h"
#include "Utils/Utf8.h"

struct FCommandArguments;

struct FLogTriggeredAction : FStructWithProperties
{
    NPROPERTY(TriggerString)
    std::wstring TriggerString{};

    NPROPERTY(Action)
    FObjectInitializeTemplate<NAction> Action{};

    NPROPERTY(bTriggerOnlyOnce)
    bool bTriggerOnlyOnce = false;

    uint32 TriggerStringCharsFound = 0;
};

class NReadFortniteLogActivity : public NActivity
{
    GENERATE_BASE_H(NReadFortniteLogActivity)

public:
    void OnCreated() override;
    void Tick(double DeltaTime) override;

private:
    void ProcessLogTriggeredActions(wchar_t Char);

    void ReadAndProcessPipe(void* StdOutReadPipeHandle);
    void HandleDecodedText(const std::wstring& Text);

    void AppendToCurrentLine(wchar_t Ch);
    void FlushCurrentLine(bool bForce);

    bool ShouldPrintLine(bool bIsGreen) const;
    bool IsCurrentLineGreen() const;

    std::wstring BuildColoredLine(const std::wstring& Line, bool bIsGreen) const;

    void ForwardCommandToFortniteCommand(const FCommandArguments& Args);

public:
    NPROPERTY(LogTriggeredActions)
    std::vector<FLogTriggeredAction> LogTriggeredActions{};

    NPROPERTY(bPrintFortniteLog)
    bool bPrintFortniteLog = true;

    NPROPERTY(ColoredPrintPrefix)
    std::wstring ColoredPrintPrefix{};

    NPROPERTY(bOnlyPrintLogWithColoredPrintPrefix)
    bool bOnlyPrintLogWithColoredPrintPrefix = false;

private:
    FUtf8StreamDecoder Utf8StreamDecoder{};
    std::wstring CurrentLine{};
    std::wstring PendingOutput{};
};
