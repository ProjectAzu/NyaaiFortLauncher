#pragma once

#include "Actions/Action.h"

#include <filesystem>

#include "Activity.h"
#include "Object/ObjectTemplate.h"
#include "Utils/WindowsInclude.h"

struct FCommandArguments;
class NActivity;

class NFortLauncher : public NActivity
{
    GENERATE_BASE_H(NFortLauncher)
    
public:
    virtual void OnCreated() override;
    virtual void OnDestroyed() override;
    
    virtual void Tick(double DeltaTime) override;

    inline void* GetFortniteProcessHandle() const { return FortniteProcessHandle; }
    inline void* GetFortniteStdOutReadPipeHandle() const { return FortniteStdOutReadPipeHandle; }
    inline void* GetFortniteStdInWritePipeHandle() const { return FortniteStdInWritePipeHandle; }
    
    int32 GetLauncherId() const { return LauncherId; }
    
    void RequestExit() { bWantsToExit = true; }

private:
    bool RunLauncher();
    bool LaunchFortniteProcess();
    
    void StopCommand(const FCommandArguments& Args);

public:
    NPROPERTY(FortniteExePath)
    std::filesystem::path FortniteExePath{};

    NPROPERTY(FortniteLaunchArguments)
    std::wstring FortniteLaunchArguments{};
    
    NPROPERTY(OnLauncherCreatedActions)
    std::vector<TObjectTemplate<NAction>> OnLauncherCreatedActions{};

    NPROPERTY(PreFortniteLaunchActions)
    std::vector<TObjectTemplate<NAction>> PreFortniteLaunchActions{};

    NPROPERTY(PostFortniteLaunchActions)
    std::vector<TObjectTemplate<NAction>> PostFortniteLaunchActions{};
    
    NPROPERTY(OnLauncherExitActions)
    std::vector<TObjectTemplate<NAction>> OnLauncherExitActions{};

    NPROPERTY(Activities)
    std::vector<TObjectTemplate<NActivity>> Activities{};

private:
    FUniqueHandle FortniteProcessHandle = nullptr;
    FUniqueHandle FortniteStdOutReadPipeHandle = nullptr;
    FUniqueHandle FortniteStdInWritePipeHandle = nullptr;
    
    bool bWantsToExit = false;
    
    int32 LauncherId = 0;
};
