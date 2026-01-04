#pragma once

#include "Activity.h"
#include "CommandManager.h"
#include "Utils/CommandLineImplementation.h"

class NActivity;

class NEngine : public NActivity
{
    GENERATE_BASE_H(NEngine)
    
public:
    NEngine();
    
    virtual void OnCreated() override;
    virtual void OnDestroyed() override;
    
    virtual void Tick(double DeltaTime) override;
    
protected:
    virtual bool ShouldPrintHelpAndExit() const;
    
    virtual TObjectTemplate<NFortLauncher> GetDefaultLauncherTemplate() const;
    
private:
    static void PrintClassesInfo();
    
    NFortLauncher* GetCommandsContextLauncher() const;
    NEngineObject* GetCommandsContextObject() const;
    
    void HelpCommand(const FCommandArguments& Args);
    void PrintLauncherInstancesListCommand(const FCommandArguments& Args);
    void SetCommandsContextLauncherIdCommand(const FCommandArguments& Args);
    void StartCommand(const FCommandArguments& Args);
    void RestartCommand(const FCommandArguments& Args);
    
public:
    void NotifyObjectDestroyed(NEngineObject* Object);
    
    FCommandManager& GetCommandManager() const;
    
    std::vector<NFortLauncher*> GetLauncherInstances() const;
    
    int32 GetNewLauncherId() { return LastUnusedLauncherId++; }
    
public:
    NPROPERTY(bUsingExternalTicking)
    bool bUsingExternalTicking = false;

    uint32 meow = 3;
    
    NPROPERTY(CommandLineTemplate)
    TObjectTemplate<NCommandLine> CommandLineTemplate{};
    
private:
    FCommandManager CommandManager{};
    
    int32 LastUnusedLauncherId = 0;
    
    int32 CommandsContextLauncherId = -1;
};
