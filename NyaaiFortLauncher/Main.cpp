#include "Launcher/FortLauncher.h"
#include "Object/Object.h"

#include <fstream>
#include <thread>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <conio.h>

#include <curl/curl.h>

void FixConsoleColors()
{
    HANDLE OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    DWORD ConsoleMode = 0;
    GetConsoleMode(OutputHandle, &ConsoleMode);

    ConsoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

    SetConsoleMode(OutputHandle, ConsoleMode);   
}

static constexpr auto ConfigFileExtension = ".nfort";

static void StartLauncher(int32 ArgsNum, char* ArgsArrayPtr[])
{
    FixConsoleColors();
    
    if (ArgsNum != 2)
    {
        Log(Error, "Please a {} config file as a command line argument.", ConfigFileExtension);
        return;
    }

    std::string ConfigPathString = ArgsArrayPtr[1];
    
    std::filesystem::path ConfigPath{};
    if (!ConvertStringToCanonicalPath(ConfigPathString, ConfigPath))
    {
        Log(Error, "Could not convert command line argument '{}' into a valid path.", ConfigPathString);
        return;
    }
   
    if (!ConfigPath.has_filename())
    {
        Log(Error, "'{}' is not a path to a file.", ConfigPath.string());
        return;
    }

    if (ConfigPath.extension() != ConfigFileExtension)
    {
        Log(Error, "The config file type is '{}' instead of '{}'.", ConfigPath.extension().string(), ConfigFileExtension);
        return;
    }

    std::ifstream InFile(ConfigPath, std::ios::in | std::ios::binary);
    if (!InFile.is_open())
    {
        Log(Error, "Could not open the '{}' Config file.", ConfigPath.string());
        return;
    }

    std::ostringstream StringStream;
    StringStream << InFile.rdbuf();
    std::string ConfigAsString = StringStream.str();

    std::vector<FPropertySetData> LauncherPropertySetData{};
    if (!FPropertySetterFunction<std::vector<FPropertySetData>>::Set(&LauncherPropertySetData, ConfigAsString))
    {
        Log(Error, "Malformatted config file '{}'.", ConfigPath.string());
        return;
    }

    Log(Info, "Launcher config used: '{}'.", ConfigPath.string());
    
    curl_global_init(CURL_GLOBAL_ALL);
    
    NSubClassOf<NFortLauncher> LauncherClass = NFortLauncher::StaticClass();
    
    NUniquePtr<NFortLauncher> FortLauncher = LauncherClass->NewObject<NFortLauncher>(
        nullptr, LauncherPropertySetData
        );

    curl_global_cleanup();

    FortLauncher = nullptr;
}

int main(int32 ArgsNum, char* ArgsArrayPtr[])
{
    StartLauncher(ArgsNum, ArgsArrayPtr);

    Log(Info, "Press any key to exit...");

    _getch();

    return 0;
}
