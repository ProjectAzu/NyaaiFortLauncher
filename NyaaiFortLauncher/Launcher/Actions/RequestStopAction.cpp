#include "RequestStopAction.h"

#include "Launcher/Activities/FortLauncher.h"

GENERATE_BASE_CPP(NRequestStopAction)

void NRequestStopAction::Execute()
{
    Super::Execute();

    GetLauncher()->RequestStop();
}
    