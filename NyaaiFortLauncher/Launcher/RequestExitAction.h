#pragma once
#include "Action.h"

class NRequestExitAction : NAction
{
    GENERATE_BASE_H(NRequestExitAction)
    
public:
    void Execute() override;
};
