#include "FortLauncher.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <tlhelp32.h>
#include <queue>

#include "Activity.h"
#include "CreateProcessAction.h"
#include "RequestExitAction.h"
#include "RequestRelaunchAction.h"

FCommandArguments::FCommandArguments(const std::string& RawString) : RawString(RawString)
{
    if (RawString.empty())
    {
        return;
    }

    Tokens.clear();
    
    std::string Current;
    bool bIsInQuotes = false;
    bool bShouldEscapeNext = false;

    for (char Char : RawString)
    {
        if (bShouldEscapeNext)                 // honour \" inside quotes
        {
            Current.push_back(Char);
            bShouldEscapeNext = false;
            continue;
        }

        if (Char == '\\')
        {
            bShouldEscapeNext = true;          // consume backslash, look at next char
            continue;
        }

        if (Char == '"')
        {
            bIsInQuotes = !bIsInQuotes;       // toggle quote mode
            continue;                   // do not include the quotes themselves
        }

        if (!bIsInQuotes && std::isspace(static_cast<unsigned char>(Char)))
        {
            if (!Current.empty())
            {
                Tokens.push_back(std::move(Current));
                Current.clear();
            }
        }
        else
        {
            Current.push_back(Char);
        }
    }

    if (!Current.empty())
    {
        Tokens.push_back(std::move(Current));   
    }
}

std::string FCommandArguments::GetArgumentAtIndex(uint8 Index) const
{
    if (Index >= static_cast<uint32>(Tokens.size()))
    {
        Log(Error, "Invalid command argument index");
        return {};
    }

    return Tokens[Index];
}

GENERATE_BASE_CPP(NFortLauncher)

