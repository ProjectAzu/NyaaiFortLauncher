module;
#include "Object/ReflectionMacros.h"

export module RunCommandAction;

export import Engine;

export class NRunCommandAction : public NAction
{
    ObjectBase(NRunCommandAction)

public:
    virtual void Execute() override;

    ReflectedMember(Command)
    std::wstring Command{};
};
