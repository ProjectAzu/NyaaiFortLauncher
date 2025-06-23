#pragma once

#include "Action.h"

class NWaitForBackendToRespondAction : public NAction
{
    GENERATE_BASE_H(NWaitForBackendToRespondAction)
    
public:
    void Execute() override;
    
    NPROPERTY(Url)
    std::string Url{};
};
