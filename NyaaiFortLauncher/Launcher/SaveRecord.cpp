#include "SaveRecord.h"

#include "Utils/FileSystem.h"
#include "Utils/TextFileLoader.h"
#include "Utils/Utf8.h"

#include <fstream>
#include <ranges>

GENERATE_BASE_CPP(NSaveRecord)

void FSaveRecordsSystem::Initialize()
{
    if (SaveFilesFolderPathOverride.empty())
    {
        wchar_t* Buffer = nullptr;
        size_t Size = 0;

        if (_wdupenv_s(&Buffer, &Size, L"LOCALAPPDATA") != 0 || !Buffer)
        {
            return;
        }

        SaveFilePath =
            std::filesystem::path{ Buffer } / L"NyaaiFortLauncher";

        free(Buffer);
    }
    else
    {
        if (!Utils::ConvertPathToAbsolutePath(SaveFilesFolderPathOverride, {}, SaveFilePath))
        {
            return;
        }
        
        if (!std::filesystem::is_directory(SaveFilePath))
        {
            Log(Error, L"FSaveRecordSystem: SaveFilesFolderPathOverride must be a folder path");
            return;
        }
    }
    
    SaveFilePath /= SaveFileName;
    
    if (SaveFilePath.empty() || std::filesystem::is_directory(SaveFilePath))
    {
        Log(Error, L"FSaveRecordSystem: '{}' is not a valid save file path", SaveFilePath.wstring());
        return;
    }
    
    {
        std::error_code ErrorCode{};
        std::filesystem::create_directories(SaveFilePath.parent_path(), ErrorCode);
        
        if (ErrorCode)
        {
            Log(Error, 
                L"FSaveRecordSystem: Could not create directory: '{}'", 
                SaveFilePath.parent_path().wstring());
            
            return;
        }
    }
    
    bIsInitialized = true;
    
    if (!std::filesystem::exists(SaveFilePath))
    {
        return;
    }
    
    std::wstring FileContent = Utils::LoadTextFileToWString(SaveFilePath);
    
    std::vector<TObjectTemplate<NSaveRecord>> SaveRecordTemplates{};
    if (!TTypeHelpers<std::vector<TObjectTemplate<NSaveRecord>>>::SetFromString(&SaveRecordTemplates, FileContent))
    {
        Log(Error, L"FSaveRecordSystem: Loading save record entries failed");
        return;
    }
    
    for (const auto& SaveRecordTemplate : SaveRecordTemplates)
    {
        if (!SaveRecordTemplate->GetRecordPath().ends_with(SaveRecordTemplate->GetClass()->GetName()))
        {
            Log(Error, L"FSaveRecordSystem: Invalid record path");
            continue;
        }
        
        SaveRecordObjects.emplace_back(SaveRecordTemplate.NewObject());
    }
    
    // In case there were any errors, this will overwrite the save with a valid one
    SaveRecordsToDisk();
}

void FSaveRecordsSystem::SaveRecordsToDisk()
{
    if (!bIsInitialized)
    {
        Initialize();
    }
    
    if (!bIsInitialized)
    {
        return;
    }
    
    if (SaveRecordObjects.empty())
    {
        return;
    }
    
    std::vector<TObjectTemplate<NSaveRecord>> SaveRecordTemplates{};
    for (const auto& Record : SaveRecordObjects)
    {
        SaveRecordTemplates.emplace_back(Record->GetClass(), Record->GetDefaultValueOverrides());
    }
    
    std::wstring SaveContentWString = TTypeHelpers<std::vector<TObjectTemplate<NSaveRecord>>>::ToString(&SaveRecordTemplates);
    
    std::string SaveContentUtf8 = WideToUtf8(SaveContentWString);
    
    std::ofstream SaveFileStream{SaveFilePath, std::ios::binary | std::ios::trunc};
    if (!SaveFileStream)
    {
        Log(Error, L"FSaveRecordsSystem::SaveRecordsToDisk couldn't open save file '{}'", SaveFilePath.wstring());
        return;
    }
    
    SaveFileStream.write(SaveContentUtf8.data(), static_cast<std::streamsize>(SaveContentUtf8.size()));
    if (!SaveFileStream)
    {
        Log(Error, L"FSaveRecordsSystem::SaveRecordsToDisk failed writing to file '{}'", SaveFilePath.wstring());
        return;
    }
}

NSaveRecord* FSaveRecordsSystem::GetSaveRecord(NObject* Owner, NSubClassOf<NSaveRecord> Class)
{
    if (!bIsInitialized)
    {
        Initialize();
    }
    
    std::vector<NObject*> OuterChain{};
    
    for (NObject* Object = Owner; Object; Object = Object->GetOuter())
    {
        OuterChain.push_back(Object);
    }
    
    std::wstring RecordPath{};
    
    for (const auto Object : std::ranges::reverse_view(OuterChain))
    {
        RecordPath += Object->GetClass()->GetName();
        RecordPath += L'.';
    }
    
    RecordPath += Class->GetName();
    
    for (const auto& Object : SaveRecordObjects)
    {
        if (Object->GetRecordPath() != RecordPath)
        {
            continue;
        }
        
        if (Object->GetClass() != Class)
        {
            Log(Error, L"Record contains invalid path");
            continue;
        }
        
        return Object;
    }
    
    auto NewRecord = Class->NewObjectRaw<NSaveRecord>();
    NewRecord->InitRecordPath(RecordPath);
    
    SaveRecordObjects.emplace_back(NewRecord);
    
    return NewRecord;
}
