#pragma once

#include "Launcher/Activity.h"

#include <vector>
#include <filesystem>

#include "Launcher/SaveRecord.h"

struct FCommandArguments;

class NBuildStoreSaveRecord : public NSaveRecord
{
    GENERATE_BASE_H(NBuildStoreSaveRecord)
    
public:
    NPROPERTY(SelectedBuildPath)
    std::filesystem::path SelectedBuildPath{};
};

class NBuildStoreActivity : public NActivity
{
    GENERATE_BASE_H(NBuildStoreActivity)
    
public:
    virtual void OnCreated() override;
    
    std::optional<std::filesystem::path> GetSelectedFortniteBuildPath() const;
    
private:
    void ListBuildsCommand(const FCommandArguments& Args);
    void QuerySelectedBuildCommand(const FCommandArguments& Args);
    void SelectBuildCommand(const FCommandArguments& Args);
    
    void ListBuilds() const;
    
public:
    NPROPERTY(FortniteBuildPaths)
    std::vector<std::filesystem::path> FortniteBuildPaths{};
    
    NPROPERTY(bSaveBuildSelectionToDisk)
    bool bSaveBuildSelectionToDisk = true;
    
private:
    uint32 SelectedBuildIndex = 0;
    
    NBuildStoreSaveRecord* SaveRecord = nullptr;
};
