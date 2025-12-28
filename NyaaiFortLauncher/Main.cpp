#include "Object/Object.h"
#include "Launcher/FortLauncher.h"
#include "Utils/TextFileLoader.h"

#include "Utils/WindowsInclude.h"
#include <conio.h>

#include <unordered_set>

#include <curl/curl.h>

static constexpr auto ConfigFileExtension = L".nfort";

static void PrintHelp()
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
            LogRaw(std::format(L"\t{} {} = {};\n", 
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
            LogRaw(std::format(L"\t{} {} = {};\n", 
                Property->GetTypeName(),
                Property->GetName(), 
                Property->GetAsString(StructInfo.SampleObjectHolder.get()))
            );
        }
    }
    
    LogRaw(L"\n");
}

static void RunLauncher(int32 ArgsNum, wchar_t* ArgsArrayPtr[])
{
    if (ArgsNum != 2)
    {
        Log(Error, L"Please provide a {} config file as a command line argument or do -help.", ConfigFileExtension);
        return;
    }

    std::wstring ConfigPathString = ArgsArrayPtr[1];
    
    if (ConfigPathString == L"help" || ConfigPathString == L"-help" || ConfigPathString == L"--help")
    {
        PrintHelp();
        return;
    }

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

    FDefaultValueOverrides LauncherPropertySetData{};
    if (!TTypeHelpers<FDefaultValueOverrides>::SetFromString(&LauncherPropertySetData, ConfigAsWString))
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

    RunLauncher(ArgsNum, ArgsArrayPtr);

    Log(Info, L"Press any key to exit...");

    CleanupLogging();

    _getch();

    return 0;
}
