#include "Launcher/FortLauncher.h"
#include "Object/Object.h"

#include "Utils/WindowsInclude.h"
#include <conio.h>

#include <curl/curl.h>

#include "Utils/TextFileLoader.h"

static constexpr auto ConfigFileExtension = L".nfort";

static void StartLauncher(int32 ArgsNum, wchar_t* ArgsArrayPtr[])
{
    if (ArgsNum != 2)
    {
        Log(Error, L"Please a {} config file as a command line argument.", ConfigFileExtension);
        return;
    }

    std::wstring ConfigPathString = ArgsArrayPtr[1];

    std::filesystem::path ConfigPath{};
    if (!ConvertStringToCanonicalPath(ConfigPathString, ConfigPath))
    {
        Log(Error, L"Could not convert command line argument '{}' into a valid path.", ConfigPathString);
        return;
    }

    if (!ConfigPath.has_filename())
    {
        Log(Error, L"'{}' is not a path to a file.", ConfigPath.wstring());
        return;
    }

    if (ConfigPath.extension() != ConfigFileExtension)
    {
        Log(Error, L"The config file type is '{}' instead of '{}'.",
            ConfigPath.extension().wstring(), ConfigFileExtension);
        return;
    }

    std::wstring ConfigAsWString{};
    try
    {
        ConfigAsWString = Utils::LoadTextFileToWString(ConfigPath);
    }
    catch (...)
    {
        Log(Error, L"Could not read/convert the '{}' Config file.", ConfigPath.wstring());
        return;
    }

    std::vector<FPropertySetData> LauncherPropertySetData{};
    if (!FPropertySetterFunction<std::vector<FPropertySetData>>::Set(&LauncherPropertySetData, ConfigAsWString))
    {
        Log(Error, L"Malformatted config file '{}'.", ConfigPath.wstring());
        return;
    }

    SetConsoleTitleW(ConfigPath.stem().c_str());

    Log(Info, L"Launcher config used: '{}'.", ConfigPath.wstring());

    curl_global_init(CURL_GLOBAL_ALL);

    NSubClassOf<NFortLauncher> LauncherClass = NFortLauncher::StaticClass();

    NUniquePtr<NFortLauncher> FortLauncher = LauncherClass->NewObject<NFortLauncher>(
        nullptr, LauncherPropertySetData
    );
    
    FortLauncher = nullptr;

    curl_global_cleanup();
}

int wmain(int32 ArgsNum, wchar_t* ArgsArrayPtr[])
{
    InitializeLogging();

    StartLauncher(ArgsNum, ArgsArrayPtr);

    Log(Info, L"Press any key to exit...");

    CleanupLogging();

    _getch();

    return 0;
}
