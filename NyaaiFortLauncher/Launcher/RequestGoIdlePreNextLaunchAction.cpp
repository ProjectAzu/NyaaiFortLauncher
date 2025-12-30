#include "RequestGoIdlePreNextLaunchAction.h"

#include "FortLauncher.h"

GENERATE_BASE_CPP(NRequestGoIdlePreNextLaunchAction)

void NRequestGoIdlePreNextLaunchAction::Execute()
{
    Super::Execute();
    
    GetLauncher()->RequestGoIdlePreNextLaunch();
}
