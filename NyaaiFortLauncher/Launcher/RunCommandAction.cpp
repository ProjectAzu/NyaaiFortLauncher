#include "RunCommandAction.h"

GENERATE_BASE_CPP(NRunCommandAction)

void NRunCommandAction::Execute()
{
    Super::Execute();
    
    EnqueueConsoleCommand(Command);
}
