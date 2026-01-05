#include "CommandManager.h"

#include <cwctype>

#include "FortLauncher.h"
#include "Engine.h"

FCommandArguments::FCommandArguments(const std::wstring& RawString) : RawString(RawString)
{
    if (RawString.empty())
    {
        return;
    }

    Tokens.clear();

    std::wstring Current{};
    bool bIsInQuotes = false;
    bool bShouldEscapeNext = false;

    for (wchar_t Char : RawString)
    {
        if (bShouldEscapeNext)
        {
            Current.push_back(Char);
            bShouldEscapeNext = false;
            continue;
        }

        if (Char == L'\\')
        {
            bShouldEscapeNext = true;
            continue;
        }

        if (Char == L'"')
        {
            bIsInQuotes = !bIsInQuotes;
            continue;
        }

        if (!bIsInQuotes && std::iswspace(static_cast<wint_t>(Char)))
        {
            if (!Current.empty())
            {
                Tokens.push_back(std::move(Current));
                Current.clear();
            }
        }
        else
        {
            Current.push_back(Char);
        }
    }

    if (!Current.empty())
    {
        Tokens.push_back(std::move(Current));
    }
}

std::wstring FCommandArguments::GetArgumentAtIndex(uint8 Index) const
{
    if (Index >= static_cast<uint32>(Tokens.size()))
    {
        Log(Error, L"Invalid command argument index");
        return {};
    }

    return Tokens[Index];
}

void FCommandManager::NotifyObjectDestroyed(NEngineObject* Object)
{
    for (int32 i = static_cast<int32>(RegisteredCommands.size()) - 1; i >= 0; i--)
    {
        if (RegisteredCommands[i].OwningObject != Object)
        {
            continue;
        }

        RegisteredCommands[i] = std::move(RegisteredCommands.back());
        RegisteredCommands.pop_back();
    }
    
    for (auto& Entry : ManuallyQueuedCommands)
    {
        if (Entry.ContextObject && Entry.ContextObject->IsObjectPartOfOuterChain(Object))
        {
            Entry.ContextObject = Cast<NEngineObject>(Object->GetOuter());
        }
    }
}

FRegisteredCommand* FCommandManager::FindRegisteredCommand(const std::wstring& Command, const NEngineObject* ContextObject)
{
    std::wstring CommandLower = Command;
    std::ranges::transform(CommandLower, CommandLower.begin(),
                           [](wchar_t c) { return static_cast<wchar_t>(std::towlower(static_cast<wint_t>(c))); });

    for (auto& Entry : RegisteredCommands)
    {
        if (!Entry.OwningObject->IsObjectPartOfOuterChain(ContextObject) && !ContextObject->IsObjectPartOfOuterChain(Entry.OwningObject))
        {
            continue;
        }
        
        std::wstring EntryCommandToLower = Entry.Command;
        std::ranges::transform(EntryCommandToLower, EntryCommandToLower.begin(),
                               [](wchar_t c) { return static_cast<wchar_t>(std::towlower(static_cast<wint_t>(c))); });

        if (EntryCommandToLower == CommandLower)
        {
            return &Entry;
        }
    }

    return nullptr;
}

void FCommandManager::ProcessCommands(const NEngineObject* ContextObject)
{
    while (auto RawCommand = GetPendingCommand())
    {
        const auto FirstSpace = std::ranges::find_if(*RawCommand,
            [](wchar_t ch)
            {
                return std::iswspace(static_cast<wint_t>(ch));
            });

        const std::wstring Command{ RawCommand->begin(), FirstSpace };

        if (Command.empty())
        {
            continue;
        }

        FRegisteredCommand* RegisteredCommand = FindRegisteredCommand(Command, ContextObject);
        if (!RegisteredCommand)
        {
            Log(Error, L"Unable to find command '{}'. Type 'help' to list available commands", Command);
            continue;
        }

        auto ArgsBegin = std::find_if_not(
            FirstSpace, RawCommand->end(),
            [](wchar_t ch) { return std::iswspace(static_cast<wint_t>(ch)); });

        const std::wstring ArgString{ ArgsBegin, RawCommand->end() };

        FCommandArguments Args(ArgString);
        RegisteredCommand->ExecuteCommandFunction(RegisteredCommand->OwningObject, Args);
    }
}

void FCommandManager::EnqueueCommand(const std::wstring& Command, NEngineObject* ContextObject)
{
    FManuallyQueuedCommand ManuallyQueuedCommand{};
    ManuallyQueuedCommand.Command = Command;
    ManuallyQueuedCommand.ContextObject = ContextObject;
    
    ManuallyQueuedCommands.push_back(std::move(ManuallyQueuedCommand));
}

void FCommandManager::ProcessManuallyQueuedCommands(NEngine* Engine)
{
    for (auto& Entry : ManuallyQueuedCommands)
    {
        const auto FirstSpace = std::ranges::find_if(Entry.Command,
            [](wchar_t ch)
            {
                return std::iswspace(static_cast<wint_t>(ch));
            });

        const std::wstring Command{ Entry.Command.begin(), FirstSpace };

        if (Command.empty())
        {
            Log(Error, L"Manually queued command was empty");
            continue;
        }

        if (!Entry.ContextObject)
        {
            Entry.ContextObject = Engine;
        }
        
        FRegisteredCommand* RegisteredCommand = FindRegisteredCommand(Command, Entry.ContextObject);
        if (!RegisteredCommand)
        {
            Log(Error, L"Unable to find command '{}'", Command);
            continue;
        }
        
        auto ArgsBegin = std::find_if_not(
            FirstSpace, Entry.Command.end(),
            [](wchar_t ch) { return std::iswspace(static_cast<wint_t>(ch)); });

        const std::wstring ArgString{ ArgsBegin, Entry.Command.end() };

        FCommandArguments Args(ArgString);
        RegisteredCommand->ExecuteCommandFunction(RegisteredCommand->OwningObject, Args);
    }
    
    ManuallyQueuedCommands.clear();
}

std::vector<FRegisteredCommand> FCommandManager::GetRegisteredCommandsForContext(const NEngineObject* ContextObject) const
{
    std::vector<FRegisteredCommand> Result{};
    
    for (const auto& Command : RegisteredCommands)
    {
        if (!Command.OwningObject->IsObjectPartOfOuterChain(ContextObject) && !ContextObject->IsObjectPartOfOuterChain(Command.OwningObject))
        {
            continue;
        }
        
        Result.emplace_back(Command);
    }
    
    return Result;
}
