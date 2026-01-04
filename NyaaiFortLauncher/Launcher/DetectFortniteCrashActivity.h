#pragma once

#include "Activity.h"
#include "Object/ObjectTemplate.h"

#include <unordered_set>

#include "Launcher/Actions/Action.h"

class NDetectFortniteCrashActivity : public NActivity
{
    GENERATE_BASE_H(NDetectFortniteCrashActivity)

public:
    virtual void OnCreated() override;
    virtual void OnDestroyed() override;
    
    virtual void Tick(double DeltaTime) override;
    
    NPROPERTY(OnFortniteCrashActions)
    std::vector<TObjectTemplate<NAction>> OnFortniteCrashActions{};

private:
    uint32 FortniteProcessId = 0;
    std::filesystem::path FortniteCrashesFolderPath{};
    std::unordered_set<std::filesystem::path> AlreadyCheckedCrashContextFilePaths{};
    std::unordered_set<std::filesystem::path> PendingCrashContextFilePaths{};
    void* CrashDirChangeHandle = nullptr;
};
