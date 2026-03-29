#include "Engine.h"

#include <iostream>
#include <thread>

#include "FortLauncher.h"

GENERATE_BASE_CPP(NEngine)

static void ApplyInvariantLocale()
{
    _configthreadlocale(_DISABLE_PER_THREAD_LOCALE);
    
    std::setlocale(LC_ALL, "C");
    
    std::locale::global(std::locale::classic());
    
    auto Locale = std::locale::classic();
    std::cin.imbue(Locale);
    std::cout.imbue(Locale);
    std::wcin.imbue(Locale);
    std::wcout.imbue(Locale);
}

void NEngine::OnCreated()
{
    Super::OnCreated();
    
    ApplyInvariantLocale();
    
    Log(Info, L"Starting {}", GetClass()->GetName());
    
    GetCommandManager().RegisterConsoleCommand(
        this,
        L"help",
        L"Prints available commands",
        &ThisClass::HelpCommand
    );
    
    GetCommandManager().RegisterConsoleCommand(
        this,
        L"ListLauncherInstances",
        L"List all running launcher instances",
        &ThisClass::PrintLauncherInstancesListCommand
    );
    
    GetCommandManager().RegisterConsoleCommand(
        this,
        L"SetContextLauncherId",
        L"Sets the context launcher id for commands",
        &ThisClass::SetCommandsContextLauncherIdCommand
    );
    
    GetCommandManager().RegisterConsoleCommand(
        this,
        L"exit",
        L"Exits the engine",
        &ThisClass::ExitCommand
    );

    GetCommandManager().RegisterConsoleCommand(
        this,
        L"ExecuteActionOnEngine",
        L"Usage: ExecuteActionOnEngine {ActionTemplate}",
        &ThisClass::ExecuteActionOnEngineCommand
    );
    
    Log(Info, L"Running on engine init actions");
    
    for (const auto& ActionTemplate : OnEngineInitActions)
    {
        NUniquePtr<NAction> Action = ActionTemplate.NewObject(this);
    }
    
    Log(Info, L"Starting default activities");
    
    for (const auto& ActivityTemplate : Activities)
    {
        StartChildActivity(ActivityTemplate);
    }
}

void NEngine::RunTickLoop()
{
    auto CurrentTime = std::chrono::high_resolution_clock::now();

    while (!ShouldEngineExit())
    {
        auto NewTime = std::chrono::high_resolution_clock::now();
        double DeltaTime = std::chrono::duration<double>(NewTime - CurrentTime).count();
        CurrentTime = NewTime;
        
        Tick(DeltaTime);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void NEngine::Tick(double DeltaTime)
{
    Super::Tick(DeltaTime);
    
    CommandManager.ProcessManuallyQueuedCommands(this);
    CommandManager.ProcessCommands(GetCommandsContextObject());
}

bool NEngine::ShouldEngineExit() const
{
    return IsCommandLineRequestingProgramExit() || bWantsToExit;
}

void NEngine::NotifyLauncherBeingDestroyed(NFortLauncher* Launcher)
{
}

NFortLauncher* NEngine::GetCommandsContextLauncher() const
{
    auto Launchers = GetLauncherInstances();
    
    NFortLauncher* CommandsContextLauncher = nullptr;
    if (CommandsContextLauncherId >= 0)
    {
        for (const auto Launcher : Launchers)
        {
            if (Launcher->GetLauncherId() == CommandsContextLauncherId)
            {
                CommandsContextLauncher = Launcher;
                break;
            }
        }
        
        if (!CommandsContextLauncher)
        {
            Log(Error, L"Bad CommandsContextLauncherId");
        }
    }
    else if (Launchers.size() == 1)
    {
        CommandsContextLauncher = Launchers[0];
    }
    
    return CommandsContextLauncher;
}

NEngineObject* NEngine::GetCommandsContextObject() const
{
    if (auto Launcher = GetCommandsContextLauncher())
    {
        return Launcher;
    }

    return const_cast<NEngine*>(this);
}

void NEngine::HelpCommand(const FCommandArguments& Args)
{
    auto LauncherContext = GetCommandsContextLauncher();
    
    Log(Info, L"Current commands launcher context: '{}'",
        LauncherContext ? std::format(L"LauncherId: {}", LauncherContext->GetLauncherId()) : L"none"
        );
    
    Log(Info, L"Available commands:");

    for (const auto& RegisteredCommand : CommandManager.GetRegisteredCommandsForContext(GetCommandsContextObject()))
    {
        Log(
            Info,
            L"{}: '{}' - {}",
            RegisteredCommand.OwningObject->GetClass()->GetName(),
            RegisteredCommand.Command,
            RegisteredCommand.Description
        );
    }
}

void NEngine::PrintLauncherInstancesListCommand(const FCommandArguments& Args)
{
    auto Launchers = GetLauncherInstances();

    if (Launchers.empty())
    {
        Log(Info, L"There are currently no running launcher instances");
        return;
    }
    
    Log(Info, L"Launcher instances:");
    for (const auto Launcher : Launchers)
    {
        Log(Info, L"'{}' Id: {}", Launcher->GetClass()->GetName(), Launcher->GetLauncherId());
    }
}

void NEngine::SetCommandsContextLauncherIdCommand(const FCommandArguments& Args)
{
    CommandsContextLauncherId = Args.GetArgumentAtIndex<int32>(0);
    Log(Info, L"Set CommandsContextLauncherId to {}", CommandsContextLauncherId);
}

void NEngine::ExitCommand(const FCommandArguments& Args)
{
    Log(Info, L"Requested engine exit");
    bWantsToExit = true;
}

void NEngine::ExecuteActionOnEngineCommand(const FCommandArguments& Args)
{
    auto ActionTemplate = Args.GetRawStringAsType<TObjectTemplate<NAction>>();

    if (!ActionTemplate)
    {
        Log(Error, L"Specify an action template");
        return;
    }

    auto Action = ActionTemplate.NewObject(this);
}

void NEngine::NotifyObjectDestroyed(NEngineObject* Object)
{
    CommandManager.NotifyObjectDestroyed(Object);
}

FCommandManager& NEngine::GetCommandManager() const
{
    return const_cast<NEngine*>(this)->CommandManager;
}

FSaveRecordsSystem& NEngine::GetSaveRecordsSystem() const
{
    return const_cast<NEngine*>(this)->SaveRecordsSystem;
}

std::vector<NFortLauncher*> NEngine::GetLauncherInstances() const
{
    std::vector<NFortLauncher*> Launchers{};
    
    for (const auto& Activity : GetChildActivitiesIncludingNested())
    {
        if (auto Launcher = Cast<NFortLauncher>(Activity))
        {
            Launchers.push_back(Launcher);
        }
    }
    
    return Launchers;
}
