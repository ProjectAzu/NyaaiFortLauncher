#include "AzuShippingClientEngine.h"

#include "Launcher/DetectFortniteCrashActivity.h"
#include "Launcher/FortLauncher.h"
#include "Launcher/ReadFortniteLogActivity.h"
#include "Launcher/Actions/CreateProcessAction.h"
#include "Launcher/Actions/InjectDllIntoFortniteAction.h"
#include "Launcher/Actions/PrintLogAction.h"
#include "Launcher/Actions/RequestRelaunchAction.h"
#include "Launcher/Actions/RunCommandAction.h"
#include "Utils/CommandLine/CoutCommandLine.h"

GENERATE_BASE_CPP(NAzuShippingClientEngine)

NAzuShippingClientEngine::NAzuShippingClientEngine()
{
	CommandLineTemplate = NCoutCommandLine::StaticClass();
}

void NAzuShippingClientEngine::OnCreated()
{
    Super::OnCreated();
    
    if (ProgramLaunchArgs.empty())
    {
        Log(Error, L"No build path arg");
        return;
    }
	
    if (!TTypeHelpers<std::filesystem::path>::SetFromString(&FortniteBuildPath, ProgramLaunchArgs[0]))
    {
        Log(Error, L"Fortnite build path '{}' is invalid", ProgramLaunchArgs[0]);
        return;
    }
	
	if (!std::filesystem::exists(FortniteBuildPath))
	{
		Log(Error, L"The fortnite build path '{}' doesn't exist", FortniteBuildPath.wstring());
		return;
	}

	if (!std::filesystem::is_directory(FortniteBuildPath))
	{
		Log(Error, L"The fortnite build path '{}' isn't a directory", FortniteBuildPath.wstring());
		return;
	}
	
    if (ProgramLaunchArgs.size() < 2)
    {
        Log(Error, L"No login arg");
        return;
    }
    Login = ProgramLaunchArgs[1];
	
    if (ProgramLaunchArgs.size() < 3)
    {
        Log(Error, L"No password arg");
        return;
    }
    Password = ProgramLaunchArgs[2];
	
	GetCommandManager().RegisterConsoleCommand(
		this,
		L"restart",
		L"Restarts the launcher",
		&ThisClass::RestartCommand
	);
	
	if (!StartLauncherInstance())
	{
		Log(Error, L"Starting a launcher instance failed, requesting exit");
		RequestExit();
	}
}

void NAzuShippingClientEngine::NotifyLauncherBeingDestroyed(NFortLauncher* Launcher)
{
	Super::NotifyLauncherBeingDestroyed(Launcher);
	
	if (!bIsRestarting)
	{
		RequestExit();
	}
}

