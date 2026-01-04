#pragma once

#include "Action.h"

class NCopyFileAction : public NAction
{
    GENERATE_BASE_H(NCopyFileAction)

public:
    virtual void Execute() override;

    NPROPERTY(From)
    std::filesystem::path From{};

    NPROPERTY(To)
    std::filesystem::path To{};
};
