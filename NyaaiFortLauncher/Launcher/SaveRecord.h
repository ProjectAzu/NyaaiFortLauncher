#pragma once

#include "Object/Object.h"

class NEngineObject;

class NSaveRecord : public NObject
{
    GENERATE_BASE_H(NSaveRecord)
    
public:
    void InitRecordPath(const std::wstring& InPath) { RecordPath = InPath; }
    std::wstring GetRecordPath() const { return RecordPath; }
    
private:
    NPROPERTY(RecordPath)
    std::wstring RecordPath{};
};

struct FSaveRecordsSystem : FStructWithProperties
{
    static std::wstring GetName() { return L"FSaveRecordsSystem"; }
    
private:
    void Initialize();
    
public:
    void SaveRecordsToDisk();
    
    NSaveRecord* GetSaveRecord(NObject* Owner, NSubClassOf<NSaveRecord> Class);
    template<class T> T* GetSaveRecord(NObject* Owner)
    {
        return reinterpret_cast<T*>(GetSaveRecord(Owner, T::StaticClass()));
    }
    
public:
    NPROPERTY(SaveFileName)
    std::wstring SaveFileName{};
    
    NPROPERTY(SaveFilesFolderPathOverride)
    std::filesystem::path SaveFilesFolderPathOverride{};
    
private:
    std::vector<NUniquePtr<NSaveRecord>> SaveRecordObjects{};
    
    std::filesystem::path SaveFilePath{};
    
    bool bIsInitialized = false;
};