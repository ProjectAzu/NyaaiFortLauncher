#include "RequestRelaunchAction.h"

#include "Launcher/Activities/FortLauncher.h"

GENERATE_BASE_CPP(NRequestRelaunchAction)

void NRequestRelaunchAction::Execute()
{
    Super::Execute();

    EnqueueCommand(L"restart");
}
