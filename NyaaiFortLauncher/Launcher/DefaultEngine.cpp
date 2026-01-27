#include "DefaultEngine.h"

#include "FortLauncher.h"
#include "Utils/TextFileLoader.h"
#include "Utils/WindowsInclude.h"

GENERATE_BASE_CPP(NDefaultEngine)

void NDefaultEngine::OnCreated()
{
    Super::OnCreated();
    
    static constexpr auto ConfigFileExtension = L".nfort";
    
    if (ProgramLaunchArgs.size() != 1)
    {
        Log(Error, L"Launch args num != 1, Please provide a {} config file as a command line argument or do -help", ConfigFileExtension);
        return;
    }
    
    std::wstring ConfigPathString = ProgramLaunchArgs[0];
    
    if (!ConvertStringToCleanAbsolutePath(ConfigPathString, ConfigPath))
    {
        Log(Error, L"Could not convert command line argument '{}' into a config path", ConfigPathString);
        return;
    }
    
    auto ConfigFilename = ConfigPath.filename().wstring();
    
    if (!ConfigPath.has_filename())
    {
        Log(Error, L"'{}' is not a path to a file", ConfigPath.wstring());
        return;
    }

    if (ConfigPath.extension() != ConfigFileExtension)
    {
        Log(Error, L"The config file type is '{}' instead of '{}'",
            ConfigPath.extension().wstring(), ConfigFileExtension);
        return;
    }
    
    SetConsoleTitleW(ConfigPath.filename().wstring().c_str());
    
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
    
    if (auto LauncherTemplate = GetLauncherTemplateFromConfig())
    {
        StartChildActivity(LauncherTemplate);
    }
    else
    {
        Log(Error, L"GetLauncherTemplateFromConfig returned nullptr, can't start");
    }
}

TObjectTemplate<NFortLauncher> NDefaultEngine::GetLauncherTemplateFromConfig() const
{
    std::wstring ConfigAsWString{};
    try
    {
        ConfigAsWString = Utils::LoadTextFileToWString(ConfigPath);
    }
    catch (...)
    {
        Log(Error, L"Could not read/convert the '{}' Config file", ConfigPath.wstring());
        return {};
    }
    
    TObjectTemplate<NFortLauncher> LauncherTemplate{};
    if (!TTypeHelpers<TObjectTemplate<NFortLauncher>>::SetFromString(&LauncherTemplate, ConfigAsWString))
    {
        Log(Error, L"Malformatted config file '{}'", ConfigPath.wstring());
        return {};
    }
    
    return LauncherTemplate;
}

void NDefaultEngine::StartCommand(const FCommandArguments& Args)
{
    auto Launchers = GetLauncherInstances();
    if (!Launchers.empty())
    {
        Log(Error, L"There already is a launcher instance running");
        return;
    }
    
    auto Template = GetLauncherTemplateFromConfig();
    if (!Template)
    {
        Log(Error, L"GetLauncherTemplateFromConfig returned nullptr, can't start");
        return;
    }
    
    StartChildActivity(GetLauncherTemplateFromConfig());
}

void NDefaultEngine::RestartCommand(const FCommandArguments& Args)
{
    auto Launcher = GetCommandsContextLauncher();
    if (!Launcher)
    {
        Log(Error, L"Can't restart, no context launcher");
        return;
    }
    
    Launcher->Destroy();
    Launcher = nullptr;
    
    auto Template = GetLauncherTemplateFromConfig();
    if (!Template)
    {
        Log(Error, L"GetDefaultLauncherTemplate returned nullptr, can't start");
        return;
    }
    
    StartChildActivity(Template);
}