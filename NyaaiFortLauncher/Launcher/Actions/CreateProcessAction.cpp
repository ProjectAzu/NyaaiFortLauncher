#include "CreateProcessAction.h"

GENERATE_BASE_CPP(NCreateProcessAction)

static HANDLE GJobHandle = nullptr;

static void InitializeJobObjectIfNeeded()
{
    if (!GJobHandle)
    {
        // Create the job object
        GJobHandle = CreateJobObjectW(nullptr, nullptr);
        if (!GJobHandle)
        {
            DWORD LastError = GetLastError();
            Log(Error, 
                L"Failed to create Job Object (Error code {}) -- child processes won't be killed automatically",
                LastError);
            return;
        }

        // Set the 'kill on job close' limit so child processes die when the job handle closes
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {};
        jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (!SetInformationJobObject(
                GJobHandle, 
                JobObjectExtendedLimitInformation, 
                &jeli, 
                sizeof(jeli)))
        {
            DWORD LastError = GetLastError();
            Log(Error, 
                L"Failed to set JobObjectExtendedLimitInformation (Error code {})",
                LastError);
            // We still have a job object, but it won't kill children automatically
        }
    }
}

void NCreateProcessAction::Execute()
{
    Super::Execute();
    
    FilePath = ResolvePossiblyFortniteBuildRelativePath(FilePath);
    WorkingDirectory = ResolvePossiblyFortniteBuildRelativePath(WorkingDirectory);

    if (!std::filesystem::exists(FilePath) || 
        !std::filesystem::is_regular_file(FilePath) || 
        FilePath.extension().wstring() != L".exe")
    {
        Log(Error, L"NCreateProcessAction: Bad file path");
        return;
    }
    
    InitializeJobObjectIfNeeded();
    
    if (!WorkingDirectory.empty())
    {
        if (!std::filesystem::exists(WorkingDirectory) ||
            !std::filesystem::is_directory(WorkingDirectory))
        {
            Log(Error, L"NCreateProcessAction: Bad working directory. Using exe dir as working dir");
            WorkingDirectory.clear();
        }   
    }
    
    if (WorkingDirectory.empty())
    {
        WorkingDirectory = FilePath.parent_path();
    }
    
    auto WorkingDirectoryW = WorkingDirectory.wstring();
    
    std::wstring ExePathW = FilePath.wstring();
    
    // If there are no arguments, we can pass null or an empty string to CreateProcess
    LPWSTR CommandLinePtr = nullptr;
    if (!LaunchArguments.empty())
    {
        if (LaunchArguments[0] != L' ')
        {
            LaunchArguments.insert(LaunchArguments.begin(), L' ');
        }
        
        CommandLinePtr = LaunchArguments.data();
    }

    StartupInfo.cb = sizeof(StartupInfo);
    
    DWORD CreationFlags = 0;
    if (bCreateSuspended)
    {
        CreationFlags |= CREATE_SUSPENDED;
    }
    
    BOOL bSuccess = CreateProcessW(
        ExePathW.c_str(),     // lpApplicationName
        CommandLinePtr,       // lpCommandLine (arguments only)
        nullptr,              // lpProcessAttributes
        nullptr,              // lpThreadAttributes
        TRUE,                 // bInheritHandles
        CreationFlags,
        nullptr,              // lpEnvironment (inherit environment)
        WorkingDirectoryW.c_str(),
        &StartupInfo,
        &ResultProcessInfo
    );

    if (!bSuccess)
    {
        DWORD LastError = GetLastError();
        Log(Error, L"Failed to create process (Error code {})", LastError);
        return;
    }
    
    if (GJobHandle)
    {
        if (!AssignProcessToJobObject(GJobHandle, ResultProcessInfo.hProcess))
        {
            DWORD LastError = GetLastError();
            Log(Error, 
                L"Failed to assign process PID {} to job object (Error code {})",
                ResultProcessInfo.dwProcessId, 
                LastError);
        }
    }
    
    auto EngineObject = Cast<NEngineObject>(GetOuter());
    if (EngineObject)
    {
        FUniqueHandle DuplicateProcessHandle{};
        
        if (DuplicateHandle(
            GetCurrentProcess(), 
            ResultProcessInfo.hProcess,            
            GetCurrentProcess(), 
            DuplicateProcessHandle.GetAddressOf(),               
            0,                   
            FALSE,               
            DUPLICATE_SAME_ACCESS))
        {
            EngineObject->AddChildProcess(std::move(DuplicateProcessHandle));
        }
        else
        {
            Log(Error, L"Failed to duplicate process handle");
        }
    }

    Log(Info, 
        L"Launched '{}' PID: {}, TID: {} as child process of '{}'", 
        FilePath.wstring(), 
        ResultProcessInfo.dwProcessId, 
        ResultProcessInfo.dwThreadId,
        EngineObject ? EngineObject->GetClass()->GetName() : L"nullptr");
    
    ResultProcessHandle = ResultProcessInfo.hProcess;
    ResultThreadHandle = ResultProcessInfo.hThread;

    bResultWasSuccess = true;
}