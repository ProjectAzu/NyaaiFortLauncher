#pragma once

#include "Action.h"

class NPrintLogAction : NAction
{
    GENERATE_BASE_H(NPrintLogAction)
    
public:
    NPrintLogAction();
    
    void Execute() override;

    NPROPERTY(Message)
    std::wstring Message{};

    NPROPERTY(LogLevel)
    std::wstring LogLevel = L"Info";
};
