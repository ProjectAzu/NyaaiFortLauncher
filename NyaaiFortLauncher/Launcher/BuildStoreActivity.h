#pragma once

#include "Activity.h"

#include <vector>
#include <filesystem>

struct FCommandArguments;

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
    
    NPROPERTY(SelectedBuildIndex)
    uint32 SelectedBuildIndex = 0;
};
