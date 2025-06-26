#pragma once

#include "Object/Object.h"

class NAction;

class NLauncherObject : public NObject
{
    GENERATE_BASE_H(NLauncherObject)
    
public:
    class NFortLauncher* GetLauncher() const;

    virtual void OnCreated() override;
    virtual void OnDestroyed() override;
};
