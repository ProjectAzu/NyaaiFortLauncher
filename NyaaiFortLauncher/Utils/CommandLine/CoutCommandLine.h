#pragma once

#include "CommandLineImplementation.h"

class NCoutCommandLine : public NCommandLine
{
    GENERATE_BASE_H(NCoutCommandLine)
    
public:
    virtual void Log(const std::wstring& Message) override;
};
