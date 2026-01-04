#pragma once
#include "Action.h"

class NRequestExitAction : public NAction
{
    GENERATE_BASE_H(NRequestExitAction)
    
public:
    virtual void Execute() override;
};
