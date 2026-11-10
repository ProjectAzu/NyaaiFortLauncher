module;
#include "Object/ReflectionMacros.h"

export module Engine:EngineObject;

export import Reflection;
export import SaveRecords;

export class NFortLauncher;
export class NEngine;
export struct FCommandManager;

export class NEngineObject : public NObject
{
    ObjectBase(NEngineObject)

public:
    virtual void OnDestroyed() override;

    NEngine* GetEngine() const;
    NFortLauncher* GetLauncher() const;

    FCommandManager& GetCommandManager() const;
    FSaveRecordsSystem& GetSaveRecordsSystem() const;

    void AddChildProcess(FUniqueHandle Handle);

    std::filesystem::path ResolvePossiblyFortniteBuildRelativePath(const std::filesystem::path& Path, bool bRequireExists = true) const;

private:
    std::vector<FUniqueHandle> ChildProcesses{};
};
