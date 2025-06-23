#include "DetectFortniteCrashActivity.h"

#include <fstream>
#include <regex>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "FortLauncher.h"

GENERATE_BASE_CPP(NDetectFortniteCrashActivity)

static bool IsFileInUse(const std::filesystem::path& p)
{
    HANDLE h = ::CreateFileW(
        p.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (h == INVALID_HANDLE_VALUE)
    {
        const DWORD err = ::GetLastError();
        return err == ERROR_SHARING_VIOLATION || err == ERROR_LOCK_VIOLATION;
    }

    ::CloseHandle(h);
    return false;
}

void NDetectFortniteCrashActivity::OnCreated()
{
    Super::OnCreated();

    FortniteProcessId = GetProcessId(GetLauncher()->GetFortniteProcessHandle());
    if (FortniteProcessId == 0)
    {
        return;
    }
    
    char* Buffer = nullptr;
    size_t Size = 0;

    if (_dupenv_s(&Buffer, &Size, "LOCALAPPDATA") || !Buffer)
    {
        return;
    }

    FortniteCrashesFolderPath =
        std::filesystem::path{Buffer} / "FortniteGame" / "Saved" / "Crashes";

    free(Buffer);

    if (!std::filesystem::exists(FortniteCrashesFolderPath) ||
        !std::filesystem::is_directory(FortniteCrashesFolderPath))
    {
        return;
    }

    for (const auto& Entry : std::filesystem::recursive_directory_iterator(FortniteCrashesFolderPath))
    {
        if (Entry.is_regular_file() && Entry.path().filename() == "CrashContext.runtime-xml")
        {
            AlreadyCheckedCrashContextFilePaths.insert(Entry.path());
        }
    }

    CrashDirChangeHandle = ::FindFirstChangeNotificationW(
        FortniteCrashesFolderPath.c_str(),
        TRUE,
        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE |
            FILE_NOTIFY_CHANGE_LAST_WRITE);
}

void NDetectFortniteCrashActivity::OnDestroyed()
{
    Super::OnDestroyed();

    if (CrashDirChangeHandle != INVALID_HANDLE_VALUE)
    {
        ::FindCloseChangeNotification(CrashDirChangeHandle);   
    }
}

void NDetectFortniteCrashActivity::Tick(double /*DeltaTime*/)
{
    if (FortniteProcessId == 0)
    {
        return;   
    }
    
    if (CrashDirChangeHandle == INVALID_HANDLE_VALUE)
    {
        return;   
    }

    const DWORD Result = ::WaitForSingleObject(CrashDirChangeHandle, 0);
    const bool DirectoryChanged = Result == WAIT_OBJECT_0;
    if (DirectoryChanged)
    {
        ::FindNextChangeNotification(CrashDirChangeHandle);   
    }

    if (!DirectoryChanged && PendingCrashContextFilePaths.empty())
    {
        return;   
    }

    std::vector<std::filesystem::path> CrashContextFilePaths{};
    
    if (DirectoryChanged)
    {
        for (const auto& Entry : std::filesystem::recursive_directory_iterator(FortniteCrashesFolderPath))
        {
            const auto& Path = Entry.path();

            if (!Entry.is_regular_file() || Path.filename() != "CrashContext.runtime-xml")
            {
                continue;   
            }

            if (AlreadyCheckedCrashContextFilePaths.contains(Path))
            {
                continue;   
            }

            if (IsFileInUse(Path))
            {
                PendingCrashContextFilePaths.insert(Path);
                continue;
            }

            CrashContextFilePaths.push_back(Path);
        }
    }
    
    for (auto it = PendingCrashContextFilePaths.begin(); it != PendingCrashContextFilePaths.end();)
    {
        if (IsFileInUse(*it))
        {
            ++it;
            continue;
        }

        CrashContextFilePaths.push_back(*it);
        it = PendingCrashContextFilePaths.erase(it);
    }
    
    for (const auto& CrashContextFilePath : CrashContextFilePaths)
    {
        std::ifstream InFileStream(CrashContextFilePath, std::ios::binary);
        std::string   Xml{std::istreambuf_iterator<char>(InFileStream), {}};
        std::smatch   M{};

        if (!std::regex_search(Xml, M,
                               std::regex(R"(<ProcessId>(\d+)</ProcessId>)")) ||
            M.size() < 2)
        {
            AlreadyCheckedCrashContextFilePaths.insert(CrashContextFilePath);
            continue;
        }

        const uint32 CrashProcessId = std::stoul(M[1]);

        AlreadyCheckedCrashContextFilePaths.insert(CrashContextFilePath);

        if (CrashProcessId != FortniteProcessId)
        {
            continue;
        }

        Log(Error, "Detected fortnite crash: 'file:///{}'",
            CrashContextFilePath.parent_path().generic_string());

        // Wait for fortnite to exit naturally so it can save the crash dump
        ::WaitForSingleObject(GetLauncher()->GetFortniteProcessHandle(), 5000);

        for (const auto& ActionTemplate : OnFortniteCrashActions)
        {
            NUniquePtr<NAction> Action = ActionTemplate.NewObject(this);   
        }

        return;
    }
}
