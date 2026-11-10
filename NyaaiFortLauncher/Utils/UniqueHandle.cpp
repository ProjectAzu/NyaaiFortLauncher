module;
#include "Utils/WindowsInclude.h"

module UniqueHandle;

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
