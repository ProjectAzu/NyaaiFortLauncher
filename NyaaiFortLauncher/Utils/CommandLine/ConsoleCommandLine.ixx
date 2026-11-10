module;
#include "Object/ReflectionMacros.h"

export module ConsoleCommandLine;

export import CommandLine;

export class NConsoleCommandLine : public NCommandLine
{
    ObjectBase(NConsoleCommandLine)

public:
    virtual void OnCreated() override;
    virtual void OnDestroyed() override;
    virtual void Log(const std::wstring& Message) override;

private:
    void InputWorker(std::stop_token StopToken);
    void Write(std::wstring_view Text);
    void RedrawInput();
    void ClearInput();

    std::jthread InputThread{};
    std::mutex OutputMutex{};
    void* InputHandle = nullptr;
    void* OutputHandle = nullptr;
    unsigned long OriginalInputMode = 0;
    unsigned long OriginalOutputMode = 0;
    bool bConsoleInput = false;
    bool bConsoleOutput = false;
    bool bPromptVisible = false;
    std::wstring Input{};
    size_t Cursor = 0;
    std::vector<std::wstring> History{};
    size_t HistoryIndex = 0;
    std::wstring SavedInput{};
    std::wstring PendingLog{};
};
