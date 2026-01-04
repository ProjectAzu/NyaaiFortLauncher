#pragma once

#include "Action.h"

class NWaitForBackendToRespondAction : public NAction
{
    GENERATE_BASE_H(NWaitForBackendToRespondAction)
    
public:
    virtual void Execute() override;
    
    NPROPERTY(Url)
    std::wstring Url{};
};
