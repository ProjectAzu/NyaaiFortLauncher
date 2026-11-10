module;
#include "Object/ReflectionMacros.h"

export module Engine:FortLauncher;

export import ObjectTemplate;
export import :Action;
export import :Activity;
export import :CommandManager;

export class NFortLauncher : public NActivity
{
    ObjectBase(NFortLauncher)

public:
    virtual void OnCreated() override;
    virtual void OnDestroyed() override;

    virtual void Tick(double DeltaTime) override;

    inline void* GetFortniteProcessHandle() const { return FortniteProcessHandle; }
    inline void* GetFortniteStdOutReadPipeHandle() const { return FortniteStdOutReadPipeHandle; }
    inline void* GetFortniteStdInWritePipeHandle() const { return FortniteStdInWritePipeHandle; }

    int32 GetLauncherId() const { return LauncherId; }

    void RequestStop() { bWantsToStop = true; }
    bool DoesWantToStop() const { return bWantsToStop; }

private:
    bool RunLauncher();
    bool LaunchFortniteProcess();

    void StopCommand(const FCommandArguments& Args);
    void ExecuteActionOnLauncherCommand(const FCommandArguments& Args);

public:
    ReflectedMember(FortniteBuildPath)
    std::filesystem::path FortniteBuildPath{};

    ReflectedMember(FortniteExePath)
    std::filesystem::path FortniteExePath = L"FortniteGame\\Binaries\\Win64\\FortniteClient-Win64-Shipping.exe";

    ReflectedMember(FortniteLaunchArguments)
    std::wstring FortniteLaunchArguments{};

    ReflectedMember(OnLauncherCreatedActions)
    std::vector<TObjectTemplate<NAction>> OnLauncherCreatedActions{};

    ReflectedMember(PreFortniteLaunchActions)
    std::vector<TObjectTemplate<NAction>> PreFortniteLaunchActions{};

    ReflectedMember(PostFortniteLaunchActions)
    std::vector<TObjectTemplate<NAction>> PostFortniteLaunchActions{};

    ReflectedMember(OnLauncherExitActions)
    std::vector<TObjectTemplate<NAction>> OnLauncherExitActions{};

    ReflectedMember(Activities)
    std::vector<TObjectTemplate<NActivity>> Activities{};

private:
    FUniqueHandle FortniteProcessHandle = nullptr;
    FUniqueHandle FortniteStdOutReadPipeHandle = nullptr;
    FUniqueHandle FortniteStdInWritePipeHandle = nullptr;

    bool bWantsToStop = false;

    int32 LauncherId = 0;

};
