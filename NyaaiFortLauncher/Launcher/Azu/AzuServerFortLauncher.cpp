#include "AzuServerFortLauncher.h"

#include "Launcher/DetectFortniteCrashActivity.h"
#include "Launcher/ReadFortniteLogActivity.h"
#include "Launcher/Actions/CreateProcessAction.h"
#include "Launcher/Actions/InjectDllIntoFortniteAction.h"
#include "Launcher/Actions/PrintLogAction.h"
#include "Launcher/Actions/RequestRelaunchAction.h"
#include "Launcher/Actions/RunCommandAction.h"

GENERATE_BASE_CPP(NAzuServerFortLauncher)

void NAzuServerFortLauncher::OnCreated()
{
	if (!std::filesystem::exists(FortnitePath))
	{
		Log(Error, L"The fortnite path '{}' doesn't exist", FortnitePath.wstring());
		return;
	}

	if (!std::filesystem::is_directory(FortnitePath))
	{
		Log(Error, L"The fortnite path '{}' isn't a directory", FortnitePath.wstring());
		return;
	}

	FortniteExePath = FortnitePath / L"FortniteGame\\Binaries\\Win64\\FortniteClient-Win64-Shipping.exe";
	
	// += so stuff can still be added in the config
	FortniteLaunchArguments +=
		L" -epicapp=Fortnite -epicenv=Prod -epiclocale=en-us -epicportal -skippatchcheck -nobe -fromfl=eac -fltoken=3db3ba5dcbd2e16703f3978d"
		L" -AUTH_TYPE=epic -nosplash -nullrhi -nosound";
	
	FortniteLaunchArguments += std::format(L" -AUTH_LOGIN={} -AUTH_PASSWORD={}", Login, Password);

	{
		TObjectTemplate<NCreateProcessAction> Template{NCreateProcessAction::StaticClass()};
		
		Template->FilePath = FortnitePath / L"FortniteGame\\Binaries\\Win64\\FortniteLauncher.exe";
		Template->bCreateSuspended = true;

		PreFortniteLaunchActions.emplace_back(Template);
	}

	{
		TObjectTemplate<NCreateProcessAction> Template{NCreateProcessAction::StaticClass()};

		Template->FilePath = FortnitePath / L"FortniteGame\\Binaries\\Win64\\FortniteClient-Win64-Shipping_EAC.exe";
		Template->bCreateSuspended = true;

		PreFortniteLaunchActions.emplace_back(Template);
	}

	{
		TObjectTemplate<NInjectDllIntoFortniteAction> Template{NInjectDllIntoFortniteAction::StaticClass()};

		Template->DllPath = RedirectDllPath;
		Template->DllThreadDescription = RedirectDllThreadDescription;
		
		PostFortniteLaunchActions.emplace_back(Template);
	}
	
	{
		TObjectTemplate<NInjectDllIntoFortniteAction> Template{NInjectDllIntoFortniteAction::StaticClass()};

		Template->DllPath = GameServerDllPath;
		Template->DllThreadDescription = AzuGameModeClass;
		
		PostFortniteLaunchActions.emplace_back(Template);
	}
	
	{
		TObjectTemplate<NReadFortniteLogActivity> ActivityTemplate{NReadFortniteLogActivity::StaticClass()};
		
		ActivityTemplate->bPrintFortniteLog = true;
		ActivityTemplate->ColoredPrintPrefix = L"[Azu";
		ActivityTemplate->bOnlyPrintLogWithColoredPrintPrefix = false;
		
		{
			FLogTriggeredAction LogTriggeredAction{};
			
			LogTriggeredAction.TriggerString = L"[UFortOnlineAccount::LoginStepUpdated] CreatingParty --> Completed";
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
		
		Activities.emplace_back(ActivityTemplate);
	}
	
	Activities.emplace_back(NDetectFortniteCrashActivity::StaticClass());

	Super::OnCreated();
}
