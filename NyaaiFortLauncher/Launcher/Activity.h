#pragma once

#include "LauncherObject.h"

class NActivity : public NLauncherObject
{
    GENERATE_BASE_H(NActivity)
    
public:
    virtual void OnCreated() override;
    virtual void Tick(double DeltaTime);
    
};
