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
        Log(Info, L"Executing action {}", GetClass()->GetName());
    }
}
