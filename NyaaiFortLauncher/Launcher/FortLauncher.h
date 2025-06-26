#pragma once

#include "LauncherObject.h"

#include "Action.h"

#include <filesystem>
#include <unordered_map>
#include <functional>

class NActivity;

struct FCommandArguments
{
    FCommandArguments(const std::string& RawString);

    std::string GetArgumentAtIndex(uint8 Index) const;

    template<class T>
    inline T GetArgumentAtIndex(uint8 Index) const
    {
        T Result{};
        FPropertySetterFunction<T>::Set(&Result, GetArgumentAtIndex(Index));
        return Result;
    }
    
    inline std::string GetRawString() const { return RawString; }
    
private:
    std::vector<std::string> Tokens{};
    std::string RawString{};
};

struct FRegisteredCommand
{
    NLauncherObject* OwningObject = nullptr;
    std::string Command{};
    std::string Description{};
    std::function<void(NLauncherObject*, const FCommandArguments&)> ExecuteCommandFunction{};
};

class NFortLauncher : public NLauncherObject
{
    GENERATE_BASE_H(NFortLauncher)
    
public:
    void OnCreated() override;
    void OnDestroyed() override;

    inline void* GetFortniteProcessHandle() const { return FortniteProcessHandle; }
    inline void* GetFortniteStdOutReadPipeHandle() const { return FortniteStdOutReadPipeHandle; }
    inline void* GetFortniteStdInWritePipeHandle() const { return FortniteStdInWritePipeHandle; }

    inline bool HasFinishedBaseLaunch() const { return bHasFinishedBaseLaunch; }

    void RequestRelaunch();
    void RequestExit();

    void NotifyObjectCreated(NLauncherObject* Object);
    void NotifyObjectDestroyed(NLauncherObject* Object);

    template<class T>
    inline void RegisterConsoleCommand(T* Object, std::string Command, std::string Description, void(T::*Function)(const FCommandArguments& Args))
    {
        static_assert(std::is_base_of_v<NLauncherObject, T>, "T has to be derived from NLauncherObject");

        if (FindRegisteredCommand(Command))
        {
            Log(Error, "Cannot register command '{}' because it's already registered.", Command);
            return;
        }
        
        FRegisteredCommand RegisteredCommand{};
        RegisteredCommand.OwningObject = Object;
        RegisteredCommand.Command = std::move(Command);
        RegisteredCommand.Description = std::move(Description);
        RegisteredCommand.ExecuteCommandFunction = [=](NLauncherObject* Object, const FCommandArguments& Args)
        {
            (reinterpret_cast<T*>(Object)->*Function)(Args);
        };

        RegisteredCommands.push_back(std::move(RegisteredCommand));
    }

private:
    bool RunLauncher();
    bool LaunchFortniteProcess();
    void DoCleanup();

    FRegisteredCommand* FindRegisteredCommand(std::string Command);
    void ProcessCommands();
    
    void HelpCommand(const FCommandArguments& Args);
    void RestartCommand(const FCommandArguments& Args);
    void ExitCommand(const FCommandArguments& Args);
    void ExecuteActionCommand(const FCommandArguments& Args);

    void KillAllChildProcesses();

public:
    NPROPERTY(FortniteExePath)
    std::filesystem::path FortniteExePath{};

    NPROPERTY(FortniteLaunchArguments)
    std::string FortniteLaunchArguments{};

    NPROPERTY(PreFortniteLaunchActions)
    std::vector<FObjectInitializeTemplate<NAction>> PreFortniteLaunchActions{};

    NPROPERTY(PostFortniteLaunchActions)
    std::vector<FObjectInitializeTemplate<NAction>> PostFortniteLaunchActions{};

    NPROPERTY(Activities)
    std::vector<FObjectInitializeTemplate<NActivity>> Activities{};

private:
    void* FortniteProcessHandle = nullptr;
    void* FortniteStdOutReadPipeHandle = nullptr;
    void* FortniteStdInWritePipeHandle = nullptr;

    bool bHasFinishedBaseLaunch = false;

    bool bWantsToRelaunch = false;
    bool bWantsToExit = false;
    
    std::vector<FRegisteredCommand> RegisteredCommands{};
};
