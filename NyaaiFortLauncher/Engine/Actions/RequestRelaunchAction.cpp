module RequestRelaunchAction;

void NRequestRelaunchAction::Execute()
{
    Super::Execute();

    EnqueueCommand(L"restart");
}
