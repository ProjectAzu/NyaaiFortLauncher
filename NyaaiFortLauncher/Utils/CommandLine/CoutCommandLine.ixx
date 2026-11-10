module;
#include "Object/ReflectionMacros.h"

export module CoutCommandLine;

export import CommandLine;

export class NCoutCommandLine : public NCommandLine
{
    ObjectBase(NCoutCommandLine)

public:
    virtual void Log(const std::wstring& Message) override;
};
