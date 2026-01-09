#include "Launcher/Engine.h"

#ifndef NYAAIFORTLAUNCHER_STATIC

int wmain(int32 ArgsNum, wchar_t* ArgsArrayPtr[])
{
    for (int32 i = 1; i < ArgsNum; i++)
    {
        ProgramLaunchArgs.emplace_back(ArgsArrayPtr[i]);
    }
    
    NUniquePtr<NEngine> Engine = NEngine::StaticClass()->NewObject<NEngine>();
    
    return 0;
}

#endif