# NyaaiFortLauncher

NyaaiFortLauncher is a highly customizable CLI launcher for fortnite.

By default all it will do is start the fortnite process, the rest is to be configured in a config file.

## Notable features

### Printing the fortnite log in the same command line as the launcher lives in

- Color fortnite log that contains a predefined prefix. In the image that prefix set to "[Azu".
- Add actions triggered by the contents of the fortnite log.

![PrintingFortniteLog](Images/PrintingFortniteLog.png)

### Forwarding commands to fortntite

The "fn" command will put everything after fn into the stdin of fortnite. All you have to do in your dll to read commands is to std::cin.

![ForwardCommandToFortnite](Images/ForwardCommandsToFortnite.png)

### Dectect fortnite crashes

This works by cheking new entries in FortniteGame\Saved\Crashes\ and checking if the process id inside CrashContext.runtime-xml matches the one started by the launcher.

It's really useful cause it prints a clickable link to the folder with the crash .dmp, so you can open it right away.

![FortniteCrashSummary](Images/FortniteCrash.png)

## Usage

NyaaiFortLauncher works by launching .nfort config files.
A config file is a text file that defines a NEngine object (`TObjectTemplate<NEngine>`).

**You can run NyaaiFortLauncher.exe --help to see the list of classes or you can look in the code**

You should associate .nfort config files with NyaaiFortLauncher.exe so you can double click configs to launch them.

In the below example I use the NBasicEngine, which is an engine that manages one launcher instance (The program supports multiple instanes).

NyaaiFortLauncher is not bound to a specific launcher instance. In the NBasicEngine you can use the stop command to stop a launcher instance, and then you can start a new one using the start command. This is useful when frequently recompiling dlls during developement, you dont have to open and close a console, you can just keep one open all the time.

Config example:
```
NBasicEngine
{
	bAutoStartLauncher: {true}

	LauncherTemplate:
	{
		NFortLauncher
		{
			FortniteBuildPath: {"15.30\\"}
		
			FortniteLaunchArguments:
			{
				"-epicapp=Fortnite -epicenv=Prod -epiclocale=en-us -epicportal -skippatchcheck"
				" -nobe -fromfl=eac -fltoken=3db3ba5dcbd2e16703f3978d"
				" -AUTH_LOGIN=example@example.com -AUTH_PASSWORD=example -AUTH_TYPE=epic"
				" -nosplash"
			}
			
			PreFortniteLaunchActions:
			{
				{
					NCreateProcessAction
					{
						FilePath: {"FortniteGame\\Binaries\\Win64\\FortniteLauncher.exe"} 
						bCreateSuspended: {true}
					}
				}
				{
					NCreateProcessAction
					{
						FilePath: {"FortniteGame\\Binaries\\Win64\\FortniteClient-Win64-Shipping_EAC.exe"} 
						bCreateSuspended: {true}
					}
				}
			}
			
			PostFortniteLaunchActions:
			{
				{
					NInjectDllIntoFortniteAction
					{
						DllPath: {"G:\\fn\\dlls\\CobaltThreadDescriptionUrl.dll"}
						DllThreadDescription: {"http 127.0.0.1 3551"}
					}
				}
			}
			
			Activities:
			{
				{
					NReadFortniteLogActivity
					{
						bPrintFortniteLog: {true}
						ColoredPrintPrefix: {"[Azu"}
						bOnlyPrintLogWithColoredPrintPrefix: {false}
						LogTriggeredActions:
						{
							{
								TriggerString: {"Region "} 
								bTriggerOnlyOnce: {true}
								Action: 
								{
									NInjectDllIntoFortniteAction { DllPath: {"C:\\fn\\ProjectAzuV2\\Client\\Build\\Client-Release.dll"} }
								}
							}
						}
					}
				}
				{
					NDetectFortniteCrashActivity
				}
			}
		}
	}
}
```

The syntax should be quite self-explanatory from the example.
The syntax for `TObjectTemplate<SomeClass>` (what the config is) is:
```
ClassDerivedFromSomeClass
{
    MemberPropertyName: {Value}
}
```
You specify the class and overrides for its member properties.

The syntax for arrays is the following:
```
{
    {Elem1Value}
}
{ {Elem2Value} }
{
    {Elem3Value}
}
```
There is no commas between elements, you can do newlines and spaces as you wish.

**Paths can be relative to the location of the config, or relative to the fortnite build path.**

## Actions
**Actions are events you can fire (Like a function call).**

In the example config I run two NCreateProcessAction's to create suspended FortniteLauncher.exe and FortniteClient-Win64-Shipping_EAC.exe processes.

I use the NInjectDllIntoFortniteAction to inject a redirect right after the game launches.

I also set a NInjectDllIntoFortniteAction action to execute when the string "Region " apppears in the fortnite log (This happens when fortnite gets to the logging in screen) to inject my client dll.

## Activities
**Activities are objects that have a lifetime and are ticked.**

In the example config I add the NReadFortniteLogActivity activity to run on the launcher, which prints the fortnite log and gives me the ability to further bind actions when certain phrases appear in the fortnite log.

I also add the NDetectFortniteCrashActivity, which is prints info about fortnite crashes when they happen.

## One config for multiple builds

To be able to define one config for a lot of builds add a NBuildStoreActivity to the engine like so:
```
NBasicEngine
{
	Activities:
	{
		{
			NBuildStoreActivity
			{
				FortniteBuildPaths:
				{
					{"15.30\\"}
                    {"13.40\\"}
					{"10.40\\"}
				}
			}
		}
	}
}
```

When a launcher instance starts it will pull the selected build from the build store.

You can change the selected build with the `SelectBuild {Name or Index}` command. You can print the list of builds with the `ListBuilds` command.

You might also want to set `bAutoStartLauncher: {false}` on the engine, so you can select the build before starting.

You can set the default build in the build store by doing `SelectedBuildIndex: {Index}`. By default it's set to 0.