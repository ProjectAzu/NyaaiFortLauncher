#pragma once

#include <functional>

#include "EngineObject.h"

struct FCommandArguments
{
    FCommandArguments(const std::wstring& RawString);

    std::wstring GetArgumentAtIndex(uint8 Index) const;

    template<class T>
    inline T GetArgumentAtIndex(uint8 Index) const
    {
        if constexpr (std::is_same_v<T, std::wstring>)
        {
            // TTypeHelpers<std::wstring>::SetFromString expects string in c++ string literal style
            return GetArgumentAtIndex(Index);
        }
        
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
    NEngineObject* OwningObject = nullptr;
    std::wstring Command{};
    std::wstring Description{};
    std::function<void(NObject*, const FCommandArguments&)> ExecuteCommandFunction{};
};

struct FManuallyQueuedCommand
{
    std::wstring Command{};
    NEngineObject* ContextObject = nullptr;
};

struct FCommandManager
{
    FCommandManager() = default;
    
    FCommandManager(const FCommandManager&) = delete;
    FCommandManager& operator=(const FCommandManager&) = delete;
    
    void NotifyObjectDestroyed(NEngineObject* Object);
    
    FRegisteredCommand* FindRegisteredCommand(const std::wstring& Command, const NEngineObject* ContextObject);
    void ProcessCommands(const NEngineObject* ContextObject);
    
    void EnqueueCommand(const std::wstring& Command, NEngineObject* ContextObject);
    
    void ProcessManuallyQueuedCommands(NEngine* Engine);
    
    std::vector<FRegisteredCommand> GetRegisteredCommandsForContext(const NEngineObject* ContextObject) const;
    
    template<class T>
    void RegisterConsoleCommand(T* Object, std::wstring Command, std::wstring Description, void(T::*Function)(const FCommandArguments& Args))
    {
        static_assert(std::is_base_of_v<NEngineObject, T>, "T has to be derived from NEngineObject");
        
        if (FindRegisteredCommand(Command, Object))
        {
            Log(Error, L"Cannot register command '{}' because it's already registered", Command);
            return;
        }
        
        FRegisteredCommand RegisteredCommand{};
        RegisteredCommand.OwningObject = Object;
        RegisteredCommand.Command = std::move(Command);
        RegisteredCommand.Description = std::move(Description);
        RegisteredCommand.ExecuteCommandFunction = [Function](NObject* Object, const FCommandArguments& Args)
        {
            (reinterpret_cast<T*>(Object)->*Function)(Args);
        };

        RegisteredCommands.push_back(std::move(RegisteredCommand));
    }
    
private:
    std::vector<FRegisteredCommand> RegisteredCommands{};
    std::vector<FManuallyQueuedCommand> ManuallyQueuedCommands{};
};
