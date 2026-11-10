module;
#include "Object/ReflectionMacros.h"

export module InjectDllIntoFortniteAction;

export import Engine;

export class NInjectDllIntoFortniteAction : public NAction
{
    ObjectBase(NInjectDllIntoFortniteAction)

public:
    virtual void Execute() override;

    ReflectedMember(DllPath)
    std::filesystem::path DllPath{};

    ReflectedMember(DllThreadDescription)
    std::wstring DllThreadDescription{};
};
