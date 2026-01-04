#pragma once

typedef void* HANDLE;

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
    
    operator HANDLE() const &
    {
        return Get();
    }
    
    operator HANDLE() const && = delete;
    
    bool IsValid() const noexcept;

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

    void Reset(HANDLE InHandle = nullptr) noexcept;

    void Swap(FUniqueHandle& Other) noexcept
    {
        HANDLE Temp = Handle;
        Handle = Other.Handle;
        Other.Handle = Temp;
    }
    
    HANDLE* GetAddressOf() noexcept
    {
        Reset();
        return &Handle;
    }

private:
    HANDLE Handle;
};