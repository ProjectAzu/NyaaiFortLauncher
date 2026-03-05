#include "RequestRelaunchAction.h"

#include "Launcher/FortLauncher.h"

GENERATE_BASE_CPP(NRequestRelaunchAction)

void NRequestRelaunchAction::Execute()
{
    Super::Execute();

    EnqueueCommand(L"restart");
}
