#include "DetectFortniteCrashActivity.h"

#include <regex>
#include <string>
#include <vector>

#include "Utils/WindowsInclude.h"
#include "Utils/TextFileLoader.h"

#include "Launcher/FortLauncher.h"

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
    
    FortniteCrashesFolderPath = ResolvePossiblyFortniteBuildRelativePath(FortniteCrashesFolderPath);
    
    if (FortniteCrashesFolderPath.empty())
    {
        Log(Error, L"NDetectFortniteCrashActivity: FortniteCrashesFolderPath is empty");
        return;
    }
    
    {
        std::error_code ErrorCode{};
        std::filesystem::create_directories(FortniteCrashesFolderPath, ErrorCode);
        
        if (ErrorCode)
        {
            Log(Error, 
                L"NDetectFortniteCrashActivity: Could not create directory: '{}'", 
                FortniteCrashesFolderPath.wstring());
            
            return;
        }
    }
    
    if (!std::filesystem::exists(FortniteCrashesFolderPath))
    {
        return;
    }

    if (!std::filesystem::is_directory(FortniteCrashesFolderPath))
    {
        return;
    }

    for (const auto& Entry : std::filesystem::recursive_directory_iterator(FortniteCrashesFolderPath))
    {
        if (Entry.is_regular_file() && Entry.path().filename().wstring() == L"CrashContext.runtime-xml")
        {
            AlreadyCheckedCrashContextFilePaths.insert(Entry.path());
        }
    }

    CrashDirChangeHandle = FindFirstChangeNotificationW(
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
        FindCloseChangeNotification(CrashDirChangeHandle);
    }
}

void NDetectFortniteCrashActivity::Tick(double DeltaTime)
{
    if (FortniteProcessId == 0)
    {
        return;
    }

    if (CrashDirChangeHandle == INVALID_HANDLE_VALUE)
    {
        return;
    }

    const DWORD Result = WaitForSingleObject(CrashDirChangeHandle, 0);
    const bool DirectoryChanged = Result == WAIT_OBJECT_0;
    if (DirectoryChanged)
    {
        FindNextChangeNotification(CrashDirChangeHandle);
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

            if (!Entry.is_regular_file() || Path.filename() != L"CrashContext.runtime-xml")
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
        std::wstring Xml{};
        try
        {
            Xml = Utils::LoadTextFileToWString(CrashContextFilePath);
        }
        catch (...)
        {
            AlreadyCheckedCrashContextFilePaths.insert(CrashContextFilePath);
            continue;
        }

        std::wsmatch M{};

        if (!std::regex_search(Xml, M, std::wregex(LR"(<ProcessId>(\d+)</ProcessId>)")) ||
            M.size() < 2)
        {
            AlreadyCheckedCrashContextFilePaths.insert(CrashContextFilePath);
            continue;
        }

        const uint32 CrashProcessId = static_cast<uint32>(std::stoul(M[1].str()));

        AlreadyCheckedCrashContextFilePaths.insert(CrashContextFilePath);

        if (CrashProcessId != FortniteProcessId)
        {
            continue;
        }

        std::wstring CrashOffset{};
        std::wstring CrashThread{};
        std::wstring CrashThreadId{};

        std::vector<std::pair<std::wstring, std::wstring>> Frames{};

        if (std::regex_search(Xml, M, std::wregex(LR"(<PCallStack>([\s\S]*?)</PCallStack>)")))
        {
            const std::wstring Pcs = M[1].str();

            std::wsmatch Top{};
            if (!std::regex_search(
                    Pcs,
                    Top,
                    std::wregex(LR"(FortniteClient[-_]?Win64[-_]?Shipping\s+0x[0-9A-Fa-f]+\s+\+\s+([0-9A-Fa-f]+))")))
            {
                std::regex_search(
                    Pcs,
                    Top,
                    std::wregex(LR"((\S+)\s+0x[0-9A-Fa-f]+\s+\+\s+([0-9A-Fa-f]+))"));
            }

            // Keep original intent but fix capture index for fallback pattern.
            if (Top.size() >= 2)
            {
                const std::wstring captured =
                    (Top.size() >= 3) ? Top[2].str() : Top[1].str();

                CrashOffset = std::wstring(L"0x") + captured;
            }

            std::wregex R(LR"((\S+)\s+0x([0-9A-Fa-f]+)\s+\+\s+([0-9A-Fa-f]+))");
            for (std::wsregex_iterator it(Pcs.begin(), Pcs.end(), R), end; it != end; ++it)
            {
                const auto& m = *it;
                Frames.emplace_back(m[1].str(), m[3].str());
            }
        }

        if (std::regex_search(
                Xml,
                M,
                std::wregex(
                    LR"(<Thread>[\s\S]*?<IsCrashed>true</IsCrashed>[\s\S]*?<ThreadID>(\d+)</ThreadID>[\s\S]*?<ThreadName>(.*?)</ThreadName>[\s\S]*?</Thread>)")))
        {
            CrashThreadId = M[1].str();
            CrashThread   = M[2].str();
        }

        if (CrashOffset.empty())   CrashOffset = L"?";
        if (CrashThread.empty())   CrashThread = L"?";
        if (CrashThreadId.empty()) CrashThreadId = L"?";

        Log(Error, L"Detected fortnite crash on thread '{}' at address '{}'", CrashThread, CrashOffset);
        Log(Error, L"Crash info folder: 'file:///{}'", CrashContextFilePath.parent_path().generic_wstring());
        
        if (bPrintCallstack)
        {
            Log(Error, L"CallStack:");
            for (const auto& Frame : Frames)
            {
                Log(Error, L"{}+0x{}", Frame.first, Frame.second);
            }   
        }

        // Wait for fortnite to exit naturally so it can save the crash dump
        WaitForSingleObject(GetLauncher()->GetFortniteProcessHandle(), 5000);

        Log(Info, L"Running on fortnite crash actions");
        for (const auto& ActionTemplate : OnFortniteCrashActions)
        {
            NUniquePtr<NAction> Action = ActionTemplate.NewObject(this);
        }

        return;
    }
}
