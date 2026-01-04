#include "RequestExitAction.h"

#include "Launcher/FortLauncher.h"

GENERATE_BASE_CPP(NRequestExitAction)

void NRequestExitAction::Execute()
{
    Super::Execute();

    GetLauncher()->RequestExit();
}
    