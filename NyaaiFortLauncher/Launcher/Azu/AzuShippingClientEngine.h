#pragma once

#include "Launcher/Engine.h"

class NAzuShippingClientEngine : public NEngine
{
    GENERATE_BASE_H(NAzuShippingClientEngine)
    
public:
    NAzuShippingClientEngine();
    
    virtual void OnCreated() override;
    
    virtual void NotifyLauncherBeingDestroyed(NFortLauncher* Launcher) override;
    
private:
    bool StartLauncherInstance();
    
    void RestartCommand(const FCommandArguments& Args);
    
private:
    std::filesystem::path FortniteBuildPath{};
    std::wstring Login{};
    std::wstring Password{};
    
    bool bIsRestarting = false;
};
