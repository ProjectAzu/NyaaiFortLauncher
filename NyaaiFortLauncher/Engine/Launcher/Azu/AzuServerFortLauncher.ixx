module;
#include "Object/ReflectionMacros.h"

export module AzuLauncher;

export import Engine;

export class NAzuServerFortLauncher : public NFortLauncher
{
    ObjectBase(NAzuServerFortLauncher)

public:
    virtual void OnCreated() override;

    ReflectedMember(GameServerDllPath)
    std::filesystem::path GameServerDllPath{};

    ReflectedMember(RedirectDllPath)
    std::filesystem::path RedirectDllPath{};

    ReflectedMember(RedirectDllThreadDescription)
    std::wstring RedirectDllThreadDescription{ L"http 127.0.0.1 3551" };

    ReflectedMember(AzuGameModeClass)
    std::wstring AzuGameModeClass{ L"NAzuGameModeSolo" };

    ReflectedMember(Login)
    std::wstring Login{};

    ReflectedMember(Password)
    std::wstring Password{};
};
