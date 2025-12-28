#pragma once

#include "Activity.h"
#include "Object/ObjectInitializeTemplate.h"

#include <unordered_set>

class NDetectFortniteCrashActivity : public NActivity
{
    GENERATE_BASE_H(NDetectFortniteCrashActivity)

public:
    void OnCreated() override;
    void OnDestroyed() override;
    
    void Tick(double DeltaTime) override;
    
    NPROPERTY(OnFortniteCrashActions)
    std::vector<TObjectInitializeTemplate<NAction>> OnFortniteCrashActions{};

private:
    uint32 FortniteProcessId = 0;
    std::filesystem::path FortniteCrashesFolderPath{};
    std::unordered_set<std::filesystem::path> AlreadyCheckedCrashContextFilePaths{};
    std::unordered_set<std::filesystem::path> PendingCrashContextFilePaths{};
    void* CrashDirChangeHandle = nullptr;
};
