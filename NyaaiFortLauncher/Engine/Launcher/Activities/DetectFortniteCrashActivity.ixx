module;
#include "Object/ReflectionMacros.h"

export module DetectFortniteCrashActivity;

export import Engine;

export class NDetectFortniteCrashActivity : public NActivity
{
    ObjectBase(NDetectFortniteCrashActivity)

public:
    virtual void OnCreated() override;
    virtual void OnDestroyed() override;

    virtual void Tick(double DeltaTime) override;

    ReflectedMember(FortniteCrashesFolderPath)
    std::filesystem::path FortniteCrashesFolderPath{ L"%localappdata%\\FortniteGame\\Saved\\Crashes" };

    ReflectedMember(bPrintCallstack)
    bool bPrintCallstack = true;

    ReflectedMember(OnFortniteCrashActions)
    std::vector<TObjectTemplate<NAction>> OnFortniteCrashActions{};

private:
    uint32 FortniteProcessId = 0;
    std::unordered_set<std::filesystem::path> AlreadyCheckedCrashContextFilePaths{};
    std::unordered_set<std::filesystem::path> PendingCrashContextFilePaths{};
    void* CrashDirChangeHandle = nullptr;
    bool bCrashDetected = false;
    double TimeUntilWatchRetry = 0.0;
    std::error_code LastScanError{};
    void StartWatching();
    void StopWatching();
    void ScanCrashContexts(bool bIgnoreExisting);
    void ProcessCrashContexts();
};
