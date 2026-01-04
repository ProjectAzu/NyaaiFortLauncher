#include "Engine.h"

#include <iostream>
#include <thread>
#include <curl/curl.h>

#include "FortLauncher.h"
#include "Utils/CommandLineImplementation.h"
#include "Utils/ReplxxCommandLine.h"
#include "Utils/TextFileLoader.h"

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
    
    curl_global_init(CURL_GLOBAL_ALL);
    
    if (ShouldPrintHelpAndExit())
    {
        PrintClassesInfo();
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
        L"start",
        L"Starts a launcher instance if none are currently running.",
        &ThisClass::StartCommand
    );
    
    GetCommandManager().RegisterConsoleCommand(
        this,
        L"restart",
        L"Kills the context launcher instance and starts a new default one",
        &ThisClass::RestartCommand
    );
    
    if (auto DefaultLauncherTemplate = GetDefaultLauncherTemplate())
    {
        StartChildActivity(DefaultLauncherTemplate);   
    }
    else
    {
        Log(Info, L"GetDefaultLauncherTemplate returned nullptr. A default launcher instance will not be started.");
    }
    
    if (bUsingExternalTicking)
    {
        return;
    }
    
    auto CurrentTime = std::chrono::high_resolution_clock::now();

    while (true)
    {
        auto NewTime = std::chrono::high_resolution_clock::now();
        double DeltaTime = std::chrono::duration<double>(NewTime - CurrentTime).count();
        CurrentTime = NewTime;
        
        Tick(DeltaTime);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
}

void NEngine::OnDestroyed()
{
    Super::OnDestroyed();
    
    CleanupCommandLine();
    
    curl_global_cleanup();
}

void NEngine::Tick(double DeltaTime)
{
    Super::Tick(DeltaTime);
    
    CommandManager.ProcessManuallyQueuedCommands(this);
    CommandManager.ProcessCommands(GetCommandsContextObject());
}

bool NEngine::ShouldPrintHelpAndExit() const
{
    if (ProgramLaunchArgs.empty())
    {
        return false;
    }
    
    return ProgramLaunchArgs[0] == L"help" || ProgramLaunchArgs[0] == L"-help" || ProgramLaunchArgs[0] == L"--help";
}

TObjectTemplate<NFortLauncher> NEngine::GetDefaultLauncherTemplate() const
{
    if (ProgramLaunchArgs.empty())
    {
        return {};
    }
    
    static constexpr auto ConfigFileExtension = L".nfort";
    
    if (ProgramLaunchArgs.size() != 1)
    {
        Log(Error, L"Please provide a {} config file as a command line argument or do -help.", ConfigFileExtension);
        return {};
    }

    std::wstring ConfigPathString = ProgramLaunchArgs[0];

    std::filesystem::path ConfigPath{};
    if (!ConvertStringToCleanAbsolutePath(ConfigPathString, ConfigPath))
    {
        Log(Error, L"Could not convert command line argument '{}' into a valid path.", ConfigPathString);
        return {};
    }

    if (!ConfigPath.has_filename())
    {
        Log(Error, L"'{}' is not a path to a file.", ConfigPath.wstring());
        return {};
    }

    if (ConfigPath.extension() != ConfigFileExtension)
    {
        Log(Error, L"The config file type is '{}' instead of '{}'.",
            ConfigPath.extension().wstring(), ConfigFileExtension);
        return {};
    }

    std::wstring ConfigAsWString{};
    try
    {
        ConfigAsWString = Utils::LoadTextFileToWString(ConfigPath);
    }
    catch (...)
    {
        Log(Error, L"Could not read/convert the '{}' Config file.", ConfigPath.wstring());
        return {};
    }
    
    TObjectTemplate<NFortLauncher> LauncherTemplate{};
    if (!TTypeHelpers<TObjectTemplate<NFortLauncher>>::SetFromString(&LauncherTemplate, ConfigAsWString))
    {
        Log(Error, L"Malformatted config file '{}'.", ConfigPath.wstring());
        return {};
    }
    
    if (!LauncherTemplate.GetClass())
    {
        LauncherTemplate.ModifyClass(NFortLauncher::StaticClass());
    }
    
    return LauncherTemplate;
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
        else for (const auto Property : Properties)
        {
            LogRaw(std::format(L"\t{} {} = {{{}}};\n", 
                Property->GetTypeName(),
                Property->GetName(), 
                Property->GetAsString(DefaultObject))
                );
            
            auto InfoOfStructsUsedInType = Property->GetInfoOfStructsWithPropertiesUsedInType();
            
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
        else for (const auto Property : Properties)
        {
            LogRaw(std::format(L"\t{} {} = {{{}}};\n", 
                Property->GetTypeName(),
                Property->GetName(), 
                Property->GetAsString(StructInfo.SampleObjectHolder.get()))
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
        Log(Info, L"There are currently no running launcher instances.");
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
    Log(Info, L"Set CommandsContextLauncherId to {}.", CommandsContextLauncherId);
}

void NEngine::StartCommand(const FCommandArguments& Args)
{
    auto Launchers = GetLauncherInstances();
    if (!Launchers.empty())
    {
        Log(Error, L"There already is a launcher instance running");
        return;
    }
    
    auto Template = GetDefaultLauncherTemplate();
    if (!Template)
    {
        Log(Error, L"GetDefaultLauncherTemplate returned nullptr, can't start");
        return;
    }
    
    StartChildActivity(GetDefaultLauncherTemplate());
}

void NEngine::RestartCommand(const FCommandArguments& Args)
{
    auto Launcher = GetCommandsContextLauncher();
    if (!Launcher)
    {
        Log(Error, L"Can't restart, no context launcher.");
        return;
    }
    
    Launcher->Destroy();
    Launcher = nullptr;
    
    auto Template = GetDefaultLauncherTemplate();
    if (!Template)
    {
        Log(Error, L"GetDefaultLauncherTemplate returned nullptr, can't start");
        return;
    }
    
    StartChildActivity(GetDefaultLauncherTemplate());
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
