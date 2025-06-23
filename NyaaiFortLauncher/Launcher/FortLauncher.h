#pragma once

#include "LauncherObject.h"

#include "Action.h"

#include <filesystem>

class NActivity;

class NFortLauncher : public NLauncherObject
{
    GENERATE_BASE_H(NFortLauncher)
    
public:
    void OnCreated() override;
    void OnDestroyed() override;

    inline void* GetFortniteProcessHandle() const { return FortniteProcessHandle; }
    inline void* GetFortniteStdOutReadPipeHandle() const { return FortniteStdOutReadPipeHandle; }

    inline bool HasFinishedBaseLaunch() const { return bHasFinishedBaseLaunch; }

    void RequestRelaunch();
    void RequestExit();

private:
    bool RunLauncher();
    bool LaunchFortniteProcess();
    void DoCleanup();

    void KillAllChildProcesses();

public:
    NPROPERTY(FortniteExePath)
    std::filesystem::path FortniteExePath{};

    NPROPERTY(FortniteLaunchArguments)
    std::string FortniteLaunchArguments{};

    NPROPERTY(PreFortniteLaunchActions)
    std::vector<FObjectInitializeTemplate<NAction>> PreFortniteLaunchActions{};

    NPROPERTY(PostFortniteLaunchActions)
    std::vector<FObjectInitializeTemplate<NAction>> PostFortniteLaunchActions{};

    NPROPERTY(Activities)
    std::vector<FObjectInitializeTemplate<NActivity>> Activities{};

private:
    void* FortniteProcessHandle = nullptr;
    void* FortniteStdOutReadPipeHandle = nullptr;

    bool bHasFinishedBaseLaunch = false;

    bool bWantsToRelaunch = false;
    bool bWantsToExit = false;
};
