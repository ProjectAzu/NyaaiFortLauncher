#include "LauncherObject.h"

#include "FortLauncher.h"

GENERATE_BASE_CPP(NLauncherObject)

class NFortLauncher* NLauncherObject::GetLauncher() const
{
    for (NObject* Object = const_cast<NLauncherObject*>(this); Object; Object = Object->GetOuter())
    {
        if (auto Launcher = Cast<NFortLauncher>(Object))
        {
            return Launcher;
        }
    }

    Log(Error, "Failed to get launcher from outer chain.");
    
    return nullptr;
}