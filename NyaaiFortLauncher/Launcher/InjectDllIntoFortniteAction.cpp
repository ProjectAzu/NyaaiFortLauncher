#include "InjectDllIntoFortniteAction.h"

#include "FortLauncher.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

GENERATE_BASE_CPP(NInjectDllIntoFortniteAction)

void NInjectDllIntoFortniteAction::Execute()
{
    Super::Execute();

    if (!exists(DllPath) || !is_regular_file(DllPath) || DllPath.extension() != ".dll")
    {
        Log(Error, "NInjectDllIntoFortniteAction failed, bad dll path");
        return;
    }

    auto Launcher = GetLauncher();

    std::string DllPathString = DllPath.string();
    const char* DllPathCSTR = DllPathString.c_str();
    SIZE_T Length = strlen(DllPathCSTR) + 1;

    HANDLE ProcessHandle = Launcher->GetFortniteProcessHandle();
    if (!ProcessHandle)
    {
        Log(Error, "NInjectDllIntoFortniteAction, invalid fortnite process handle");
        return;
    }

    LPVOID AllocatedMemory = VirtualAllocEx(ProcessHandle, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!AllocatedMemory)
    {
        Log(Error, "NInjectDllIntoFortniteAction failed to allocate memory");
        return;
    }

    if (!WriteProcessMemory(ProcessHandle, AllocatedMemory, DllPathCSTR, Length, 0)) {
        
        Log(Error, "NInjectDllIntoFortniteAction failed to write process memory");
        return ;
    }

    auto LoadLibraryA = reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "LoadLibraryA"));

    HANDLE DllThreadHandle = CreateRemoteThread(ProcessHandle, 0, 0, LoadLibraryA, AllocatedMemory, CREATE_SUSPENDED, 0);

    VirtualFree(AllocatedMemory, Length, MEM_RELEASE);

    if (!DllThreadHandle || DllThreadHandle == INVALID_HANDLE_VALUE)
    {
        Log(Error, "NInjectDllIntoFortniteAction failed to start remote thread");
        return;
    }
    
    if (!DllThreadDescription.empty())
    {
        std::wstring WThreadDescription(DllThreadDescription.begin(), DllThreadDescription.end());

        SetThreadDescription(DllThreadHandle, WThreadDescription.c_str());

        Log(Info, "Set dll thread description to '{}'", DllThreadDescription);
    }

    ResumeThread(DllThreadHandle);

    CloseHandle(DllThreadHandle);

    Log(Info, "Injected dll '{}' into fortnite", DllPath.string());
}