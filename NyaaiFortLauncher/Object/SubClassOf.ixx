export module SubClassOf;

export import Reflection;

export template <class BaseType>
class TSubClassOf
{
public:
    using ClassType = std::remove_pointer_t<decltype(BaseType::StaticClass())>;

    inline TSubClassOf(NClass* Class)
    {
        if (Class && Class->IsSubclassOf(BaseType::StaticClass()))
        {
            ClassPrivate = Cast<ClassType>(Class);
        }
    }

    TSubClassOf()
        : TSubClassOf(nullptr) {}

    inline TSubClassOf(std::nullptr_t) {}

    inline TSubClassOf(const TSubClassOf<BaseType>& Other)
        : ClassPrivate(Other.ClassPrivate)
    {
    }

    template <class T>
    inline TSubClassOf(const TSubClassOf<T>& Other)
        : TSubClassOf(Other.Get())
    {
    }

    inline ClassType* Get() const { return ClassPrivate; }

    inline ClassType* operator->() const
    {
        return ClassPrivate;
    }

    operator bool() const noexcept
    {
        return ClassPrivate != nullptr;
    }

    operator ClassType*() const noexcept
    {
        return ClassPrivate;
    }

private:
    ClassType* ClassPrivate = nullptr;
};

template <class T>
struct TTypeHelpers<TSubClassOf<T>>
{
    static bool SetFromString(TSubClassOf<T>* Property, const std::wstring& Value)
    {
        NClass* Class = nullptr;

        if (!TTypeHelpers<NClass*>::SetFromString(&Class, Value))
        {
            return false;
        }

        *Property = Class;

        if (Class && !*Property)
        {
            Log(Error, L"{} setter failed, {} is not a subclass of {}", GetName(), Class->GetName(), T::StaticClass()->GetName());

            return false;
        }

        return true;
    }

    static std::wstring ToString(const TSubClassOf<T>* Property)
    {
        if (*Property)
        {
            return (*Property)->GetName();
        }

        return L"nullptr";
    }

    static std::wstring GetName()
    {
        return std::format(L"TSubClassOf<{}>", T::StaticClass()->GetName());
    }
};
