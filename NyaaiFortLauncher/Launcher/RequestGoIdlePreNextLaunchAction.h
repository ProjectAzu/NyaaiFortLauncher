#pragma once

#include "Action.h"

class NRequestGoIdlePreNextLaunchAction : public NAction
{
    GENERATE_BASE_H(NRequestGoIdlePreNextLaunchAction)
    
public:
    virtual void Execute() override;
};
