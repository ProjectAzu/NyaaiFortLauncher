#include "InjectDllIntoFortniteAction.h"

#include "FortLauncher.h"

#include "Utils/WindowsInclude.h"

GENERATE_BASE_CPP(NInjectDllIntoFortniteAction)

void NInjectDllIntoFortniteAction::Execute()
{
    Super::Execute();

    if (!exists(DllPath) || !is_regular_file(DllPath) || DllPath.extension().wstring() != L".dll")
    {
        Log(Error, L"NInjectDllIntoFortniteAction failed, bad dll path");
        return;
    }

    auto Launcher = GetLauncher();

    std::wstring DllPathString = DllPath.wstring();
    const wchar_t* DllPathCSTR = DllPathString.c_str();

    // Bytes (NOT wchar count)
    const SIZE_T BytesToWrite = (DllPathString.size() + 1) * sizeof(wchar_t);

    HANDLE ProcessHandle = Launcher->GetFortniteProcessHandle();
    if (!ProcessHandle)
    {
        Log(Error, L"NInjectDllIntoFortniteAction, invalid fortnite process handle");
        return;
    }

    // Allocate exactly what we need (in bytes) in the remote process
    LPVOID AllocatedMemory = VirtualAllocEx(ProcessHandle, nullptr, BytesToWrite, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!AllocatedMemory)
    {
        Log(Error, L"NInjectDllIntoFortniteAction failed to allocate memory");
        return;
    }

    if (!WriteProcessMemory(ProcessHandle, AllocatedMemory, DllPathCSTR, BytesToWrite, nullptr))
    {
        Log(Error, L"NInjectDllIntoFortniteAction failed to write process memory");
        VirtualFreeEx(ProcessHandle, AllocatedMemory, 0, MEM_RELEASE);
        return;
    }

    // Wide version, since we're passing wchar_t*
    auto LoadLibraryWPtr = reinterpret_cast<LPTHREAD_START_ROUTINE>(
        GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW")
        );

    if (!LoadLibraryWPtr)
    {
        Log(Error, L"NInjectDllIntoFortniteAction failed to resolve LoadLibraryW");
        VirtualFreeEx(ProcessHandle, AllocatedMemory, 0, MEM_RELEASE);
        return;
    }

    HANDLE DllThreadHandle = CreateRemoteThread(ProcessHandle, nullptr, 0, LoadLibraryWPtr, AllocatedMemory, CREATE_SUSPENDED, nullptr);
    if (!DllThreadHandle || DllThreadHandle == INVALID_HANDLE_VALUE)
    {
        Log(Error, L"NInjectDllIntoFortniteAction failed to start remote thread");
        VirtualFreeEx(ProcessHandle, AllocatedMemory, 0, MEM_RELEASE);
        return;
    }

    if (!DllThreadDescription.empty())
    {
        SetThreadDescription(DllThreadHandle, DllThreadDescription.c_str());
        Log(Info, L"Set dll thread description to '{}'", DllThreadDescription);
    }

    ResumeThread(DllThreadHandle);

    // Wait so the remote thread can safely read the argument buffer before we free it
    //WaitForSingleObject(DllThreadHandle, INFINITE);

    // Remote free (MEM_RELEASE requires size = 0)
    //VirtualFreeEx(ProcessHandle, AllocatedMemory, 0, MEM_RELEASE); i dont care it can leak memory im not waiting

    CloseHandle(DllThreadHandle);

    Log(Info, L"Injected dll '{}' into fortnite", DllPathString);
}