#pragma once

#include "Launcher/EngineObject.h"

class NAction : public NEngineObject
{
    GENERATE_BASE_H(NAction)
    
public:
    virtual void OnCreated() override;

protected:
    virtual void Execute();
    
    bool bPrintExecutingActionMessage = true;
};
