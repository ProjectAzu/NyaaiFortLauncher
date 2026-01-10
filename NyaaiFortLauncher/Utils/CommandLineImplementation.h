#pragma once

#include "Object/Object.h"

#include "CommandLine.h"

class NCommandLine : public NObject
{
    GENERATE_BASE_H(NCommandLine)
    
public:
    virtual void Log(const std::wstring& Message);
    virtual bool ShouldProgramExit() const;
    
    void EnqueueCommand(const std::wstring& Command);
    std::optional<std::wstring> GetPendingCommand();
    
private:
    std::queue<std::wstring> CommandQueue{};
};

void InitCommandLine(const TObjectTemplate<NCommandLine>& CommandLineTemplate);
void CleanupCommandLine();