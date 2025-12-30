#pragma once

#include "Activity.h"
#include "Action.h"
#include "Object/ObjectInitializeTemplate.h"
#include "Utils/Utf8.h"

struct FCommandArguments;

struct FLogTriggeredAction : FStructWithProperties
{
    STRUCT_WITH_PROPERTIES_SIMPLE_NAME_GETTER(FLogTriggeredAction)
    
    NPROPERTY(TriggerString)
    std::wstring TriggerString{};

    NPROPERTY(Action)
    TObjectInitializeTemplate<NAction> Action{};
    
    NPROPERTY(Actions)
    std::vector<TObjectInitializeTemplate<NAction>> Actions{};

    NPROPERTY(bTriggerOnlyOnce)
    bool bTriggerOnlyOnce = false;

    uint32 TriggerStringCharsFound = 0;
};

class NReadFortniteLogActivity : public NActivity
{
    GENERATE_BASE_H(NReadFortniteLogActivity)

public:
    virtual void OnCreated() override;
    virtual void Tick(double DeltaTime) override;

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
