#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

struct FUniqueHandle
{
    FUniqueHandle() noexcept
        : Handle(nullptr)
    {
    }

    FUniqueHandle(HANDLE InHandle) noexcept
        : Handle(InHandle)
    {
    }

    ~FUniqueHandle() noexcept
    {
        Reset();
    }
    
    FUniqueHandle(const FUniqueHandle&) = delete;
    FUniqueHandle& operator=(const FUniqueHandle&) = delete;
    
    FUniqueHandle(FUniqueHandle&& Other) noexcept
        : Handle(Other.Release())
    {
    }

    FUniqueHandle& operator=(FUniqueHandle&& Other) noexcept
    {
        if (this != &Other)
        {
            Reset(Other.Release());
        }
        return *this;
    }
    
    HANDLE Get() const noexcept
    {
        return Handle;
    }

    bool IsValid() const noexcept
    {
        return Handle != nullptr && Handle != INVALID_HANDLE_VALUE;
    }

    explicit operator bool() const noexcept
    {
        return IsValid();
    }
    
    HANDLE Release() noexcept
    {
        HANDLE Old = Handle;
        Handle = nullptr;
        return Old;
    }

    void Reset(HANDLE InHandle = nullptr) noexcept
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

    void Swap(FUniqueHandle& Other) noexcept
    {
        HANDLE Temp = Handle;
        Handle = Other.Handle;
        Other.Handle = Temp;
    }

private:
    HANDLE Handle;
};