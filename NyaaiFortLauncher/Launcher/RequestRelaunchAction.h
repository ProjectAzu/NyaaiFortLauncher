#pragma once
#include "Action.h"

class NRequestRelaunchAction : NAction
{
    GENERATE_BASE_H(NRequestRelaunchAction)
    
public:
    void Execute() override;
};
