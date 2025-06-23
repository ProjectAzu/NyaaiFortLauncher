#include "FortLauncher.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <tlhelp32.h>
#include <queue>

#include "Activity.h"
#include "CreateProcessAction.h"

GENERATE_BASE_CPP(NFortLauncher)

void NFortLauncher::OnCreated()
{
    Super::OnCreated();
    
    Log(Info, "Launcher starting");
    
    Log(Info, "Fortnite exe path: {}", FortniteExePath.string());
    if (!exists(FortniteExePath) || !is_regular_file(FortniteExePath) || FortniteExePath.extension() != ".exe")
    {
        Log(Error, "The fortnite exe path is not valid.");
        return;
    }
    
    Log(Info, "Fortnite launch arguments: {}", FortniteLaunchArguments);

    if (!RunLauncher())
    {
        Log(Error, "Failed to run launcher, exiting.");
        return;
    }

    while (bWantsToRelaunch && !bWantsToExit)
    {
        bWantsToRelaunch = false;

        Log(Info, "Relaunching");

        DoCleanup();

        if (!RunLauncher())
        {
            Log(Error, "Failed to relaunch, exiting.");
            return;
        }
    }
}

void NFortLauncher::OnDestroyed()
{
    Super::OnDestroyed();

    DoCleanup();
}

void NFortLauncher::RequestRelaunch()
{
    Log(Info, "Requested relaunch.");

    bWantsToRelaunch = true;
}

void NFortLauncher::RequestExit()
{
    Log(Info, "Requested exit.");

    bWantsToExit = true;
}

bool NFortLauncher::RunLauncher()
{
    Log(Info, "Running pre fortnite launch actions");
    for (const auto& ActionTemplate : PreFortniteLaunchActions)
    {
        NUniquePtr<NAction> Action = ActionTemplate.NewObject(this);
    }

    if (!LaunchFortniteProcess())
    {
        return false;
    }

    Log(Info, "Running post fortnite launch actions");
    for (const auto& ActionTemplate : PostFortniteLaunchActions)
    {
        NUniquePtr<NAction> Action = ActionTemplate.NewObject(this);
    }

    bHasFinishedBaseLaunch = true;
    Log(Info, "Finished base launch!");
    
    Log(Info, "Spawning activities");
    
    std::vector<NUniquePtr<NActivity>> ActivitiesSpawned{};
    for (const auto& ActivityTemplate : Activities)
    {
        ActivitiesSpawned.emplace_back(ActivityTemplate.NewObject(this));
    }

    auto CurrentTime = std::chrono::high_resolution_clock::now();

    while (true)
    {
        auto NewTime = std::chrono::high_resolution_clock::now();
        double DeltaTime = std::chrono::duration<double>(NewTime - CurrentTime).count();
        CurrentTime = NewTime;
        
        for (const auto& Activity : ActivitiesSpawned)
        {
            Activity->Tick(DeltaTime);
        }

        if (bWantsToRelaunch || bWantsToExit)
        {
            return true;
        }

        if (WaitForSingleObject(FortniteProcessHandle, 100) != WAIT_TIMEOUT)
        {
            break;
        }
    }

    return true;
}

bool NFortLauncher::LaunchFortniteProcess()
{
    HANDLE StdOutWriteHandle = nullptr;

    SECURITY_ATTRIBUTES SeciurityAtributes{};
    SeciurityAtributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    SeciurityAtributes.lpSecurityDescriptor = nullptr;
    SeciurityAtributes.bInheritHandle = TRUE;

    if (!CreatePipe(&FortniteStdOutReadPipeHandle, &StdOutWriteHandle, &SeciurityAtributes, 0))
    {
        Log(Error, "Failed to create pipe");
        return false;
    }

    if (!SetHandleInformation(FortniteStdOutReadPipeHandle, HANDLE_FLAG_INHERIT, 0))
    {
        Log(Error, "Failed to set handle information");
        
        CloseHandle(StdOutWriteHandle);
        CloseHandle(FortniteStdOutReadPipeHandle);
        
        return false;
    }
    
    NUniquePtr<NCreateProcessAction> CreateFortniteProcessAction =
        NCreateProcessAction::StaticClass()->NewObject<NCreateProcessAction>(this, {}, true);

    CreateFortniteProcessAction->bReturnProcessHandle = true;
    
    CreateFortniteProcessAction->StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    CreateFortniteProcessAction->StartupInfo.hStdOutput = StdOutWriteHandle;
    CreateFortniteProcessAction->StartupInfo.hStdError = StdOutWriteHandle;
    
    CreateFortniteProcessAction->FilePath = FortniteExePath;
    CreateFortniteProcessAction->LaunchArguments = FortniteLaunchArguments;
    
    CreateFortniteProcessAction->FinishConstruction();

    CloseHandle(StdOutWriteHandle);

    if (!CreateFortniteProcessAction->bResultWasSuccess)
    {
        CloseHandle(FortniteStdOutReadPipeHandle);
        
        return false;
    }

    FortniteProcessHandle = CreateFortniteProcessAction->ResultProcessHandle;

    return true;
}

