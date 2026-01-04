#pragma once

#include "Object/Object.h"
#include "Utils/UniqueHandle.h"

class NFortLauncher;
class NEngine;
struct FCommandManager;

class NEngineObject : public NObject
{
    GENERATE_BASE_H(NEngineObject)
    
public:
    virtual void OnDestroyed() override;
    
    NEngine* GetEngine() const;
    NFortLauncher* GetLauncher() const;
    
    FCommandManager& GetCommandManager() const;
    
    void AddChildProcess(FUniqueHandle Handle);
    
private:
    std::vector<FUniqueHandle> ChildProcesses{};
};
