#pragma once

#include "Action.h"

class NRequestGoIdleAction : public NAction
{
    GENERATE_BASE_H(NRequestGoIdleAction)
    
public:
    virtual void Execute() override;
};
