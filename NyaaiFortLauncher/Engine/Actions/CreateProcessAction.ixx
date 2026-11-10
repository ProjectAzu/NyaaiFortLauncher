module;
#include "Object/ReflectionMacros.h"
#include "Utils/WindowsInclude.h"

export module CreateProcessAction;

export import Engine;

export class NCreateProcessAction : public NAction
{
    ObjectBase(NCreateProcessAction)

public:
    virtual void Execute() override;

    ReflectedMember(FilePath)
    std::filesystem::path FilePath{};

    ReflectedMember(LaunchArguments)
    std::wstring LaunchArguments{};

    ReflectedMember(bCreateSuspended)
    bool bCreateSuspended = false;

    ReflectedMember(WorkingDirectory)
    std::filesystem::path WorkingDirectory{};

    STARTUPINFOW StartupInfo{};
    PROCESS_INFORMATION ResultProcessInfo{};

    FUniqueHandle ResultProcessHandle = nullptr;
    FUniqueHandle ResultThreadHandle = nullptr;
    bool bResultWasSuccess = false;
};
