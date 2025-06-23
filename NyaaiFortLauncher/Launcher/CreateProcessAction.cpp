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
                "Failed to create Job Object (Error code {}) -- child processes won't be killed automatically.",
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
                "Failed to set JobObjectExtendedLimitInformation (Error code {})",
                LastError);
            // We still have a job object, but it won't kill children automatically
        }
    }
}

void NCreateProcessAction::Execute()
{
    Super::Execute();

    if (!exists(FilePath) || !is_regular_file(FilePath) || FilePath.extension() != ".exe")
    {
        Log(Error, "NCreateProcessAction: Bad file path");
        return;
    }

    // Ensure our Job Object is initialized. All child processes will join this job.
    InitializeJobObjectIfNeeded();
    
    std::wstring ExePathW = FilePath.wstring();
    // Convert LaunchArguments (UTF-8) to wide
    std::wstring ArgsW(LaunchArguments.begin(), LaunchArguments.end());

    // If there are no arguments, we can pass null or an empty string to CreateProcess
    LPWSTR CommandLinePtr = nullptr;
    if (!ArgsW.empty())
    {
        if (ArgsW[0] != L' ')
        {
            ArgsW.insert(ArgsW.begin(), L' ');
        }
        
        // Command line should contain only the arguments, not the .exe path again
        CommandLinePtr = ArgsW.data(); // safe in C++17+ as ArgsW is non-empty
    }

    StartupInfo.cb = sizeof(StartupInfo);

    // We want to suspend if bSuspendProcessAfterCreation is true
    DWORD CreationFlags = 0;
    if (bSuspendProcessAfterCreation)
    {
        CreationFlags |= CREATE_SUSPENDED;
    }

    // In many console or server apps, you might prefer bInheritHandles = FALSE
    // but set to TRUE if you want the child to inherit any of this process's
    // inheritable handles, stdin/out, etc.
    BOOL bSuccess = CreateProcessW(
        ExePathW.c_str(),     // lpApplicationName
        CommandLinePtr,       // lpCommandLine (arguments only)
        nullptr,              // lpProcessAttributes
        nullptr,              // lpThreadAttributes
        TRUE,                 // bInheritHandles
        CreationFlags,
        nullptr,              // lpEnvironment (inherit environment)
        nullptr,              // lpCurrentDirectory (inherit current directory)
        &StartupInfo,
        &ResultProcessInfo
    );

    if (!bSuccess)
    {
        DWORD LastError = GetLastError();
        Log(Error, "Failed to create process (Error code {})", LastError);
        return;
    }

    // If our job object was successfully created, assign the new process to it
    if (GJobHandle)
    {
        if (!AssignProcessToJobObject(GJobHandle, ResultProcessInfo.hProcess))
        {
            DWORD LastError = GetLastError();
            Log(Error, 
                "Failed to assign process PID {} to job object (Error code {})",
                ResultProcessInfo.dwProcessId, 
                LastError);
        }
    }

    Log(Info, 
        "Launched '{}' PID: {}, TID: {}", 
        FilePath.string(), 
        ResultProcessInfo.dwProcessId, 
        ResultProcessInfo.dwThreadId);
    
    CloseHandle(ResultProcessInfo.hThread);

    if (bReturnProcessHandle)
    {
        ResultProcessHandle = ResultProcessInfo.hProcess;
    }
    else
    {
        CloseHandle(ResultProcessInfo.hProcess);
    }

    bResultWasSuccess = true;
}