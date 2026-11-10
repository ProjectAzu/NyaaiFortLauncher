module;
#include "Object/ReflectionMacros.h"

export module Engine:Action;

export import :EngineObject;

export class NAction : public NEngineObject
{
    ObjectBase(NAction)

public:
    virtual void OnCreated() override;

protected:
    virtual void Execute();

    bool bPrintExecutingActionMessage = true;
};
