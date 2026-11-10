module;
#include "Object/ReflectionMacros.h"

export module CommandLine;

export import Reflection;
export import Logging;
export import ObjectTemplate;

export class NCommandLine : public NObject, public FCommandLineBackend
{
    ObjectBase(NCommandLine)

public:
    virtual void Log(const std::wstring& Message) override;
    virtual bool ShouldProgramExit() const override;

    virtual void EnqueueCommand(const std::wstring& Command) override;
    virtual std::optional<std::wstring> GetPendingCommand() override;
    virtual void DestroyBackend() override { Destroy(); }

private:
    std::queue<std::wstring> CommandQueue{};
    std::mutex QueueMutex{};
};

export void InitCommandLine(const TObjectTemplate<NCommandLine>& CommandLineTemplate);
