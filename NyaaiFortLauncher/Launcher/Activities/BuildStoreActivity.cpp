#include "BuildStoreActivity.h"

#include "Launcher/CommandManager.h"

GENERATE_BASE_CPP(NBuildStoreSaveRecord)

GENERATE_BASE_CPP(NBuildStoreActivity)

void NBuildStoreActivity::OnCreated()
{
    Super::OnCreated();
    
    if (bSaveBuildSelectionToDisk)
    {
        SaveRecord = GetSaveRecordsSystem().GetSaveRecord<NBuildStoreSaveRecord>(this);
    
        if (!SaveRecord->SelectedBuildPath.empty())
        {
            for (uint32 i = 0; i < FortniteBuildPaths.size(); i++)
            {
                if (FortniteBuildPaths[i] == SaveRecord->SelectedBuildPath)
                {
                    SelectedBuildIndex = i;
                    break;
                }
            }   
        }   
    }
    
    GetCommandManager().RegisterConsoleCommand(
        this,
        L"ListBuilds",
        L"Lists the builds available in the build store",
        &ThisClass::ListBuildsCommand
    );
    
    GetCommandManager().RegisterConsoleCommand(
        this,
        L"SelectBuild",
        L"Usage: SelectBuild {BuildName or Index}",
        &ThisClass::SelectBuildCommand
    );
    
    GetCommandManager().RegisterConsoleCommand(
        this,
        L"QuerySelectedBuild",
        L"Query the currently selected build",
        &ThisClass::QuerySelectedBuildCommand
    );
    
    if (FortniteBuildPaths.empty())
    {
        Log(Error, L"BuildStore is empty, please add at least one build");
        return;
    }
    
    Log(Info, L"BuildStore: Default build is: '{}'.", 
        GetSelectedFortniteBuildPath().value().wstring());
    
    ListBuilds();
}

std::optional<std::filesystem::path> NBuildStoreActivity::GetSelectedFortniteBuildPath() const
{
    if (FortniteBuildPaths.empty())
    {
        return std::nullopt;
    }
    
    if (SelectedBuildIndex < FortniteBuildPaths.size())
    {
        return FortniteBuildPaths[SelectedBuildIndex];
    }
    
    return FortniteBuildPaths.back();
}

void NBuildStoreActivity::ListBuildsCommand(const FCommandArguments& Args)
{
    ListBuilds();
}

void NBuildStoreActivity::QuerySelectedBuildCommand(const FCommandArguments& Args)
{
    auto SelectedBuild = GetSelectedFortniteBuildPath();
    Log(Info, L"Currently selected build: '{}'", SelectedBuild ? SelectedBuild.value().wstring() : L"none");
}

void NBuildStoreActivity::SelectBuildCommand(const FCommandArguments& Args)
{
    std::wstring BuildName = Args.GetArgumentAtIndex<std::wstring>(0);
    if (BuildName.empty())
    {
        Log(Error, L"Provide a build name or index");
        return;
    }
    
    std::wstring BuildNameToLower = BuildName;
    std::ranges::transform(BuildNameToLower, BuildNameToLower.begin(),
                           [](wchar_t c) { return static_cast<wchar_t>(std::towlower(static_cast<wint_t>(c))); });
    
    bool bFoundBuildFromName = false;
    
    for (uint32 i = 0; i < FortniteBuildPaths.size(); i++)
    {
        std::wstring BuildPathString = FortniteBuildPaths[i].wstring();
        
        std::ranges::transform(BuildPathString, BuildPathString.begin(),
                       [](wchar_t c) { return static_cast<wchar_t>(std::towlower(static_cast<wint_t>(c))); });
        
        if (BuildPathString.contains(BuildNameToLower))
        {
            SelectedBuildIndex = i;
            bFoundBuildFromName = true;
            break;
        }
    }
    
    if (!bFoundBuildFromName)
    {
        uint32 Index = Args.GetArgumentAtIndex<uint32>(0);
    
        if (Index >= FortniteBuildPaths.size())
        {
            Log(Error, L"'{}' is not a valid build name or index, use the ListBuilds command to list builds", BuildName);
            return;
        }
        
        SelectedBuildIndex = Index;
    }
    
    std::wstring SelectedBuildPath = GetSelectedFortniteBuildPath().value().wstring();
    
    if (SaveRecord)
    {
        SaveRecord->SelectedBuildPath = SelectedBuildPath;
        GetSaveRecordsSystem().SaveRecordsToDisk();
    }
    
    Log(Info, L"Selected build '{}'", SelectedBuildPath);
}

void NBuildStoreActivity::ListBuilds() const
{
    Log(Info, L"Available builds in build store:");
    
    if (FortniteBuildPaths.empty())
    {
        Log(Info, L"There are currently no builds in the build store");
        return;
    }
    
    for (uint32 i = 0; i < FortniteBuildPaths.size(); i++)
    {
        Log(Info, L"{} - '{}'", i, FortniteBuildPaths[i].wstring());
    }
}
