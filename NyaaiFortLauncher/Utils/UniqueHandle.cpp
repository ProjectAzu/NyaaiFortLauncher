#include "UniqueHandle.h"

#include "WindowsInclude.h"

bool FUniqueHandle::IsValid() const noexcept
{
    return Handle != nullptr && Handle != INVALID_HANDLE_VALUE;
}

void FUniqueHandle::Reset(HANDLE InHandle) noexcept
{
    if (Handle == InHandle)
    {
        return;
    }

    if (IsValid())
    {
        CloseHandle(Handle);
    }

    Handle = InHandle;
}