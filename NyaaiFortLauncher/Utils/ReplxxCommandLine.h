#pragma once

#include "CommandLineImplementation.h"

class NReplxxCommandLine : public NCommandLine
{
    GENERATE_BASE_H(NReplxxCommandLine)
    
public:
    virtual void OnCreated() override;
    virtual void OnDestroyed() override;
    
    virtual void Log(const std::wstring& Message) override;
    virtual bool ShouldProgramExit() const override;
};
