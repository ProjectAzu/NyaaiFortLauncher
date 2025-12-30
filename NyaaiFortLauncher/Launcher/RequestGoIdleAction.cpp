#include "RequestGoIdleAction.h"

#include "FortLauncher.h"

GENERATE_BASE_CPP(NRequestGoIdleAction)

void NRequestGoIdleAction::Execute()
{
    Super::Execute();
    
    GetLauncher()->RequestGoIdle();
}
