module RequestStopAction;

void NRequestStopAction::Execute()
{
    Super::Execute();

    GetLauncher()->RequestStop();
}
