export module Reflection;

export import TypeHelpers;
export import :Property;
export import :UniqueObjectPtr;

export class NObject;
export class NClass;
export class NObjectClass;
export class NCapabilityClass;

export using ThisClass = void;

export template <class T>
concept IsReflectedType = requires {
    typename T::ThisClass;
    T::StaticClass();
} && std::same_as<typename T::ThisClass, T> && std::is_pointer_v<decltype(T::StaticClass())> && std::derived_from<std::remove_pointer_t<decltype(T::StaticClass())>, NClass>;

export class NObject
{
public:
    NObject() = default;

    NObject(const NObject&) = delete;
    NObject& operator=(const NObject&) = delete;
    NObject(NObject&& Other) = delete;
    NObject& operator=(NObject&& Other) = delete;

    virtual ~NObject() = default;

    using Super = void;
    using ThisClass = NObject;
    using ThisObjectClass = NObject;
    virtual NObject* GetCapabilityOwner() { return this; }
    virtual const NObject* GetCapabilityOwner() const { return this; }

    void FinishConstruction();

    bool SetPropertyValue(const std::wstring& PropertyName, const std::wstring& Value);

    FDefaultValueOverrides GetDefaultValueOverrides() const;

    static NObjectClass* StaticClass();
    virtual NObjectClass* GetObjectClass() const;
    NObjectClass* GetClass() const { return GetObjectClass(); }

    inline NObject* GetOuter() const { return OuterPrivate; }

    NObject* GetObjectOfTypeFromOuterChain(NClass* Class, bool bIgnoreSelf) const;
    template <class T>
    T* GetObjectOfTypeFromOuterChain(bool bIgnoreSelf) const
    {
        return scast<T*>(GetObjectOfTypeFromOuterChain(T::StaticClass(), bIgnoreSelf));
    }

    bool IsObjectPartOfOuterChain(const NObject* InObject) const;

    void Destroy();

    // The first function called on the object, when finishing construction. Unless using deferred construct, right after it is created.
    virtual void OnCreated();

    // Warning - OnDestroyed is only called if OnCreated was called (If construction was finished)
    virtual void OnDestroyed();

    const std::vector<FProperty>& GetPropertiesArrayConstRef() const;

private:
    friend class NObjectClass;
    NObject* OuterPrivate = nullptr;

    bool bHasFinishedConstruction = false;
    bool bIsInsideOnCreated = false;
};

export struct FClassOperations
{
    size_t Size;
    size_t Alignment;
    void (*DefaultConstructor)(void*);
    void (*Destructor)(void*);
    void* (*Factory)();
    void (*Deleter)(void*);
};

export template <class T>
FClassOperations GetClassOperations()
{
    FClassOperations Operations{};
    Operations.Size = sizeof(T);
    Operations.Alignment = alignof(T);

    if constexpr (std::default_initializable<T>)
    {
        Operations.DefaultConstructor = [](void* Address)
        {
            std::construct_at(scast<T*>(Address));
        };
        Operations.Factory = []() -> void*
        {
            return new T{};
        };
    }

    if constexpr (std::destructible<T>)
    {
        Operations.Destructor = [](void* Instance)
        {
            std::destroy_at(scast<T*>(Instance));
        };
        Operations.Deleter = [](void* Instance)
        {
            delete scast<T*>(Instance);
        };
    }

    return Operations;
}

export template <class T>
NObject* ConvertToObject(void* Instance)
{
    return scast<T*>(Instance);
}

export struct FCapabilityRegistration
{
    const NCapabilityClass* Class;
    void* (*Convert)(NObject*, const NCapabilityClass*);
    const void* (*ConvertConst)(const NObject*, const NCapabilityClass*);
};

export class NClass : public NObject
{
public:
    using Super = NObject;
    using ThisClass = NClass;
    using ThisObjectClass = NClass;

    static NObjectClass* StaticClass();
    virtual NObjectClass* GetObjectClass() const override;

    NClass(const wchar_t* Name, NClass* (*SuperClassGetter)(), FClassOperations Operations, std::vector<FProperty> (*InheritedPropertiesGetter)() = nullptr);

    std::wstring GetName() const { return Name; }
    uint16 GetId() const { return Id; }
    NClass* GetSuper() const { return SuperClass; }
    bool IsSubclassOf(const NClass* Other) const;

    size_t GetSize() const { return Operations.Size; }
    size_t GetAlignment() const { return Operations.Alignment; }
    bool CanDefaultConstructObject() const { return Operations.DefaultConstructor != nullptr; }
    void DefaultConstructObjectAt(void* Address) const { Operations.DefaultConstructor(Address); }
    void DestructObject(void* Instance) const { Operations.Destructor(Instance); }
    void* MakeDefaultConstructedObject() const { return Operations.Factory ? Operations.Factory() : nullptr; }
    void DeleteObject(void* Instance) const { Operations.Deleter(Instance); }

    const std::vector<FProperty>& GetProperties() const { return Properties; }
    bool SetPropertyValue(void* Instance, const std::wstring& PropertyName, const std::wstring& Value) const;

