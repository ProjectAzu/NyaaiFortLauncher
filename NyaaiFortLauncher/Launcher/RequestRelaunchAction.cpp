#include "RequestRelaunchAction.h"

#include "FortLauncher.h"

GENERATE_BASE_CPP(NRequestRelaunchAction)

void NRequestRelaunchAction::Execute()
{
    Super::Execute();

    GetLauncher()->RequestRelaunch();
}
