module;
#include "Utils/WindowsInclude.h"

module DetectFortniteCrashActivity;

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

    FortniteCrashesFolderPath = ResolvePossiblyFortniteBuildRelativePath(FortniteCrashesFolderPath, false);

    if (FortniteCrashesFolderPath.empty())
    {
        Log(Error, L"NDetectFortniteCrashActivity: FortniteCrashesFolderPath is empty");

        return;
    }

    StartWatching();
    ScanCrashContexts(true);
}

void NDetectFortniteCrashActivity::OnDestroyed()
{
    if (FortniteProcessId != 0 && !FortniteCrashesFolderPath.empty() && !bCrashDetected)
    {
        ScanCrashContexts(false);
        ProcessCrashContexts();
    }

    StopWatching();
    Super::OnDestroyed();
}

void NDetectFortniteCrashActivity::StartWatching()
{
    auto WatchPath = FortniteCrashesFolderPath;

    while (!WatchPath.empty())
    {
        HANDLE Handle = FindFirstChangeNotificationW(WatchPath.c_str(), TRUE, FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE);

        if (Handle != INVALID_HANDLE_VALUE)
        {
            CrashDirChangeHandle = Handle;
            return;
        }

        const DWORD Error = GetLastError();

        if (Error != ERROR_PATH_NOT_FOUND && Error != ERROR_FILE_NOT_FOUND)
        {
            Log(Warning, L"Cannot watch Fortnite crashes folder '{}': {}", WatchPath.wstring(), Error);
            return;
        }

        const auto Parent = WatchPath.parent_path();

        if (Parent == WatchPath)
        {
            return;
        }

        WatchPath = Parent;
    }
}

void NDetectFortniteCrashActivity::StopWatching()
{
    if (CrashDirChangeHandle)
    {
        FindCloseChangeNotification(CrashDirChangeHandle);
        CrashDirChangeHandle = nullptr;
    }
}

void NDetectFortniteCrashActivity::ScanCrashContexts(bool bIgnoreExisting)
{
    std::error_code ErrorCode{};
    std::filesystem::recursive_directory_iterator It(FortniteCrashesFolderPath, ErrorCode);
    const std::filesystem::recursive_directory_iterator End{};

    while (!ErrorCode && It != End)
    {
        const auto& Entry = *It;

        if (Entry.path().filename() == L"CrashContext.runtime-xml" && Entry.is_regular_file(ErrorCode))
        {
            if (bIgnoreExisting)
            {
                AlreadyCheckedCrashContextFilePaths.insert(Entry.path());
            }
            else if (!AlreadyCheckedCrashContextFilePaths.contains(Entry.path()))
            {
                PendingCrashContextFilePaths.insert(Entry.path());
            }
        }

        if (!ErrorCode)
        {
            It.increment(ErrorCode);
        }
    }

    if (ErrorCode == std::errc::no_such_file_or_directory)
    {
        ErrorCode.clear();
    }

    if (ErrorCode && ErrorCode != LastScanError)
    {
        Log(Warning, L"Cannot scan Fortnite crashes folder '{}': {}", FortniteCrashesFolderPath.wstring(), ErrorCode.value());
    }

    LastScanError = ErrorCode;
}

void NDetectFortniteCrashActivity::Tick(double DeltaTime)
{
    Super::Tick(DeltaTime);

    if (FortniteProcessId == 0 || bCrashDetected)
    {
        return;
    }

    if (FortniteCrashesFolderPath.empty())
    {
        return;
    }

    if (CrashDirChangeHandle)
    {
        const DWORD Result = WaitForSingleObject(CrashDirChangeHandle, 0);

        if (Result == WAIT_OBJECT_0)
        {
            if (!FindNextChangeNotification(CrashDirChangeHandle))
            {
                StopWatching();
            }

            ScanCrashContexts(false);
        }
        else if (Result == WAIT_FAILED)
        {
            StopWatching();
        }
    }

    if (!CrashDirChangeHandle)
    {
        TimeUntilWatchRetry -= DeltaTime;

        if (TimeUntilWatchRetry <= 0.0)
        {
            TimeUntilWatchRetry = 1.0;
            StartWatching();
            ScanCrashContexts(false);
        }
    }

    ProcessCrashContexts();
}

void NDetectFortniteCrashActivity::ProcessCrashContexts()
{
    std::vector<std::filesystem::path> CrashContextFilePaths{};

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
            PendingCrashContextFilePaths.insert(CrashContextFilePath);
            continue;
        }

        std::wsmatch M{};

        if (!std::regex_search(Xml, M, std::wregex(LR"(<ProcessId>(\d+)</ProcessId>)")) ||
            M.size() < 2)
        {
            PendingCrashContextFilePaths.insert(CrashContextFilePath);
            continue;
        }

        uint32 CrashProcessId = 0;
        const std::wstring ProcessIdText = M[1].str();
        std::wistringstream ProcessIdStream{ ProcessIdText };

        if (!(ProcessIdStream >> CrashProcessId))
        {
            AlreadyCheckedCrashContextFilePaths.insert(CrashContextFilePath);
            continue;
        }

        AlreadyCheckedCrashContextFilePaths.insert(CrashContextFilePath);

        if (CrashProcessId != FortniteProcessId)
        {
            continue;
        }

        bCrashDetected = true;

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
            CrashThread = M[2].str();
        }

        if (CrashOffset.empty())
        {
            CrashOffset = L"?";
        }

        if (CrashThread.empty())
        {
            CrashThread = L"?";
        }

        if (CrashThreadId.empty())
        {
            CrashThreadId = L"?";
        }

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
            TUniqueObjectPtr<NAction> Action = ActionTemplate.NewObject(this);
        }

        return;
    }
}
