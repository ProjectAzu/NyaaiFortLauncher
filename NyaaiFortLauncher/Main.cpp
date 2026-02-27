#ifndef NYAAIFORTLAUNCHER_STATIC

#include "Launcher/Engine.h"
#include "Launcher/DefaultEngine.h"

int wmain(int32 ArgsNum, wchar_t* ArgsArrayPtr[])
{
    InitCommandLineArgs(ArgsNum, ArgsArrayPtr);
    
    NSubClassOf<NEngine> EngineClass = NDefaultEngine::StaticClass();
    
    if (auto EngineClassOverride = GetCommandLineArgValue(L"--EngineClass"))
    {
        NSubClassOf<NEngine> Class{};
        TTypeHelpers<NSubClassOf<NEngine>>::SetFromString(&Class, EngineClassOverride.value());
        
        if (Class)
        {
            EngineClass = Class;
        }
    }
    
    NUniquePtr<NEngine> Engine = EngineClass->NewObject<NEngine>();
    
    Engine->RunTickLoop();
    
    return 0;
}

#endif