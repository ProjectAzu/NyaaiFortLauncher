module;
#include "Object/ReflectionMacros.h"

export module CopyFileAction;

export import Engine;

export class NCopyFileAction : public NAction
{
    ObjectBase(NCopyFileAction)

public:
    virtual void Execute() override;

    ReflectedMember(From)
    std::filesystem::path From{};

    ReflectedMember(To)
    std::filesystem::path To{};
};
