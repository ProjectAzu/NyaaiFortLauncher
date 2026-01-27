#pragma once
#include "Engine.h"

class NDefaultEngine : public NEngine
{
public:
    GENERATE_BASE_H(NDefaultEngine)
    
    virtual void OnCreated() override;
    
    virtual TObjectTemplate<NFortLauncher> GetLauncherTemplateFromConfig() const;
    
    void StartCommand(const FCommandArguments& Args);
    void RestartCommand(const FCommandArguments& Args);
    
private:
    std::filesystem::path ConfigPath{};
};
