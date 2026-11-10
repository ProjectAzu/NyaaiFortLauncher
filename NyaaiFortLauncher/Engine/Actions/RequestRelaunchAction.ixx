module;
#include "Object/ReflectionMacros.h"

export module RequestRelaunchAction;

export import Engine;

export class NRequestRelaunchAction : public NAction
{
    ObjectBase(NRequestRelaunchAction)

public:
    virtual void Execute() override;
};
