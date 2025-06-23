#pragma once

#include "Action.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

class NCreateProcessAction : public NAction
{
    GENERATE_BASE_H(NCreateProcessAction)
    
public:
    void Execute() override;
    
    NPROPERTY(FilePath)
    std::filesystem::path FilePath{};

    NPROPERTY(LaunchArguments)
    std::string LaunchArguments{};

    NPROPERTY(bSuspendProcessAfterCreation)
    bool bSuspendProcessAfterCreation = false;

    STARTUPINFOW StartupInfo{};
    PROCESS_INFORMATION ResultProcessInfo{};

    // Be careful, you will need to close the handle yourself if you do this
    bool bReturnProcessHandle = false;

    void* ResultProcessHandle = nullptr;
    bool bResultWasSuccess = false;
};
