#include "LauncherObject.h"

#include "FortLauncher.h"

GENERATE_BASE_CPP(NLauncherObject)

void NLauncherObject::OnCreated()
{
    Super::OnCreated();

    GetLauncher()->NotifyObjectCreated(this);
}

void NLauncherObject::OnDestroyed()
{
    Super::OnDestroyed();

    GetLauncher()->NotifyObjectDestroyed(this);
}

class NFortLauncher* NLauncherObject::GetLauncher() const
{
    for (NObject* Object = const_cast<NLauncherObject*>(this); Object; Object = Object->GetOuter())
    {
        if (auto Launcher = Cast<NFortLauncher>(Object))
        {
            return Launcher;
        }
    }

    Log(Error, L"Failed to get launcher from outer chain.");
    
    return nullptr;
}
