#pragma once

#include "Action.h"

class NPrintLogAction : public NAction
{
    GENERATE_BASE_H(NPrintLogAction)
    
public:
    NPrintLogAction();
    
    virtual void Execute() override;

    NPROPERTY(Message)
    std::wstring Message{};

    NPROPERTY(LogLevel)
    std::wstring LogLevel = L"Info";
};