    static NClass* GetClassById(uint16 Id);
    static NClass* GetClassByName(const std::wstring& Name);
    static std::span<NClass* const> GetAllClasses();
    std::vector<NClass*> GetAllDerivedClasses(bool bIncludeSelf = false) const;

private:
    friend void InitReflection();

    const wchar_t* Name;
    NClass* (*SuperClassGetter)();
    FClassOperations Operations;
    std::vector<FProperty> (*InheritedPropertiesGetter)();
    NClass* Next = nullptr;
    uint16 Id = 0;
    NClass* SuperClass = nullptr;
    std::vector<FProperty> Properties{};
};

export class NObjectClass : public NClass
{
public:
    using Super = NClass;
    using ThisClass = NObjectClass;
    using ThisObjectClass = NObjectClass;

    static NObjectClass* StaticClass();
    virtual NObjectClass* GetObjectClass() const override;

    NObjectClass(const wchar_t* Name, NClass* (*SuperClassGetter)(), FClassOperations Operations, NObject* (*ObjectConverter)(void*), std::vector<FProperty> (*InheritedPropertiesGetter)() = nullptr);

    const FCapabilityRegistration* GetCapabilityRegistration(const NCapabilityClass* Class) const;

    NObject* NewObjectRaw(NObject* Outer = nullptr, bool bDeferConstruction = false, const FDefaultValueOverrides& DefaultValueOverrides = {}) const;

    template <class T>
    T* NewObjectRaw(NObject* Outer = nullptr, bool bDeferConstruction = false, const FDefaultValueOverrides& DefaultValueOverrides = {}) const
    {
        if (!IsSubclassOf(T::StaticClass()))
        {
            Log(Error, L"NewObject bad return type. {} is not a subclass of {}", GetName(), T::StaticClass()->GetName());

            return nullptr;
        }

        return scast<T*>(NewObjectRaw(Outer, bDeferConstruction, DefaultValueOverrides));
    }

    TUniqueObjectPtr<NObject> NewObject(NObject* Outer = nullptr, bool bDeferConstruction = false, const FDefaultValueOverrides& DefaultValueOverrides = {}) const
    {
        return NewObjectRaw(Outer, bDeferConstruction, DefaultValueOverrides);
    }

    template <class T>
    TUniqueObjectPtr<T> NewObject(NObject* Outer = nullptr, bool bDeferConstruction = false, const FDefaultValueOverrides& DefaultValueOverrides = {}) const
    {
        return NewObjectRaw<T>(Outer, bDeferConstruction, DefaultValueOverrides);
    }

private:
    friend void InitReflection();

    NObject* (*ObjectConverter)(void*);
    std::vector<FCapabilityRegistration> Capabilities{};
};

export class NCapabilityClass : public NClass
{
public:
    using Super = NClass;
    using ThisClass = NCapabilityClass;
    using ThisObjectClass = NCapabilityClass;

    static NObjectClass* StaticClass();
    virtual NObjectClass* GetObjectClass() const override;

    using NClass::NClass;
};

export class CCapability
{
public:
    using Super = void;
    using ThisClass = CCapability;
    using ThisCapabilityClass = CCapability;

    static NCapabilityClass* StaticClass();
    virtual NCapabilityClass* GetCapabilityClass() const { return StaticClass(); }
    NCapabilityClass* GetClass() const { return GetCapabilityClass(); }

    virtual ~CCapability() = default;
    virtual NObject* GetCapabilityOwner() = 0;
    virtual const NObject* GetCapabilityOwner() const = 0;
};

export template <class T>
NClass* GetReflectedSuperClass()
{
    if constexpr (IsReflectedType<typename T::Super>)
    {
        return T::Super::StaticClass();
    }

    return nullptr;
}

export template <class T>
std::vector<FProperty> GetInheritedProperties()
{
    if constexpr (IsReflectedType<typename T::Super>)
    {
        auto Properties = T::Super::StaticClass()->GetProperties();

        for (auto& Property : Properties)
        {
            if constexpr (!std::derived_from<T, NObject>)
            {
                Property.template Rebase<T, typename T::Super>();
            }

            Property.template SetDefaultOwner<T>();
        }

        return Properties;
    }

    return {};
}

export template <class T>
T* Cast(NObject* Object)
{
    if (!Object)
    {
        return nullptr;
    }

    if constexpr (std::derived_from<T, CCapability> && !std::derived_from<T, NObject>)
    {
        auto Registration = Object->GetClass()->GetCapabilityRegistration(T::StaticClass());

        return Registration ? scast<T*>(Registration->Convert(Object, T::StaticClass())) : nullptr;
    }
    else
    {
        return Object->GetClass()->IsSubclassOf(T::StaticClass()) ? scast<T*>(Object) : nullptr;
    }
}

