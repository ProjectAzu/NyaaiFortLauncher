export module Reflection:UniqueObjectPtr;

export import Core;

export template <class T>
class TUniqueObjectPtr
{
public:
    TUniqueObjectPtr() = default;

    inline TUniqueObjectPtr(T* ReflectedClass)
        : Object(ReflectedClass)
    {
    }

    inline TUniqueObjectPtr(std::nullptr_t)
        : Object(nullptr) {}

    TUniqueObjectPtr(const TUniqueObjectPtr&) = delete;
    TUniqueObjectPtr& operator=(const TUniqueObjectPtr&) = delete;

    TUniqueObjectPtr(TUniqueObjectPtr&& Other) noexcept
        : Object(Other.Object)
    {
        Other.Object = nullptr;
    }

    TUniqueObjectPtr& operator=(TUniqueObjectPtr&& Other) noexcept
    {
        Reset(Other.Release());

        return *this;
    }

    TUniqueObjectPtr& operator=(T* Ptr) noexcept
    {
        Reset(Ptr);

        return *this;
    }

    bool operator==(T* Ptr) noexcept
    {
        return Object == Ptr;
    }

    operator bool() const noexcept
    {
        return Object != nullptr;
    }

    ~TUniqueObjectPtr()
    {
        Reset(nullptr);
    }

    inline T* operator->() const { return Object; }
    inline T* Get() const { return Object; }

    operator T*() const& { return Get(); }
    operator T*() const&& = delete;

    T* Release()
    {
        T* Released = Object;
        Object = nullptr;

        return Released;
    }

    void Reset(T* NewObject)
    {
        if (Object == NewObject)
        {
            return;
        }

        if (Object)
        {
            Object->Destroy();
        }

        Object = NewObject;
    }

private:
    T* Object = nullptr;
};
