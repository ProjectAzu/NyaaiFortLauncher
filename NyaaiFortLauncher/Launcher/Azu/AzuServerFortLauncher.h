#pragma once

#include "Launcher/Activities/FortLauncher.h"

class NAzuServerFortLauncher : public NFortLauncher
{
	GENERATE_BASE_H(NAzuServerFortLauncher)

public:
	virtual void OnCreated() override;
	
	NPROPERTY(GameServerDllPath)
	std::filesystem::path GameServerDllPath{};
	
	NPROPERTY(RedirectDllPath)
	std::filesystem::path RedirectDllPath{};
	
	NPROPERTY(RedirectDllThreadDescription)
	std::wstring RedirectDllThreadDescription{L"http 127.0.0.1 3551"};
	
	NPROPERTY(AzuGameModeClass)
	std::wstring AzuGameModeClass{L"NAzuGameModeSolo"};
	
	NPROPERTY(Login)
	std::wstring Login{};
	
	NPROPERTY(Password)
	std::wstring Password{};
};

