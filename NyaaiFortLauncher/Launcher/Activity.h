#pragma once
#include "LauncherObject.h"

class NActivity : public NLauncherObject
{
    GENERATE_BASE_H(NActivity)
    
public:
    void OnCreated() override;
    virtual void Tick(double DeltaTime);
    
};
