#include "Launcher/Engine.h"

int wmain(int32 ArgsNum, wchar_t* ArgsArrayPtr[])
{
    for (int32 i = 1; i < ArgsNum; i++)
    {
        ProgramLaunchArgs.emplace_back(ArgsArrayPtr[i]);
    }
    
    NUniquePtr<NEngine> FortLauncher = NEngine::StaticClass()->NewObject<NEngine>();
    
    return 0;
}
