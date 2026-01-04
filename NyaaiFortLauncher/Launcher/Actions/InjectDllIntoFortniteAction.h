#pragma once
#include "Action.h"

class NInjectDllIntoFortniteAction : public NAction
{
    GENERATE_BASE_H(NInjectDllIntoFortniteAction)
    
public:
    virtual void Execute() override;
    
    NPROPERTY(DllPath)
    std::filesystem::path DllPath{};

    NPROPERTY(DllThreadDescription)
    std::wstring DllThreadDescription{};
};
