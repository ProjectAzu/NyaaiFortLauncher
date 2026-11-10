module Engine;

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
