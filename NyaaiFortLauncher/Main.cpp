#ifndef NYAAIFORTLAUNCHER_STATIC

#include "Launcher/Engine.h"
#include "Launcher/DefaultEngine.h"

static NSubClassOf<NEngine> ProcessEngineClassOverride()
{
    if (ProgramLaunchArgs.empty())
    {
        return nullptr;
    }
    
    if (ProgramLaunchArgs[0] != L"-EngineClassOverride")
    {
        return nullptr;
    }
    
    ProgramLaunchArgs.erase(ProgramLaunchArgs.begin());
    
    if (ProgramLaunchArgs.empty())
    {
        return nullptr;
    }
    
    NSubClassOf<NEngine> Class{};
    TTypeHelpers<NSubClassOf<NEngine>>::SetFromString(&Class, ProgramLaunchArgs[0]);
    
    ProgramLaunchArgs.erase(ProgramLaunchArgs.begin());
    
    return Class;
}

int wmain(int32 ArgsNum, wchar_t* ArgsArrayPtr[])
{
    for (int32 i = 1; i < ArgsNum; i++)
    {
        ProgramLaunchArgs.emplace_back(ArgsArrayPtr[i]);
    }
    
    NSubClassOf<NEngine> EngineClass = NDefaultEngine::StaticClass();
    
    if (auto EngineClassOverride = ProcessEngineClassOverride())
    {
        EngineClass = EngineClassOverride;
    }
    
    NUniquePtr<NEngine> Engine = EngineClass->NewObject<NEngine>();
    
    Engine->RunTickLoop();
    
    return 0;
}

#endif