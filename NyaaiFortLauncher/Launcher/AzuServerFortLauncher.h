#pragma once

#include "FortLauncher.h"

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
	
	NPROPERTY(AzuGameModeClass)
	std::wstring AzuGameModeClass{L"NAzuGameModeSolo"};
};

