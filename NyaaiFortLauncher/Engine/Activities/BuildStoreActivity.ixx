module;
#include "Object/ReflectionMacros.h"

export module BuildStoreActivity;

export import Engine;
export import SaveRecords;

export class NBuildStoreSaveRecord : public NSaveRecord
{
    ObjectBase(NBuildStoreSaveRecord)

public:
    ReflectedMember(SelectedBuildPath)
    std::filesystem::path SelectedBuildPath{};
};

export class NBuildStoreActivity : public NActivity
{
    ObjectBase(NBuildStoreActivity)

public:
    virtual void OnCreated() override;

    std::optional<std::filesystem::path> GetSelectedFortniteBuildPath() const;

private:
    void ListBuildsCommand(const FCommandArguments& Args);
    void QuerySelectedBuildCommand(const FCommandArguments& Args);
    void SelectBuildCommand(const FCommandArguments& Args);

    void ListBuilds() const;

public:
    ReflectedMember(FortniteBuildPaths)
    std::vector<std::filesystem::path> FortniteBuildPaths{};

    ReflectedMember(bSaveBuildSelectionToDisk)
    bool bSaveBuildSelectionToDisk = true;

private:
    uint32 SelectedBuildIndex = 0;

    NBuildStoreSaveRecord* SaveRecord = nullptr;
};
