#pragma once

#include "Engine.h"
#include "FortLauncher.h"

// The basic engine manages only one launcher instance at a time
class NBasicEngine : public NEngine
{
public:
    GENERATE_BASE_H(NBasicEngine)
    
    virtual void OnCreated() override;
    
    void StartCommand(const FCommandArguments& Args);
    void RestartCommand(const FCommandArguments& Args);
    
    NPROPERTY(LauncherTemplate)
    TObjectTemplate<NFortLauncher> LauncherTemplate = NFortLauncher::StaticClass();
    
    NPROPERTY(bAutoStartLauncher)
    bool bAutoStartLauncher = true;
};
