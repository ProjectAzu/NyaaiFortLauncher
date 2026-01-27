#pragma once

#include "Launcher/FortLauncher.h"

class NAzuServerFortLauncher : public NFortLauncher
{
	GENERATE_BASE_H(NAzuServerFortLauncher)

public:
	virtual void OnCreated() override;

	NPROPERTY(FortnitePath)
	std::filesystem::path FortnitePath{};
	
	NPROPERTY(GameServerDllPath)
	std::filesystem::path GameServerDllPath{};
	
	NPROPERTY(RedirectDllPath)
	std::filesystem::path RedirectDllPath{};
	
	NPROPERTY(RedirectDllThreadDescription)
	std::wstring RedirectDllThreadDescription{L"http 78.47.120.58 8080"};
	
	NPROPERTY(AzuGameModeClass)
	std::wstring AzuGameModeClass{L"NAzuGameModeSolo"};
	
	NPROPERTY(Login)
	std::wstring Login{};
	
	NPROPERTY(Password)
	std::wstring Password{};
};

