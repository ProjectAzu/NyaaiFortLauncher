#include "EngineObject.h"

#include "Engine.h"
#include "FortLauncher.h"
#include "Utils/FileSystem.h"
#include "Utils/WindowsInclude.h"

GENERATE_BASE_CPP(NEngineObject)

void NEngineObject::OnDestroyed()
{
    Super::OnDestroyed();
    
    GetEngine()->NotifyObjectDestroyed(this); 
    
    for (auto& ChildProcessHandle : ChildProcesses)
    {
        if (ChildProcessHandle)
        {
            TerminateProcess(ChildProcessHandle.Get(), 0);
            ChildProcessHandle.Reset();
        }
    }
}

NEngine* NEngineObject::GetEngine() const
{
    return GetObjectOfTypeFromOuterChain<NEngine>(false);
}

NFortLauncher* NEngineObject::GetLauncher() const
{
    return GetObjectOfTypeFromOuterChain<NFortLauncher>(false);
}

FCommandManager& NEngineObject::GetCommandManager() const
{
    return GetEngine()->GetCommandManager();
}

FSaveRecordsSystem& NEngineObject::GetSaveRecordsSystem() const
{
    return GetEngine()->GetSaveRecordsSystem();
}

void NEngineObject::AddChildProcess(FUniqueHandle Handle)
{
    ChildProcesses.push_back(std::move(Handle));
}

std::filesystem::path NEngineObject::ResolvePossiblyFortniteBuildRelativePath(const std::filesystem::path& Path) const
{
    std::vector<std::filesystem::path> AbsoluteSearchRoots{};
    
    if (auto Launcher = GetLauncher())
    {
        if (!Launcher->FortniteBuildPath.empty() && Launcher->FortniteBuildPath.is_absolute())
        {
            AbsoluteSearchRoots.push_back(Launcher->FortniteBuildPath);
        }
    }
    
    std::filesystem::path Result{};
    Utils::ConvertPathToAbsolutePath(Path.wstring(), AbsoluteSearchRoots, Result);
    return Result;
}
