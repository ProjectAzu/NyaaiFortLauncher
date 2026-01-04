#pragma once

#include "Action.h"

#include "Utils/WindowsInclude.h"

class NCreateProcessAction : public NAction
{
    GENERATE_BASE_H(NCreateProcessAction)
    
public:
    virtual void Execute() override;
    
    NPROPERTY(FilePath)
    std::filesystem::path FilePath{};

    NPROPERTY(LaunchArguments)
    std::wstring LaunchArguments{};

    NPROPERTY(bCreateSuspended)
    bool bCreateSuspended = false;
    
    NPROPERTY(WorkingDirectory)
    std::filesystem::path WorkingDirectory{};

    STARTUPINFOW StartupInfo{};
    PROCESS_INFORMATION ResultProcessInfo{};
    
    FUniqueHandle ResultProcessHandle = nullptr;
    FUniqueHandle ResultThreadHandle = nullptr;
    bool bResultWasSuccess = false;
};
