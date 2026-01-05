#pragma once
#include "Action.h"

class NRequestStopAction : public NAction
{
    GENERATE_BASE_H(NRequestStopAction)
    
public:
    virtual void Execute() override;
};
