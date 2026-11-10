module;
#include "Utils/WindowsInclude.h"
#include <tlhelp32.h>

module Engine;
import CreateProcessAction;
import BuildStoreActivity;

void NFortLauncher::OnCreated()
{
    Super::OnCreated();

    LauncherId = GetEngine()->GetNewLauncherId();

    Log(Info, L"Launcher starting, Id: {}", LauncherId);

    Log(Info, L"Running on launcher created actions");

    for (const auto& ActionTemplate : OnLauncherCreatedActions)
    {
        TUniqueObjectPtr<NAction> Action = ActionTemplate.NewObject(this);
    }

    if (auto BuildStore = GetEngine()->FindChildActivity<NBuildStoreActivity>())
    {
        Log(Info, L"Found NBuildStoreActivity, pulling the fortnite build path from it");

        auto BuildPathFromBuildStore = BuildStore->GetSelectedFortniteBuildPath();

        if (!BuildPathFromBuildStore)
        {
            Log(Error, L"BuildStore->GetSelectedFortniteBuildPath() returned nullopt, the launcher won't start");
            RequestStop();

            return;
        }

        FortniteBuildPath = BuildPathFromBuildStore.value();
    }

    if (!Utils::ConvertPathToAbsolutePath(FortniteBuildPath, {}, FortniteBuildPath))
    {
        RequestStop();

        return;
    }

    Log(Info, L"Fortnite build path: '{}'", FortniteBuildPath.wstring());

    Log(Info, L"Fortnite launch arguments: {}", FortniteLaunchArguments);

    GetCommandManager().RegisterConsoleCommand(
        this,
        L"stop",
        L"Kills the launcher",
        &ThisClass::StopCommand);

    GetCommandManager().RegisterConsoleCommand(
        this,
        L"ExecuteActionOnLauncher",
        L"Usage: ExecuteActionOnLauncher {ActionTemplate}",
        &ThisClass::ExecuteActionOnLauncherCommand);

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
        TUniqueObjectPtr<NAction> Action = ActionTemplate.NewObject(this);
    }

    GetEngine()->NotifyLauncherBeingDestroyed(this);
}

void NFortLauncher::Tick(double DeltaTime)
{
    if (WaitForSingleObject(FortniteProcessHandle.Get(), 0) != WAIT_TIMEOUT)
    {
        RequestStop();
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
        TUniqueObjectPtr<NAction> Action = ActionTemplate.NewObject(this);
    }

    if (!LaunchFortniteProcess())
    {
        return false;
    }

    Log(Info, L"Running post fortnite launch actions");

    for (const auto& ActionTemplate : PostFortniteLaunchActions)
    {
        TUniqueObjectPtr<NAction> Action = ActionTemplate.NewObject(this);
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
    FUniqueHandle StdOutReadHandle{};
    FUniqueHandle StdOutWriteHandle{};
    FUniqueHandle StdInReadHandle{};
    FUniqueHandle StdInWriteHandle{};

    SECURITY_ATTRIBUTES SecurityAttributes{};
    SecurityAttributes.nLength = sizeof(SecurityAttributes);
    SecurityAttributes.bInheritHandle = TRUE;

    if (!CreatePipe(StdOutReadHandle.GetAddressOf(), StdOutWriteHandle.GetAddressOf(), &SecurityAttributes, 0) ||
        !CreatePipe(StdInReadHandle.GetAddressOf(), StdInWriteHandle.GetAddressOf(), &SecurityAttributes, 0))
    {
        Log(Error, L"Failed to create pipe: {}", GetLastError());

        return false;
    }

    if (!SetHandleInformation(StdOutReadHandle, HANDLE_FLAG_INHERIT, 0) ||
        !SetHandleInformation(StdInWriteHandle, HANDLE_FLAG_INHERIT, 0))
    {
        Log(Error, L"Failed to set pipe handle information: {}", GetLastError());

        return false;
    }

    auto CreateFortniteProcessAction = NCreateProcessAction::StaticClass()->NewObject<NCreateProcessAction>(this, true);
    CreateFortniteProcessAction->StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    CreateFortniteProcessAction->StartupInfo.hStdOutput = StdOutWriteHandle;
    CreateFortniteProcessAction->StartupInfo.hStdError = StdOutWriteHandle;
    CreateFortniteProcessAction->StartupInfo.hStdInput = StdInReadHandle;
    CreateFortniteProcessAction->FilePath = FortniteExePath;
    CreateFortniteProcessAction->LaunchArguments = FortniteLaunchArguments;
    CreateFortniteProcessAction->FinishConstruction();

    if (!CreateFortniteProcessAction->bResultWasSuccess)
    {
        return false;
    }

    FortniteStdOutReadPipeHandle = std::move(StdOutReadHandle);
    FortniteStdInWritePipeHandle = std::move(StdInWriteHandle);
    FortniteProcessHandle = CreateFortniteProcessAction->ResultProcessHandle.Release();

    return true;
}

void NFortLauncher::StopCommand(const FCommandArguments& Args)
{
    RequestStop();
}

void NFortLauncher::ExecuteActionOnLauncherCommand(const FCommandArguments& Args)
{
    auto ActionTemplate = Args.GetRawStringAsType<TObjectTemplate<NAction>>();

    if (!ActionTemplate)
    {
        Log(Error, L"Specify an action template");

        return;
    }

    auto Action = ActionTemplate.NewObject(this);
}
