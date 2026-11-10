export module UniqueHandle;

export import CoreTypes;

export struct FUniqueHandle
{
    FUniqueHandle() noexcept
        : Handle(nullptr)
    {
    }

    FUniqueHandle(void* InHandle) noexcept
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

    void* Get() const noexcept
    {
        return Handle;
    }

    operator void*() const&
    {
        return Get();
    }

    operator void*() const&& = delete;

    bool IsValid() const noexcept;

    explicit operator bool() const noexcept
    {
        return IsValid();
    }

    void* Release() noexcept
    {
        void* Old = Handle;
        Handle = nullptr;

        return Old;
    }

    void Reset(void* InHandle = nullptr) noexcept;

    void Swap(FUniqueHandle& Other) noexcept
    {
        void* Temp = Handle;
        Handle = Other.Handle;
        Other.Handle = Temp;
    }

    void** GetAddressOf() noexcept
    {
        Reset();

        return &Handle;
    }

private:
    void* Handle;
};
