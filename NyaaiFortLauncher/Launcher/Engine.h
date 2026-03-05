#pragma once

#include "Activity.h"
#include "CommandManager.h"
#include "SaveRecord.h"
#include "Actions/Action.h"

struct FSaveRecordsSystem;
class NActivity;

class NEngine : public NActivity
{
    GENERATE_BASE_H(NEngine)
    
public:
    virtual void OnCreated() override;
    
    void RunTickLoop();
    
    virtual void Tick(double DeltaTime) override;
    
    virtual bool ShouldEngineExit() const;
    
    virtual void NotifyLauncherBeingDestroyed(NFortLauncher* Launcher);
    
    NFortLauncher* GetCommandsContextLauncher() const;
    NEngineObject* GetCommandsContextObject() const;
    
    void RequestExit() { bWantsToExit = true; }

private:
    void HelpCommand(const FCommandArguments& Args);
    void PrintLauncherInstancesListCommand(const FCommandArguments& Args);
    void SetCommandsContextLauncherIdCommand(const FCommandArguments& Args);
    void ExitCommand(const FCommandArguments& Args);

public:
    void NotifyObjectDestroyed(NEngineObject* Object);
    
    FCommandManager& GetCommandManager() const;
    FSaveRecordsSystem& GetSaveRecordsSystem() const;
    
    std::vector<NFortLauncher*> GetLauncherInstances() const;
    
    int32 GetNewLauncherId() { return LastUnusedLauncherId++; }
    
public:
    NPROPERTY(OnEngineInitActions)
    std::vector<TObjectTemplate<NAction>> OnEngineInitActions{};
    
    NPROPERTY(Activities)
    std::vector<TObjectTemplate<NActivity>> Activities{};
    
    NPROPERTY(SaveRecordsSystem)
    FSaveRecordsSystem SaveRecordsSystem{};
    
private:
    FCommandManager CommandManager{};
    
    int32 LastUnusedLauncherId = 0;
    
    int32 CommandsContextLauncherId = -1;
    
    bool bWantsToExit = false;
};
