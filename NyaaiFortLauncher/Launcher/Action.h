#pragma once

#include "LauncherObject.h"

class NAction : public NLauncherObject
{
    GENERATE_BASE_H(NAction)
    
public:
    virtual void OnCreated() override;

protected:
    virtual void Execute();
    
    bool bPrintExecutingActionMessage = true;
};
