#pragma once
#include "Action.h"

class NRequestRelaunchAction : public NAction
{
    GENERATE_BASE_H(NRequestRelaunchAction)
    
public:
    virtual void Execute() override;
};
