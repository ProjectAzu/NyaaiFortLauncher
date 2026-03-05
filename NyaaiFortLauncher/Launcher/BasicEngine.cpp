#include "BasicEngine.h"

#include "Activities/FortLauncher.h"
#include "Utils/TextFileLoader.h"
#include "Utils/WindowsInclude.h"

GENERATE_BASE_CPP(NBasicEngine)

void NBasicEngine::OnCreated()
{
    Super::OnCreated();
    
    GetCommandManager().RegisterConsoleCommand(
        this,
        L"start",
        L"Starts a launcher instance if none are currently running",
        &ThisClass::StartCommand
    );
    
    GetCommandManager().RegisterConsoleCommand(
        this,
        L"restart",
        L"Kills the context launcher instance and starts a new default one",
        &ThisClass::RestartCommand
    );
    
    if (bAutoStartLauncher)
    {
        if (LauncherTemplate)
        {
            Log(Info, L"bShouldAutoStart is true, starting a launcher instance");
            StartChildActivity(LauncherTemplate);
        }
        else
        {
            Log(Error, L"LauncherTemplate is nullptr, can't auto start");
        }
    }
}

void NBasicEngine::StartCommand(const FCommandArguments& Args)
{
    auto Launchers = GetLauncherInstances();
    if (!Launchers.empty())
    {
        Log(Error, L"There already is a launcher instance running");
        return;
    }
    
    if (!LauncherTemplate)
    {
        Log(Error, L"LauncherTemplate is nullptr, can't start");
        return;
    }
    
    StartChildActivity(LauncherTemplate);
}

void NBasicEngine::RestartCommand(const FCommandArguments& Args)
{
    auto Launcher = GetCommandsContextLauncher();
    if (!Launcher)
    {
        Log(Error, L"Can't restart, no context launcher");
        return;
    }
    
    Launcher->Destroy();
    Launcher = nullptr;
    
    if (!LauncherTemplate)
    {
        Log(Error, L"LauncherTemplate is nullptr, can't start");
        return;
    }
    
    StartChildActivity(LauncherTemplate);
}