#include "RequestExitAction.h"

#include "FortLauncher.h"

GENERATE_BASE_CPP(NRequestExitAction)

void NRequestExitAction::Execute()
{
    Super::Execute();

    GetLauncher()->RequestExit();
}
    