void NFortLauncher::DoCleanup()
{
    Log(Info, "Cleaning up");
    
    bHasFinishedBaseLaunch = false;
    
    if (FortniteProcessHandle)
    {
        CloseHandle(FortniteProcessHandle);
        FortniteProcessHandle = nullptr;
    }

    if (FortniteStdOutReadPipeHandle)
    {
        CloseHandle(FortniteStdOutReadPipeHandle);
        FortniteStdOutReadPipeHandle = nullptr;
    }
    
    KillAllChildProcesses();
}

void NFortLauncher::KillAllChildProcesses()
{
    Log(Info, "Killing child processes");
    
    DWORD CurrentPID = GetCurrentProcessId();

    // 1) Take a snapshot of all processes
    HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Snapshot == INVALID_HANDLE_VALUE)
    {
        Log(Error, "CreateToolhelp32Snapshot failed (Error code: {})", GetLastError());
        return;
    }

    // 2) Iterate through the snapshot to build a parent->children map
    std::vector<PROCESSENTRY32> Processes;
    Processes.reserve(256);

    PROCESSENTRY32 pe32 = {};
    pe32.dwSize = sizeof(pe32);

    if (!Process32First(Snapshot, &pe32))
    {
        CloseHandle(Snapshot);
        Log(Error, "Process32First failed (Error code: {})", GetLastError());
        return;
    }

    do 
    {
        Processes.push_back(pe32);
    }
    while (Process32Next(Snapshot, &pe32));

    CloseHandle(Snapshot);

    // We'll store the relationships in a map from ParentPID -> vector of ChildPIDs
    // For quick lookup, we can use an std::unordered_map<DWORD, std::vector<DWORD>>
    // but here we can just use a simple local lambda that finds children on demand.

    // 3) Traverse all descendants of CurrentPID
    std::queue<DWORD> Queue;
    std::vector<DWORD> AllDescendants; // store to kill later or as we go

    // Start with the current process's PID
    Queue.push(CurrentPID);

    while (!Queue.empty())
    {
        DWORD ParentPID = Queue.front();
        Queue.pop();

        // Find children of 'ParentPID' in our list
        for (const auto& proc : Processes)
        {
            // Skip conhost.exe so we don't kill our console
            if (std::wstring(proc.szExeFile) == L"conhost.exe")
            {
                continue;
            }

            if (proc.th32ParentProcessID == ParentPID)
            {
                DWORD ChildPID = proc.th32ProcessID;
                // Avoid the case of re-adding our own PID in weird edge cases
                if (ChildPID != CurrentPID)
                {
                    Queue.push(ChildPID);
                    AllDescendants.push_back(ChildPID);
                }
            }
        }
    }

    // 4) Kill each descendant
    for (DWORD pid : AllDescendants)
    {
        HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (hProc == nullptr)
        {
            Log(Error, 
                "Failed to open child process PID {} for termination (Error code: {})",
                pid, GetLastError());
            continue;
        }

        // Attempt to kill it
        if (!TerminateProcess(hProc, 1))
        {
            Log(Error, 
                "Failed to terminate child process PID {} (Error code: {})", 
                pid, GetLastError());
        }
        else
        {
            Log(Info, "Terminated child process PID {}", pid);
        }
        CloseHandle(hProc);
    }
}
