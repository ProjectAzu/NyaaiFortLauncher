#include "Utils/WindowsInclude.h"
#include "Launcher/Engine.h"
#include "Utils/TextFileLoader.h"
#include "Utils/CommandLine/CommandLineImplementation.h"
#include "Utils/CommandLine/ReplxxCommandLine.h"
#include <conio.h>

#include "Utils/FileSystem.h"

static void PrintClassesInfo()
{
    Log(Info, L"Printing list of classes and their properties with default values:");
    
    std::unordered_set<std::wstring> SetOfEncounteredStructTypeNames{};
    std::vector<FInfoOfStructWithPropertiesUsedInType> InfoOfEncounteredStructs{};
    
    for (const auto Class : NObject::StaticClass()->GetAllDerivedClasses())
    {
        LogRaw(L"\n");
        
        if (Class->GetSuper())
        {
            LogRaw(std::format(L"class {} : {}\n", Class->GetName(), Class->GetSuper()->GetName()));   
        }
        else
        {
            LogRaw(std::format(L"class {}\n", Class->GetName()));
        }
        
        LogRaw(L"{\n");
        
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
        
        LogRaw(L"}\n");
    }
    
    LogRaw(L"\n");
    
    Log(Info,L"Printing list of encountered structs and their properties with default values:");
    
    for (const auto& StructInfo : InfoOfEncounteredStructs)
    {
        LogRaw(L"\n");
        LogRaw(std::format(L"struct {}\n", StructInfo.TypeName));
        
        LogRaw(L"{\n");
        
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
        
        LogRaw(L"}\n");
    }
    
    LogRaw(L"\n");
}

static bool RunEngine()
{
    static constexpr auto ConfigFileExtension = L".nfort";
    
    std::optional<std::wstring> FirstCommandLinePositionalArg = GetCommandLinePositionalArg(0);
    if (!FirstCommandLinePositionalArg)
    {
        Log(Error, L"Please provide a path to a {} config file as the first command line positional argument or do --help", ConfigFileExtension);
        return false;
    }
    
    std::wstring ConfigPathString = FirstCommandLinePositionalArg.value();
    
    std::filesystem::path ConfigPath{};
    if (!Utils::ConvertPathToAbsolutePath(ConfigPathString, {}, ConfigPath))
    {
        Log(Error, L"Could not convert command line argument '{}' into a config path", ConfigPathString);
        return false;
    }
    
    auto ConfigFilename = ConfigPath.filename().wstring();
    
    if (!ConfigPath.has_filename())
    {
        Log(Error, L"'{}' is not a path to a file", ConfigPath.wstring());
        return false;
    }

    if (ConfigPath.extension() != ConfigFileExtension)
    {
        Log(Error, L"The config file type is '{}' instead of '{}'",
            ConfigPath.extension().wstring(), ConfigFileExtension);
        return false;
    }
    
    std::wstring ConfigAsWString{};
    try
    {
        ConfigAsWString = Utils::LoadTextFileToWString(ConfigPath);
    }
    catch (...)
    {
        Log(Error, L"Could not read/convert the '{}' Config file", ConfigPath.wstring());
        return false;
    }
    
    TObjectTemplate<NEngine> EngineTemplate{};
    if (!TTypeHelpers<TObjectTemplate<NEngine>>::SetFromString(&EngineTemplate, ConfigAsWString))
    {
        Log(Error, L"Malformatted config file '{}'", ConfigPath.wstring());
        return false;
    }
    
    if (!EngineTemplate)
    {
        Log(Error, L"EngineTemplate is nullptr, can't start");
        return false;
    }
    
    std::wstring ConfigFileName = ConfigPath.filename().wstring();
    
    SetConsoleTitleW(ConfigFileName.c_str());
    
    auto Engine = EngineTemplate.NewObject(nullptr, true);
    
    if (Engine->GetSaveRecordsSystem().SaveFileName.empty())
    {
        Engine->GetSaveRecordsSystem().SaveFileName = ConfigFileName + L".SaveRecords";   
    }
    
    Engine->FinishConstruction();
    
    Engine->RunTickLoop();
    
    return true;
}

int wmain(int32 ArgsNum, wchar_t* ArgsArrayPtr[])
{
    InitCommandLineArgs(ArgsNum, ArgsArrayPtr);
    
    NSubClassOf<NCommandLine> CommandLineClass = NReplxxCommandLine::StaticClass();
    if (auto CommandLineClassOverride = GetCommandLineArgValue(L"--CommandLineClass"))
    {
        NSubClassOf<NCommandLine> Class{};
        if (TTypeHelpers<NSubClassOf<NCommandLine>>::SetFromString(&Class, CommandLineClassOverride.value()))
        {
            CommandLineClass = Class;
        }
    }
    
    InitCommandLine(CommandLineClass);
    
    if (HasCommandLineArg(L"-h") || 
        HasCommandLineArg(L"--help") || 
        HasCommandLineArg(L"-help") ||
        (GetCommandLinePositionalArg(0) && GetCommandLinePositionalArg(0).value() == L"help"))
    {
        PrintClassesInfo();
        
        CleanupCommandLine();
        
        return 0;
    }
    
    if (!RunEngine())
    {
        Log(Info, L"Press any key to exit...");
        
        CleanupCommandLine();
        
        _getch();
        
        return 0;
    }
    
    CleanupCommandLine();
    
    return 0;
}