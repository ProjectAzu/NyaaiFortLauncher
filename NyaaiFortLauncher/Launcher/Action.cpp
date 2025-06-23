#include "Action.h"

GENERATE_BASE_CPP(NAction)

void NAction::OnCreated()
{
    Super::OnCreated();

    Execute();
}

void NAction::Execute()
{
    if (bPrintExecutingActionMessage)
    {
        Log(Info, "Executing action {}.", GetClass()->GetName());
    }
}
