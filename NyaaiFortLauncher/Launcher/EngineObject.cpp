#include "EngineObject.h"

#include "Engine.h"
#include "FortLauncher.h"
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

void NEngineObject::AddChildProcess(FUniqueHandle Handle)
{
    ChildProcesses.push_back(std::move(Handle));
}
