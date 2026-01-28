#include "FortLauncher.h"

#include "Utils/WindowsInclude.h"
#include <tlhelp32.h>
#include <queue>

#include <cwctype>

#include "CommandManager.h"
#include "Engine.h"
#include "Actions/CreateProcessAction.h"

GENERATE_BASE_CPP(NFortLauncher)

void NFortLauncher::OnCreated()
{
    Super::OnCreated();
    
    LauncherId = GetEngine()->GetNewLauncherId();

    Log(Info, L"Launcher starting, Id: {}", LauncherId);
    
    Log(Info, L"Running on launcher created actions");
    for (const auto& ActionTemplate : OnLauncherCreatedActions)
    {
        NUniquePtr<NAction> Action = ActionTemplate.NewObject(this);
    }

    Log(Info, L"Fortnite exe path: {}", FortniteExePath.wstring());
    if (!exists(FortniteExePath) || !is_regular_file(FortniteExePath) || FortniteExePath.extension().wstring() != L".exe")
    {
        Log(Error, L"The fortnite exe path is not valid");
        
        RequestStop();
        return;
    }

    Log(Info, L"Fortnite launch arguments: {}", FortniteLaunchArguments);
    
    GetCommandManager().RegisterConsoleCommand(
        this,
        L"stop",
        L"Kills the launcher",
        &ThisClass::StopCommand
    );
    
    if (!RunLauncher())
    {
        RequestStop();
        return;
    }
}

void NFortLauncher::OnDestroyed()
{
    Super::OnDestroyed();
    
    Log(Info, L"Running on launcher exit actions");
    for (const auto& ActionTemplate : OnLauncherExitActions)
    {
        NUniquePtr<NAction> Action = ActionTemplate.NewObject(this);
    }
    
    GetEngine()->NotifyLauncherBeingDestroyed(this);
}

void NFortLauncher::Tick(double DeltaTime)
{
    if (WaitForSingleObject(FortniteProcessHandle.Get(), 0) != WAIT_TIMEOUT)
    {
        bWantsToStop = true;
    }
    
    if (bWantsToStop)
    {
        Destroy();
        return;
    }
    
    Super::Tick(DeltaTime);
}

bool NFortLauncher::RunLauncher()
{
    Log(Info, L"Running pre fortnite launch actions");
    for (const auto& ActionTemplate : PreFortniteLaunchActions)
    {
        NUniquePtr<NAction> Action = ActionTemplate.NewObject(this);
    }

    if (!LaunchFortniteProcess())
    {
        return false;
    }

    Log(Info, L"Running post fortnite launch actions");
    for (const auto& ActionTemplate : PostFortniteLaunchActions)
    {
        NUniquePtr<NAction> Action = ActionTemplate.NewObject(this);
    }
    
    Log(Info, L"Finished base launch!");

    Log(Info, L"Spawning activities");
    
    for (const auto& ActivityTemplate : Activities)
    {
        StartChildActivity(ActivityTemplate);
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

    if (!CreatePipe(FortniteStdOutReadPipeHandle.GetAddressOf(), &StdOutWriteHandle, &SeciurityAtributes, 0))
    {
        Log(Error, L"Failed to create pipe");
        return false;
    }

    if (!SetHandleInformation(FortniteStdOutReadPipeHandle, HANDLE_FLAG_INHERIT, 0))
    {
        Log(Error, L"Failed to set handle information");

        CloseHandle(StdOutWriteHandle);
        StdOutWriteHandle = nullptr;

        CloseHandle(FortniteStdOutReadPipeHandle);
        FortniteStdOutReadPipeHandle = nullptr;

        return false;
    }

    HANDLE StdInReadHandle = nullptr;

    if (!CreatePipe(&StdInReadHandle, FortniteStdInWritePipeHandle.GetAddressOf(), &SeciurityAtributes, 0))
    {
        Log(Error, L"Failed to create pipe");
        return false;
    }

    if (!SetHandleInformation(FortniteStdInWritePipeHandle, HANDLE_FLAG_INHERIT, 0))
    {
        Log(Error, L"Failed to set handle information");

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
        NCreateProcessAction::StaticClass()->NewObject<NCreateProcessAction>(this, true);

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

    FortniteProcessHandle = CreateFortniteProcessAction->ResultProcessHandle.Release();

    return true;
}

void NFortLauncher::StopCommand(const FCommandArguments& Args)
{
    bWantsToStop = true;
}
