#pragma once

#include "Action.h"

class NRunCommandAction : public NAction
{
    GENERATE_BASE_H(NRunCommandAction)
    
public:
    virtual void Execute() override;
    
    NPROPERTY(Command)
    std::wstring Command{};
};
