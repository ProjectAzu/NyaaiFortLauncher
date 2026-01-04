#include "RunCommandAction.h"

#include "Launcher/CommandManager.h"

GENERATE_BASE_CPP(NRunCommandAction)

void NRunCommandAction::Execute()
{
    Super::Execute();
    
    GetCommandManager().EnqueueCommand(Command, this);
}
