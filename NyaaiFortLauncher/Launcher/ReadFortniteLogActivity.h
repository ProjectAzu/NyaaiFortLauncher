#pragma once

#include "Activity.h"

#include "Action.h"

struct FLogTriggeredAction : FStructWithProperties
{
    NPROPERTY(TriggerString)
    std::string TriggerString{};

    NPROPERTY(Action)
    FObjectInitializeTemplate<NAction> Action{};

    NPROPERTY(bTriggerOnlyOnce)
    bool bTriggerOnlyOnce = false;

    int32 TriggerStringCharsFound = 0;
};

class NReadFortniteLogActivity : public NActivity
{
    GENERATE_BASE_H(NReadFortniteLogActivity)
    
public:
    void OnCreated() override;
    void Tick(double DeltaTime) override;

    NPROPERTY(LogTriggeredActions)
    std::vector<FLogTriggeredAction> LogTriggeredActions{};

    NPROPERTY(bPrintFortniteLog)
    bool bPrintFortniteLog = true;

    NPROPERTY(ColoredPrintPrefix)
    std::string ColoredPrintPrefix{};

    NPROPERTY(bOnlyPrintLogWithColoredPrintPrefix)
    bool bOnlyPrintLogWithColoredPrintPrefix = false;

private:
    std::string AwaitingPrintString{};
    static constexpr uint32 MaxAwaitingPrintStringLength = 3000; 
};
