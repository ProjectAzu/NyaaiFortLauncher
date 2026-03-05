#include "RequestStopAction.h"

#include "Launcher/FortLauncher.h"

GENERATE_BASE_CPP(NRequestStopAction)

void NRequestStopAction::Execute()
{
    Super::Execute();

    GetLauncher()->RequestStop();
}
    