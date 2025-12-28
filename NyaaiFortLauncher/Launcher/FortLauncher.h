#pragma once

#include "LauncherObject.h"

#include "Action.h"

#include <filesystem>
#include <unordered_map>
#include <functional>

#include "Object/ObjectInitializeTemplate.h"

class NActivity;

struct FCommandArguments
{
    FCommandArguments(const std::wstring& RawString);

    std::wstring GetArgumentAtIndex(uint8 Index) const;

    template<class T>
    inline T GetArgumentAtIndex(uint8 Index) const
    {
        T Result{};
        TTypeHelpers<T>::SetFromString(&Result, GetArgumentAtIndex(Index));
        return Result;
    }
    
    inline std::wstring GetRawString() const { return RawString; }
    
private:
    std::vector<std::wstring> Tokens{};
    std::wstring RawString{};
};

struct FRegisteredCommand
{
    NLauncherObject* OwningObject = nullptr;
    std::wstring Command{};
    std::wstring Description{};
    std::function<void(NLauncherObject*, const FCommandArguments&)> ExecuteCommandFunction{};
};

class NFortLauncher : public NLauncherObject
{
    GENERATE_BASE_H(NFortLauncher)
    
public:
    void OnCreated() override;

    inline void* GetFortniteProcessHandle() const { return FortniteProcessHandle; }
    inline void* GetFortniteStdOutReadPipeHandle() const { return FortniteStdOutReadPipeHandle; }
    inline void* GetFortniteStdInWritePipeHandle() const { return FortniteStdInWritePipeHandle; }

    inline bool HasFinishedBaseLaunch() const { return bHasFinishedBaseLaunch; }

    void RequestRelaunch();
    void RequestExit();

    void NotifyObjectCreated(NLauncherObject* Object);
    void NotifyObjectDestroyed(NLauncherObject* Object);

    template<class T>
    inline void RegisterConsoleCommand(T* Object, std::wstring Command, std::wstring Description, void(T::*Function)(const FCommandArguments& Args))
    {
        static_assert(std::is_base_of_v<NLauncherObject, T>, "T has to be derived from NLauncherObject");

        if (FindRegisteredCommand(Command))
        {
            Log(Error, L"Cannot register command '{}' because it's already registered.", Command);
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

    FRegisteredCommand* FindRegisteredCommand(const std::wstring& Command);
    void ProcessCommands();
    
    void HelpCommand(const FCommandArguments& Args);
    void RestartCommand(const FCommandArguments& Args);
    void ExitCommand(const FCommandArguments& Args);
    void ExecuteActionCommand(const FCommandArguments& Args);

    static void KillAllChildProcesses();

public:
    NPROPERTY(FortniteExePath)
    std::filesystem::path FortniteExePath{};

    NPROPERTY(FortniteLaunchArguments)
    std::wstring FortniteLaunchArguments{};

    NPROPERTY(PreFortniteLaunchActions)
    std::vector<TObjectInitializeTemplate<NAction>> PreFortniteLaunchActions{};

    NPROPERTY(PostFortniteLaunchActions)
    std::vector<TObjectInitializeTemplate<NAction>> PostFortniteLaunchActions{};
    
    NPROPERTY(PostFortniteExitActions)
    std::vector<TObjectInitializeTemplate<NAction>> PostFortniteExitActions{};

    NPROPERTY(Activities)
    std::vector<TObjectInitializeTemplate<NActivity>> Activities{};

private:
    void* FortniteProcessHandle = nullptr;
    void* FortniteStdOutReadPipeHandle = nullptr;
    void* FortniteStdInWritePipeHandle = nullptr;

    bool bHasFinishedBaseLaunch = false;

    bool bWantsToRelaunch = false;
    bool bWantsToExit = false;
    
    std::vector<FRegisteredCommand> RegisteredCommands{};
};