bool NAzuShippingClientEngine::StartLauncherInstance()
{
	TObjectTemplate<NFortLauncher> LauncherTemplate{NFortLauncher::StaticClass()};

	LauncherTemplate->FortniteExePath = FortniteBuildPath / L"FortniteGame\\Binaries\\Win64\\FortniteClient-Win64-Shipping.exe";
	
	LauncherTemplate->FortniteLaunchArguments +=
		L"-epicapp=Fortnite -epicenv=Prod -epiclocale=en-us -epicportal -skippatchcheck -nobe -fromfl=eac -fltoken=3db3ba5dcbd2e16703f3978d"
		L" -AUTH_TYPE=epic -nosplash -nullrhi -nosound";
	
	LauncherTemplate->FortniteLaunchArguments += std::format(L" -AUTH_LOGIN={} -AUTH_PASSWORD={}", Login, Password);

	{
		TObjectTemplate<NCreateProcessAction> Template{NCreateProcessAction::StaticClass()};
		
		Template->FilePath = FortniteBuildPath / L"FortniteGame\\Binaries\\Win64\\FortniteLauncher.exe";
		Template->bCreateSuspended = true;

		LauncherTemplate->PreFortniteLaunchActions.emplace_back(Template);
	}

	{
		TObjectTemplate<NCreateProcessAction> Template{NCreateProcessAction::StaticClass()};

		Template->FilePath = FortniteBuildPath / L"FortniteGame\\Binaries\\Win64\\FortniteClient-Win64-Shipping_EAC.exe";
		Template->bCreateSuspended = true;

		LauncherTemplate->PreFortniteLaunchActions.emplace_back(Template);
	}
	
	{
		TObjectTemplate<NInjectDllIntoFortniteAction> Template{NInjectDllIntoFortniteAction::StaticClass()};
    	
		Template->DllPath = L"AzuClient.dll";
    	
		LauncherTemplate->PostFortniteLaunchActions.emplace_back(Template);
	}
	
	{
		TObjectTemplate<NReadFortniteLogActivity> ActivityTemplate{NReadFortniteLogActivity::StaticClass()};
		
		ActivityTemplate->bPrintFortniteLog = true;
		ActivityTemplate->ColoredPrintPrefix = L"[Azu";
		ActivityTemplate->bOnlyPrintLogWithColoredPrintPrefix = false;
		
		{
			FLogTriggeredAction LogTriggeredAction{};
			
			LogTriggeredAction.TriggerString = L"Region ";
			LogTriggeredAction.bTriggerOnlyOnce = true;
			
			TObjectTemplate<NRunCommandAction> ActionTemplate{NRunCommandAction::StaticClass()};
			ActionTemplate->Command = L"fn NotifyFortniteHasLoaded";
		
			LogTriggeredAction.Action = ActionTemplate;
			
			ActivityTemplate->LogTriggeredActions.emplace_back(LogTriggeredAction);
		}
		
		{
			FLogTriggeredAction LogTriggeredAction{};
			
			LogTriggeredAction.TriggerString = L"[UOnlineAccountCommon::ForceLogout] ForceLogout";
			LogTriggeredAction.bTriggerOnlyOnce = true;
			
			{
				TObjectTemplate<NPrintLogAction> ActionTemplate{NPrintLogAction::StaticClass()};
				ActionTemplate->LogLevel = L"Error";
				ActionTemplate->Message = L"Fortnite forced logout, relaunching";
		
				LogTriggeredAction.Actions.emplace_back(ActionTemplate);
			}
			
			LogTriggeredAction.Actions.emplace_back(NRequestRelaunchAction::StaticClass());
			
			ActivityTemplate->LogTriggeredActions.emplace_back(LogTriggeredAction);
		}
		
		{
			FLogTriggeredAction LogTriggeredAction{};
			
			LogTriggeredAction.TriggerString = L"errors.com.epicgames.account.invalid_account_credentials";
			
			TObjectTemplate<NPrintLogAction> ActionTemplate{NPrintLogAction::StaticClass()};
			ActionTemplate->LogLevel = L"Error";
			ActionTemplate->Message = L"Invalid fortnite account credentials";
		
			LogTriggeredAction.Action = ActionTemplate;
			
			ActivityTemplate->LogTriggeredActions.emplace_back(LogTriggeredAction);
		}
		
		{
			FLogTriggeredAction LogTriggeredAction{};
			
			LogTriggeredAction.TriggerString = L"request failed, libcurl error: 7 (Couldn't connect to server)";
			
			TObjectTemplate<NPrintLogAction> ActionTemplate{NPrintLogAction::StaticClass()};
			ActionTemplate->LogLevel = L"Error";
			ActionTemplate->Message = L"Fortnite could not connect to the backend";
		
			LogTriggeredAction.Action = ActionTemplate;
			
			ActivityTemplate->LogTriggeredActions.emplace_back(LogTriggeredAction);
		}
		
		LauncherTemplate->Activities.emplace_back(ActivityTemplate);
	}
	
	LauncherTemplate->Activities.emplace_back(NDetectFortniteCrashActivity::StaticClass());
	
	auto Launcher = StartChildActivity<NFortLauncher>(LauncherTemplate);
	
	return !Launcher->DoesWantToStop();
}

void NAzuShippingClientEngine::RestartCommand(const FCommandArguments& Args)
{
	Log(Info, L"Restarting");
	
	bIsRestarting = true;
	
	for (const auto Launcher : GetLauncherInstances())
	{
		Launcher->Destroy();
	}
	
	if (!StartLauncherInstance())
	{
		Log(Error, L"Starting a launcher instance failed, requesting exit");
		RequestExit();
	}
	
	bIsRestarting = false;
}