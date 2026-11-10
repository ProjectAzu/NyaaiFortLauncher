module;
#include "Object/ReflectionMacros.h"

export module RequestStopAction;

export import Engine;

export class NRequestStopAction : public NAction
{
    ObjectBase(NRequestStopAction)

public:
    virtual void Execute() override;
};
