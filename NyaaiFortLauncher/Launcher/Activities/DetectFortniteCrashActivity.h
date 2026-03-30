#pragma once

#include "Launcher/Activity.h"

#include <unordered_set>

#include "Launcher/Actions/Action.h"

class NDetectFortniteCrashActivity : public NActivity
{
    GENERATE_BASE_H(NDetectFortniteCrashActivity)

public:
    virtual void OnCreated() override;
    virtual void OnDestroyed() override;
    
    virtual void Tick(double DeltaTime) override;
    
    NPROPERTY(FortniteCrashesFolderPath)
    std::filesystem::path FortniteCrashesFolderPath{L"%localappdata%\\FortniteGame\\Saved\\Crashes"};
    
    NPROPERTY(bPrintCallstack)
    bool bPrintCallstack = true;
    
    NPROPERTY(OnFortniteCrashActions)
    std::vector<TObjectTemplate<NAction>> OnFortniteCrashActions{};

private:
    uint32 FortniteProcessId = 0;
    std::unordered_set<std::filesystem::path> AlreadyCheckedCrashContextFilePaths{};
    std::unordered_set<std::filesystem::path> PendingCrashContextFilePaths{};
    void* CrashDirChangeHandle = nullptr;
};
