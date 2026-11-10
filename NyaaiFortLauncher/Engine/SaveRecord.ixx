module;
#include "Object/ReflectionMacros.h"

export module SaveRecords;

export import Reflection;
export import SubClassOf;

export class NSaveRecord : public NObject
{
    ObjectBase(NSaveRecord)

public:
    void InitRecordPath(const std::wstring& InPath) { RecordPath = InPath; }
    std::wstring GetRecordPath() const { return RecordPath; }

private:
    ReflectedMember(RecordPath)
    std::wstring RecordPath{};
};

export struct FSaveRecordsSystem
{
    ReflectedClassBase(FSaveRecordsSystem)

private:
    void Initialize();

public:
    void SaveRecordsToDisk();

    NSaveRecord* GetSaveRecord(NObject* Owner, TSubClassOf<NSaveRecord> Class);
    template <class T>
    T* GetSaveRecord(NObject* Owner)
    {
        return rcast<T*>(GetSaveRecord(Owner, T::StaticClass()));
    }

public:
    ReflectedMember(SaveFileName)
    std::wstring SaveFileName{};

    ReflectedMember(SaveFilesFolderPathOverride)
    std::filesystem::path SaveFilesFolderPathOverride{};

private:
    std::vector<TUniqueObjectPtr<NSaveRecord>> SaveRecordObjects{};

    std::filesystem::path SaveFilePath{};

    bool bIsInitialized = false;
};
