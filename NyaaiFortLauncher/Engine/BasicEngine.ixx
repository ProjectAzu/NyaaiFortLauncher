module;
#include "Object/ReflectionMacros.h"

export module BasicEngine;

export import Engine;

// The basic engine manages only one launcher instance at a time
export class NBasicEngine : public NEngine
{
public:
    ObjectBase(NBasicEngine)

    virtual void OnCreated() override;

    void StartCommand(const FCommandArguments& Args);
    void RestartCommand(const FCommandArguments& Args);

    ReflectedMember(LauncherTemplate)
    TObjectTemplate<NFortLauncher> LauncherTemplate = NFortLauncher::StaticClass();

    ReflectedMember(bAutoStartLauncher)
    bool bAutoStartLauncher = true;
};