void NFortLauncher::OnCreated()
{
    Super::OnCreated();

    RegisterConsoleCommand(
        this,
        "help",
        "Lists available commands",
        &ThisClass::HelpCommand
    );

    RegisterConsoleCommand(
        this,
        "restart",
        "Requests a relaunch.",
        &ThisClass::RestartCommand
    );

    RegisterConsoleCommand(
        this,
        "exit",
        "Requests that the launcher exits.",
        &ThisClass::ExitCommand
    );

    RegisterConsoleCommand(
        this,
        "ExecuteAction",
        "Executes an action. Usage: ExecuteAction \"ActionTemplate\"",
        &ThisClass::ExecuteActionCommand
    );
    
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

void NFortLauncher::NotifyObjectCreated(NLauncherObject* Object)
{
}

void NFortLauncher::NotifyObjectDestroyed(NLauncherObject* Object)
{
    for (int32 i = static_cast<int32>(RegisteredCommands.size()) - 1; i >= 0; i--)
    {
        if (RegisteredCommands[i].OwningObject != Object)
        {
            continue;
        }

        RegisteredCommands[i] = std::move(RegisteredCommands.back());
        RegisteredCommands.pop_back();
    }
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

        ProcessCommands();
        
        for (const auto& Activity : ActivitiesSpawned)
        {
            Activity->Tick(DeltaTime);
        }

        if (bWantsToRelaunch || bWantsToExit)
        {
            return true;
        }

        if (WaitForSingleObject(FortniteProcessHandle, 50) != WAIT_TIMEOUT)
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
        StdOutWriteHandle = nullptr;
        
        CloseHandle(FortniteStdOutReadPipeHandle);
        FortniteStdOutReadPipeHandle = nullptr;
        
        return false;
    }

    HANDLE StdInReadHandle = nullptr;

    if (!CreatePipe(&StdInReadHandle, &FortniteStdInWritePipeHandle, &SeciurityAtributes, 0))
    {
        Log(Error, "Failed to create pipe");
        return false;
    }

    if (!SetHandleInformation(FortniteStdInWritePipeHandle, HANDLE_FLAG_INHERIT, 0))
    {
        Log(Error, "Failed to set handle information");
        
        CloseHandle(StdInReadHandle);
        StdInReadHandle = nullptr;
        
        CloseHandle(FortniteStdInWritePipeHandle);
        FortniteStdInWritePipeHandle = nullptr;

        CloseHandle(StdOutWriteHandle);
        StdOutWriteHandle = nullptr;
        
        CloseHandle(FortniteStdOutReadPipeHandle);
        FortniteStdOutReadPipeHandle = nullptr;
        
        return false;
    }
    
    NUniquePtr<NCreateProcessAction> CreateFortniteProcessAction =
        NCreateProcessAction::StaticClass()->NewObject<NCreateProcessAction>(this, {}, true);

    CreateFortniteProcessAction->bReturnProcessHandle = true;
    
    CreateFortniteProcessAction->StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    CreateFortniteProcessAction->StartupInfo.hStdOutput = StdOutWriteHandle;
    CreateFortniteProcessAction->StartupInfo.hStdError = StdOutWriteHandle;
    CreateFortniteProcessAction->StartupInfo.hStdInput = StdInReadHandle;
    
    CreateFortniteProcessAction->FilePath = FortniteExePath;
    CreateFortniteProcessAction->LaunchArguments = FortniteLaunchArguments;
    
    CreateFortniteProcessAction->FinishConstruction();

    CloseHandle(StdOutWriteHandle);
    StdOutWriteHandle = nullptr;

    CloseHandle(StdInReadHandle);
    StdInReadHandle = nullptr;

    if (!CreateFortniteProcessAction->bResultWasSuccess)
    {
        CloseHandle(FortniteStdOutReadPipeHandle);
        FortniteStdOutReadPipeHandle = nullptr;

        CloseHandle(FortniteStdInWritePipeHandle);
        FortniteStdInWritePipeHandle = nullptr;
        
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

    if (FortniteStdInWritePipeHandle)
    {
        CloseHandle(FortniteStdInWritePipeHandle);
        FortniteStdInWritePipeHandle = nullptr;
    }
    
    KillAllChildProcesses();
}

FRegisteredCommand* NFortLauncher::FindRegisteredCommand(std::string Command)
{
    std::ranges::transform(Command, Command.begin(),
                           [](unsigned char c){ return std::tolower(c); });
    
    for (auto& Entry : RegisteredCommands)
    {
        std::string EntryCommandToLower = Entry.Command;
        std::ranges::transform(EntryCommandToLower, EntryCommandToLower.begin(),
                               [](unsigned char c){ return std::tolower(c); });
        
        if (EntryCommandToLower == Command)
        {
            return &Entry;
        }
    }

    return nullptr;
}

void NFortLauncher::ProcessCommands()
{
    while (auto RawCommand = GetPendingConsoleCommand())
    {
        const auto FirstSpace = std::ranges::find_if(*RawCommand,
            [](unsigned char ch)
            {
                return std::isspace(ch);
            });
        
        const std::string Command{ RawCommand->begin(), FirstSpace };
        
        if (Command.empty())
        {
            Log(Info, "Type 'help' to list available commands.");
            
            continue;
        }

        FRegisteredCommand* RegisteredCommand = FindRegisteredCommand(Command);
        if (!RegisteredCommand)
        {
            Log(Error, "Unable to find command '{}'. Type 'help' to list available commands.", Command);
            continue;
        }
        
        auto ArgsBegin = std::find_if_not(
            FirstSpace, RawCommand->end(),
        [](unsigned char ch) { return std::isspace(ch); });

        const std::string ArgString{ ArgsBegin, RawCommand->end() };
        
        FCommandArguments Args(ArgString);
        RegisteredCommand->ExecuteCommandFunction(RegisteredCommand->OwningObject, Args);
    }
}

void NFortLauncher::HelpCommand(const FCommandArguments& Args)
{
    Log(Info, "Available commands:");
    
    for (const auto& RegisteredCommand : RegisteredCommands)
    {
        Log(
            Info,
            "{}: '{}' - {}",
            RegisteredCommand.OwningObject->GetClass()->GetName(),
            RegisteredCommand.Command,
            RegisteredCommand.Description
            );
    }
}

void NFortLauncher::RestartCommand(const FCommandArguments& Args)
{
    NRequestRelaunchAction::StaticClass()->NewObject(this);
}

void NFortLauncher::ExitCommand(const FCommandArguments& Args)
{
    NRequestExitAction::StaticClass()->NewObject(this);
}

void NFortLauncher::ExecuteActionCommand(const FCommandArguments& Args)
{
    FObjectInitializeTemplate<NAction> ActionTemplate = Args.GetArgumentAtIndex<FObjectInitializeTemplate<NAction>>(0);
    if (!ActionTemplate.Class)
    {
        return;
    }

    ActionTemplate.NewObject(this);
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
