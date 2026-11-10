module RunCommandAction;

void NRunCommandAction::Execute()
{
    Super::Execute();

    GetCommandManager().EnqueueCommand(Command, this);
}
