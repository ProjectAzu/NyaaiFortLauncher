#include "Engine.h"

#include <iostream>
#include <thread>
#include <curl/curl.h>

#include "FortLauncher.h"
#include "Utils/CommandLine/ReplxxCommandLine.h"

GENERATE_BASE_CPP(NEngine)

NEngine::NEngine()
{
    CommandLineTemplate = NReplxxCommandLine::StaticClass();
}

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
    
    InitCommandLine(CommandLineTemplate);
    
    Log(Info, L"Starting {}", GetClass()->GetName());
    
    curl_global_init(CURL_GLOBAL_ALL);
    
    if (ShouldPrintHelpAndExit())
    {
        PrintClassesInfo();
        RequestExit();
        return;
    }
    
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
    
    Log(Info, L"Running on engine init actions");
    
    for (const auto& ActionTemplate : OnEngineInitActions)
    {
        NUniquePtr<NAction> Action = ActionTemplate.NewObject(this);
    }
    
    Log(Info, L"Starting default activities");
    
    for (const auto& ActivityTemplate : DefaultActivities)
    {
        StartChildActivity(ActivityTemplate);
    }
}

void NEngine::OnDestroyed()
{
    Super::OnDestroyed();
    
    CleanupCommandLine();
    
    curl_global_cleanup();
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
    return ShouldProgramExit() || bWantsToExit;
}

void NEngine::NotifyLauncherBeingDestroyed(NFortLauncher* Launcher)
{
}

bool NEngine::ShouldPrintHelpAndExit() const
{
    if (ProgramLaunchArgs.empty())
    {
        return false;
    }
    
    return ProgramLaunchArgs[0] == L"help" || ProgramLaunchArgs[0] == L"-help" || ProgramLaunchArgs[0] == L"--help";
}

void NEngine::PrintClassesInfo()
{
    Log(Info, L"Printing list of classes and their properties with default values:");
    
    std::unordered_set<std::wstring> SetOfEncounteredStructTypeNames{};
    std::vector<FInfoOfStructWithPropertiesUsedInType> InfoOfEncounteredStructs{};
    
    for (const auto Class : NObject::StaticClass()->GetAllDerivedClasses())
    {
        LogRaw(L"\n");
        LogRaw(std::format(L"{}:\n", Class->GetName()));
        
        auto DefaultObject = Class->GetDefaultObject();
        const auto& Properties = DefaultObject->GetPropertiesArrayConstRef();
        
        if (Properties.empty())
        {
            LogRaw(L"\tThis class has no properties\n");
        }
        else for (const auto& Property : Properties)
        {
            LogRaw(std::format(L"\t{} {} = {{{}}};\n", 
                Property.GetTypeName(),
                Property.GetName(), 
                Property.GetAsString(DefaultObject))
                );
            
            auto InfoOfStructsUsedInType = Property.GetInfoOfStructsWithPropertiesUsedInType();
            
            for (auto& Info : InfoOfStructsUsedInType)
            {
                if (SetOfEncounteredStructTypeNames.contains(Info.TypeName))
                {
                    continue;
                }
                
                SetOfEncounteredStructTypeNames.insert(Info.TypeName);
                
                InfoOfEncounteredStructs.emplace_back(std::move(Info));
            }
        }
    }
    
    LogRaw(L"\n");
    
    Log(Info,L"Printing list of encountered structs and their properties with default values:");
    
    for (const auto& StructInfo : InfoOfEncounteredStructs)
    {
        LogRaw(L"\n");
        LogRaw(std::format(L"{}:\n", StructInfo.TypeName));
        
        const auto& Properties = StructInfo.SampleObjectHolder->GetPropertiesArrayConstRef();
        
        if (Properties.empty())
        {
            LogRaw(L"\tThis struct has no properties\n");
        }
        else for (const auto& Property : Properties)
        {
            LogRaw(std::format(L"\t{} {} = {{{}}};\n", 
                Property.GetTypeName(),
                Property.GetName(), 
                Property.GetAsString(StructInfo.SampleObjectHolder.get()))
            );
        }
    }
    
    LogRaw(L"\n");
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

void NEngine::NotifyObjectDestroyed(NEngineObject* Object)
{
    CommandManager.NotifyObjectDestroyed(Object);
}

FCommandManager& NEngine::GetCommandManager() const
{
    return const_cast<NEngine*>(this)->CommandManager;
}

std::vector<NFortLauncher*> NEngine::GetLauncherInstances() const
{
    std::vector<NFortLauncher*> Launchers{};
    
    for (const auto& Activity : GetChildActivities())
    {
        if (auto Launcher = Cast<NFortLauncher>(Activity))
        {
            Launchers.push_back(Launcher);
        }
    }
    
    return Launchers;
}
