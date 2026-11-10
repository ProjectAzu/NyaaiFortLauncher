module;
#include "Object/ReflectionMacros.h"

export module ReadFortniteLogActivity;

export import Engine;

export struct FLogTriggeredAction
{
    ReflectedClassBase(FLogTriggeredAction)

public:

    ReflectedMember(TriggerString)
    std::wstring TriggerString{};

    ReflectedMember(Action)
    TObjectTemplate<NAction> Action{};

    ReflectedMember(Actions)
    std::vector<TObjectTemplate<NAction>> Actions{};

    ReflectedMember(bTriggerOnlyOnce)
    bool bTriggerOnlyOnce = false;

    uint32 TriggerStringCharsFound = 0;
};

export class NReadFortniteLogActivity : public NActivity
{
    ObjectBase(NReadFortniteLogActivity)

public:
    virtual void OnCreated() override;
    virtual void OnDestroyed() override;
    virtual void Tick(double DeltaTime) override;

private:
    void ProcessLogTriggeredActions(wchar_t Char);

    void ReadAndProcessPipe(void* StdOutReadPipeHandle);
    void HandleDecodedText(const std::wstring& Text);

    void AppendToCurrentLine(wchar_t Ch);
    void FlushCurrentLine(bool bForce);

    bool ShouldPrintLine(bool bIsGreen) const;
    bool IsCurrentLineColored() const;

    std::wstring BuildColoredLine(const std::wstring& Line, bool bIsColored) const;

    void ForwardCommandToFortniteCommand(const FCommandArguments& Args);

public:
    ReflectedMember(LogTriggeredActions)
    std::vector<FLogTriggeredAction> LogTriggeredActions{};

    ReflectedMember(bPrintFortniteLog)
    bool bPrintFortniteLog = true;

    ReflectedMember(bReadUtf16)
    bool bReadUtf16 = false;

    ReflectedMember(ColoredPrintPrefix)
    std::wstring ColoredPrintPrefix{};

    ReflectedMember(bOnlyPrintLogWithColoredPrintPrefix)
    bool bOnlyPrintLogWithColoredPrintPrefix = false;

private:
    FUtf8StreamDecoder Utf8StreamDecoder{};
    FUtf16StreamDecoder Utf16StreamDecoder{};
    bool bPipeClosed = false;
    std::wstring CurrentLine{};
    std::wstring PendingOutput{};
};