export template <class T>
const T* Cast(const NObject* Object)
{
    if (!Object)
    {
        return nullptr;
    }

    if constexpr (std::derived_from<T, CCapability> && !std::derived_from<T, NObject>)
    {
        auto Registration = Object->GetClass()->GetCapabilityRegistration(T::StaticClass());

        return Registration ? scast<const T*>(Registration->ConvertConst(Object, T::StaticClass())) : nullptr;
    }
    else
    {
        return Object->GetClass()->IsSubclassOf(T::StaticClass()) ? scast<const T*>(Object) : nullptr;
    }
}

export template <class T, class Source>
    requires(std::derived_from<Source, CCapability> && !std::derived_from<Source, NObject>)
T* Cast(Source* Capability)
{
    return Capability ? Cast<T>(Capability->GetCapabilityOwner()) : nullptr;
}

export template <class T, class Source>
    requires(std::derived_from<Source, CCapability> && !std::derived_from<Source, NObject>)
const T* Cast(const Source* Capability)
{
    return Capability ? Cast<T>(Capability->GetCapabilityOwner()) : nullptr;
}

export template <class Capability>
auto ConvertCapability(Capability* Value, const NCapabilityClass* Target) -> std::conditional_t<std::is_const_v<Capability>, const void*, void*>
{
    if (std::remove_const_t<Capability>::StaticClass() == Target)
    {
        return Value;
    }

    using Super = typename std::remove_const_t<Capability>::Super;

    if constexpr (!std::is_void_v<Super>)
    {
        using Base = std::conditional_t<std::is_const_v<Capability>, const Super, Super>;

        return ConvertCapability(scast<Base*>(Value), Target);
    }

    return nullptr;
}

export template <class Owner, class Capability>
FCapabilityRegistration MakeCapabilityRegistration()
{
    return { Capability::StaticClass(),
        [](NObject* Object, const NCapabilityClass* Target) -> void*
    {
        return ConvertCapability(scast<Capability*>(scast<Owner*>(Object)), Target);
    },
        [](const NObject* Object, const NCapabilityClass* Target) -> const void*
    {
        return ConvertCapability(scast<const Capability*>(scast<const Owner*>(Object)), Target);
    } };
}

export struct FPropertyRegistration
{
    FPropertyRegistration(NClass* (*OwnerGetter)(), FProperty (*PropertyFactory)(), uint32 Order);

    NClass* (*OwnerGetter)();
    FProperty (*PropertyFactory)();
    uint32 Order;
    FPropertyRegistration* Next = nullptr;
};

export struct FCapabilityRegistrationNode
{
    FCapabilityRegistrationNode(NObjectClass* (*OwnerGetter)(), FCapabilityRegistration (*Factory)());

    NObjectClass* (*OwnerGetter)();
    FCapabilityRegistration (*Factory)();
    FCapabilityRegistrationNode* Next = nullptr;
};

export void InitReflection();
export void PrintClassesInfo(const NClass* BaseClass = nullptr);

template <typename T>
struct TTypeHelpers<T, std::enable_if_t<IsReflectedType<T> && !std::derived_from<T, NObject>>>
{
    static bool SetFromString(T* Instance, const std::wstring& Value)
    {
        FDefaultValueOverrides PropertiesSetData;

        if (!TTypeHelpers<FDefaultValueOverrides>::SetFromString(&PropertiesSetData, Value))
        {
            return false;
        }

        for (const auto& PropertySetData : PropertiesSetData)
        {
            if (!T::StaticClass()->SetPropertyValue(Instance, PropertySetData.PropertyName, PropertySetData.SetValue))
            {
                Log(Error, L"Setting property '{}' in class '{}' failed", PropertySetData.PropertyName, GetName());

                return false;
            }
        }

        return true;
    }

    static std::wstring ToString(const T* Instance)
    {
        FDefaultValueOverrides DefaultValueOverrides{};

        for (const auto& Property : T::StaticClass()->GetProperties())
        {
            FPropertySetData PropertySetData{};
            PropertySetData.PropertyName = Property.GetName();
            PropertySetData.SetValue = Property.GetAsString(Instance);

            DefaultValueOverrides.emplace_back(std::move(PropertySetData));
        }

        return TTypeHelpers<FDefaultValueOverrides>::ToString(&DefaultValueOverrides);
    }

    static std::wstring GetName() { return T::StaticClass()->GetName(); }
};

template <class T>
struct TTypeHelpers<T*, std::enable_if_t<std::derived_from<T, NClass>>>
{
    static bool SetFromString(T** Property, const std::wstring& Value)
    {
        auto CleanString = RemoveUnnecessaryCharsFromString(Value);

        if (CleanString.empty() || CleanString == L"nullptr" || CleanString == L"null" || CleanString == L"NULL" || CleanString == L"0")
        {
            *Property = nullptr;

            return true;
        }

        *Property = Cast<T>(NClass::GetClassByName(CleanString));

        if (!*Property)
        {
            Log(Error, L"{} property setter failed, no matching class", GetName());

            return false;
        }

        return true;
    }

    static std::wstring ToString(T* const* Property)
    {
        if (*Property)
        {
            return (*Property)->GetName();
        }

        return L"nullptr";
    }

    static std::wstring GetName() { return T::StaticClass()->GetName() + L"*"; }
};
