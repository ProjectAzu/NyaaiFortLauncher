module;
#include "Object/ReflectionMacros.h"

export module PrintLogAction;

export import Engine;

export class NPrintLogAction : public NAction
{
    ObjectBase(NPrintLogAction)

public:
    NPrintLogAction();

    virtual void Execute() override;

    ReflectedMember(Message)
    std::wstring Message{};

    ReflectedMember(LogLevel)
    std::wstring LogLevel = L"Info";
};
