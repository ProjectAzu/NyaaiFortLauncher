#pragma once

#include "Action.h"

class NPrintLogAction : NAction
{
    GENERATE_BASE_H(NPrintLogAction)
    
public:
    NPrintLogAction();
    
    void Execute() override;

    NPROPERTY(Message)
    std::string Message{};

    NPROPERTY(LogLevel)
    std::string LogLevel{};
};
