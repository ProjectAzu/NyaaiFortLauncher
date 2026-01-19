#pragma once

#include <string>
#include <vector>
#include <utility>
#include <concepts>

#include "Utils/CommandLine.h"
#include "IntegerTypes.h"

#define GENERATE_BASE_H(ClassName) \
public: \
    typedef ThisClass Super; \
    typedef ClassName ThisClass; \
    virtual ~ClassName() override = default; \
    static NClass* StaticClass(); \
    virtual NClass* GetClass() const override; \
private:

#define GENERATE_BASE_CPP(ClassName) \
static NClass ClassName##_Class{L#ClassName, ClassName::Super::StaticClass(), []()\
    { \
        return reinterpret_cast<NObject*>(new (ClassName){}); \
    }}; \
NClass* ClassName::StaticClass() { return &ClassName##_Class; } \
NClass* ClassName::GetClass() const { return &ClassName##_Class; }

#define NPROPERTY(Name) \
FPropertyCreator Name##_PropertyCreator{ \
        L#Name, this, &(Name), \
        reinterpret_cast<bool(*)(void*, const std::wstring&)>(&TTypeHelpers<decltype(Name)>::SetFromString), \
        &TTypeHelpers<decltype(Name)>::GetName, \
        &TTypeHelpers<decltype(Name)>::GetInfoOfStructsWithPropertiesUsedInType, \
        reinterpret_cast<std::wstring(*)(const void*)>(&TTypeHelpers<decltype(Name)>::ToString) \
    };

class NObject;
class NClass;

struct FPropertySetData
{
    std::wstring PropertyName{};
    std::wstring SetValue;
};

typedef std::vector<FPropertySetData> FDefaultValueOverrides;

struct FPropertyCreator
{
    // this is so ugly lmao
    FPropertyCreator(const wchar_t* Name, NObject* OwningObject, void* NativeProperty,
        bool(*Setter)(void* PropertyPtr, const std::wstring& Value), std::wstring(*TypeNameGetter)(), 
        std::vector<struct FInfoOfStructWithPropertiesUsedInType>(*FInfoOfStructsWithPropertiesUsedInTypeGetter)(),
        std::wstring(*ValueToStringConverter)(const void* Value));
    
    FPropertyCreator(const wchar_t* Name, struct FStructWithProperties* OwningObject, void* NativeProperty, 
        bool(*Setter)(void* PropertyPtr, const std::wstring& Value), std::wstring(*TypeNameGetter)(), 
        std::vector<struct FInfoOfStructWithPropertiesUsedInType>(*FInfoOfStructsWithPropertiesUsedInTypeGetter)(),
        std::wstring(*ValueToStringConverter)(const void* Value));
};

struct FProperty
{
public:
    friend struct FPropertyCreator;
    
    const wchar_t* GetName() const { return Name; }
    std::wstring GetTypeName() const { return TypeNameGetter(); }

    void* GetNativePropertyPtr(void* Object) const
    {
        return reinterpret_cast<void*>(reinterpret_cast<uint64>(Object) + Offset);
    }
    
    const void* GetNativePropertyPtr(const void* Object) const
    {
        return reinterpret_cast<const void*>(reinterpret_cast<uint64>(Object) + Offset);
    }
    
    std::vector<struct FInfoOfStructWithPropertiesUsedInType> GetInfoOfStructsWithPropertiesUsedInType() const
    {
        return InfoOfStructsWithPropertiesUsedInTypeGetter();
    }
    
    inline bool Set(void* Object, const std::wstring& Value) 
    {
        return Setter(GetNativePropertyPtr(Object), Value);
    }
    
    std::wstring GetAsString(const void* Object) const
    {
        return ValueToStringConverter(GetNativePropertyPtr(Object));
    }
    
private:
    const wchar_t* Name = nullptr;
    std::wstring(*TypeNameGetter)() = nullptr;
    std::vector<struct FInfoOfStructWithPropertiesUsedInType>(*InfoOfStructsWithPropertiesUsedInTypeGetter)() = nullptr;
    std::wstring(*ValueToStringConverter)(const void* Value) = nullptr;
    uint16 Offset = 0;
    bool(*Setter)(void* PropertyPtr, const std::wstring& Value) = nullptr;
};

#define STRUCT_WITH_PROPERTIES_SIMPLE_NAME_GETTER(Name) static std::wstring GetName() { return L#Name; }

// all structs with properties should have a no arg constructor for the sample object and should implement GetName
struct FStructWithProperties
{
    bool SetPropertyValue(const std::wstring& PropertyName, const std::wstring& Value);
    
    // Please implement this when deriving from FStructWithProperties
    static std::wstring GetName()
    {
        Log(Warning, L"GetName not implemented for this FStructWithProperties, please implement");
        return L"Struct with unknown name";
    }
    
    const std::vector<FProperty>& GetPropertiesArrayConstRef() const { return Properties; }
    
private:
    friend struct FPropertyCreator;

    template<typename, typename>
    friend struct TTypeHelpers;
    
    std::vector<FProperty> Properties{};
};

class NObject
{
public:
    NObject() = default;
    
    NObject(const NObject&) = delete;
    NObject&operator=(const NObject&) = delete;
    NObject(NObject&& Other) = delete;
    NObject&operator=(NObject&& Other) = delete;
    
    virtual ~NObject() = default;

    typedef NObject ThisClass;

    void FinishConstruction();

    bool SetPropertyValue(const std::wstring& PropertyName, const std::wstring& Value);

    static NClass* StaticClass();
    virtual NClass* GetClass() const;

    inline NObject* GetOuter() const { return OuterPrivate; }
    
    NObject* GetObjectOfTypeFromOuterChain(NClass* Class, bool bIgnoreSelf) const;
    template<class T> T* GetObjectOfTypeFromOuterChain(bool bIgnoreSelf) const
    {
        return reinterpret_cast<T*>(GetObjectOfTypeFromOuterChain(T::StaticClass(), bIgnoreSelf));
    }
    
    bool IsObjectPartOfOuterChain(const NObject* InObject) const;

    void Destroy();

    // The first function called on the object, when finishing construction. Unless using deferred construct, right after it is created.
    virtual void OnCreated();
    
    // Warning - OnDestroyed is only called if OnCreated was called (If construction was finished)
    virtual void OnDestroyed();
    
    const std::vector<FProperty>& GetPropertiesArrayConstRef() const { return Properties; }

private:
    friend class NClass;
    NObject* OuterPrivate = nullptr;

    bool bHasFinishedConstruction = false;
    bool bIsInsideOnCreated = false;
    
    friend struct FPropertyCreator;
    std::vector<FProperty> Properties{};
};

template <class T>
class NUniquePtr
{
public:
    NUniquePtr() = default;

    inline NUniquePtr(T* ReflectedClass) : Object(ReflectedClass)
    {
		
    }

    inline NUniquePtr(std::nullptr_t) : Object(nullptr) {}
    
    NUniquePtr(const NUniquePtr&) = delete;
    NUniquePtr&operator=(const NUniquePtr&) = delete;

    NUniquePtr(NUniquePtr&& Other) noexcept : Object(Other.Object)
    {
        Other.Object = nullptr;
    }
    
    NUniquePtr&operator=(NUniquePtr&& Other) noexcept
    {
        Reset(Other.Release());
        return *this;
    }

    NUniquePtr& operator=(T* Ptr) noexcept
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
	
    ~NUniquePtr()
    {
        Reset(nullptr);
    }

    inline T* operator->() const { return reinterpret_cast<T*>(Object); }
    inline T* Get() const { return reinterpret_cast<T*>(Object); }
    
    operator T*() const & { return Get(); }
    operator T*() const && = delete;

    T* Release()
    {
        T* Released = reinterpret_cast<T*>(Object);
        Object = nullptr;
        return Released;
    }

    void Reset(T* NewObject)
    {
        if(Object)
        {
            Object->Destroy();
        }
        
        Object = reinterpret_cast<NObject*>(NewObject);
    }

private:
    NObject* Object = nullptr;
};

class NClass
{
public:
    NClass(const std::wstring& Name, NClass* SuperClass, NObject*(*NewObjectFactory)());

    NClass(const NClass&) = delete;
    NClass&operator=(const NClass&) = delete;
    NClass(NClass&& Other) = delete;
    NClass&operator=(NClass&& Other) = delete;
    
    std::wstring GetName() const { return Name; }
    uint16 GetId() const { return Id; }
    NClass* GetSuper() const { return SuperClass; }
    
    const NObject* GetDefaultObject() const;
    template<class T> const T* GetDefaultObject() const
    {
        if (!IsSubclassOf(T::StaticClass()))
        {
            return nullptr;
        }
        
        return reinterpret_cast<const T*>(GetDefaultObject());
    }

    bool IsSubclassOf(const NClass* Other) const;
    
    NObject* NewObjectRaw(NObject* Outer = nullptr, bool bDeferConstruction = false, const FDefaultValueOverrides& DefaultValueOverrides = {}) const;
    
    template<class T>
    T* NewObjectRaw(NObject* Outer = nullptr, bool bDeferConstruction = false, const FDefaultValueOverrides& DefaultValueOverrides = {}) const
    {
        if (!IsSubclassOf(T::StaticClass()))
        {
            Log(Error, L"NewObject bad return type. {} is not a subclass of {}", GetName(), T::StaticClass()->GetName());
            return nullptr;
        }
        
        return reinterpret_cast<T*>(NewObjectRaw(Outer, bDeferConstruction, DefaultValueOverrides));
    }
    
    NUniquePtr<NObject> NewObject(NObject* Outer = nullptr, bool bDeferConstruction = false, const FDefaultValueOverrides& DefaultValueOverrides = {}) const
    {
        return NewObjectRaw(Outer, bDeferConstruction, DefaultValueOverrides);
    }
    
    template<class T>
    NUniquePtr<T> NewObject(NObject* Outer = nullptr, bool bDeferConstruction = false, const FDefaultValueOverrides& DefaultValueOverrides = {}) const
    {
        return NewObjectRaw<T>(Outer, bDeferConstruction, DefaultValueOverrides);
    }

    static NClass* GetClassById(uint16 Id);
    static NClass* GetClassByName(const std::wstring& Name);
    std::vector<NClass*> GetAllDerivedClasses() const;

private:
    std::wstring Name;
    uint16 Id = 0;
    NClass* SuperClass;
    NObject*(*NewObjectFactory)();
    const NObject* DefaultObject = nullptr;
};

template<class NObjectType>
class NSubClassOf
{
public:
    inline NSubClassOf(NClass* Class)
    {
        if(Class && Class->IsSubclassOf(NObjectType::StaticClass()))
        {
            ClassPrivate = Class;
        }
    }
	
    NSubClassOf() : NSubClassOf(nullptr) {}

    inline NSubClassOf(std::nullptr_t) {}

    inline NSubClassOf(const NSubClassOf<NObjectType>& Other) : ClassPrivate(Other.ClassPrivate)
    {
    }

    template<class T>
    inline NSubClassOf(const NSubClassOf<T>& Other) : NSubClassOf(Other.Get())
    {
    }

    inline NClass* Get() const { return ClassPrivate; }

    inline NClass* operator->() const
    {
        return ClassPrivate;
    }
    
    operator bool() const noexcept
    {
        return ClassPrivate != nullptr;
    }

    operator NClass*() const noexcept
    {
        return ClassPrivate;
    }
	
private:
    NClass* ClassPrivate = nullptr;
};

template<class T> inline T* Cast(NObject* Object)
{
    return Object ? (Object->GetClass()->IsSubclassOf(T::StaticClass()) ? reinterpret_cast<T*>(Object) : nullptr) : nullptr;
}

template<class T>
struct TObjectTemplate
{
    TObjectTemplate() = default;

    TObjectTemplate(const TObjectTemplate& OtherTemplate)
    {
        CopyFrom(OtherTemplate);
    }

    template<class OtherT>
    TObjectTemplate(const TObjectTemplate<OtherT>& OtherTemplate)
    {
        CopyFrom(OtherTemplate);
    }

    TObjectTemplate(NSubClassOf<T> Class)
    {
        Reset(Class);
    }

    TObjectTemplate(NClass* Class)
    {
        if (Class)
        {
            TemplateObject = Class->template NewObject<T>(nullptr, true);
        }
    }

    TObjectTemplate(TObjectTemplate&&) noexcept = default;
    TObjectTemplate& operator=(TObjectTemplate&&) noexcept = default;

    TObjectTemplate& operator=(const TObjectTemplate& OtherTemplate)
    {
        CopyFrom(OtherTemplate);
        return *this;
    }

    template<class OtherT>
    TObjectTemplate& operator=(const TObjectTemplate<OtherT>& OtherTemplate)
    {
        CopyFrom(OtherTemplate);
        return *this;
    }

    TObjectTemplate(NSubClassOf<T> Class, const FDefaultValueOverrides& DefaultValueOverrides)
    {
        if (!Class)
        {
            TemplateObject.Reset(nullptr);
            return;
        }

        TemplateObject.Reset(Class->template NewObjectRaw<T>(nullptr, true, DefaultValueOverrides));
    }

    T* NewObjectRaw(NObject* Outer = nullptr, bool bDeferConstruction = false) const
    {
        if (!TemplateObject)
        {
            Log(Error, L"TObjectTemplate<T>::NewObjectRaw:, Cannot initialize template, no TemplateObject");
            return nullptr;
        }

        return TemplateObject->GetClass()->template NewObjectRaw<T>(Outer, bDeferConstruction, GetDefaultValueOverrides());
    }

    NUniquePtr<T> NewObject(NObject* Outer = nullptr, bool bDeferConstruction = false) const
    {
        return NewObjectRaw(Outer, bDeferConstruction);
    }

    void Reset(NSubClassOf<T> InClass)
    {
        if (!InClass)
        {
            TemplateObject.Reset(nullptr);
            return;
        }

        TemplateObject = InClass->template NewObject<T>(nullptr, true);
    }

    // Modifies the currently set class while preserving the default value overrides
    void ModifyClass(NSubClassOf<T> InClass)
    {
        if (!InClass)
        {
            TemplateObject.Reset(nullptr);

            Log(Error, L"TObjectTemplate<T>::ModifyClass: InClass can't be nullptr. If this is intended, use Reset(nullptr)");

            return;
        }

        if (TemplateObject && !InClass->IsSubclassOf(TemplateObject->GetClass()))
        {
            Log(Error, L"TObjectTemplate<T>::ModifyClass: InClass '{}' has to be a subclass of the currently set class '{}'",
                InClass->GetName(), TemplateObject->GetClass()->GetName());

            Reset(InClass);

            return;
        }

        TemplateObject.Reset(InClass->template NewObjectRaw<T>(nullptr, true, GetDefaultValueOverrides()));
    }

    NSubClassOf<T> GetClass() const
    {
        return TemplateObject ? TemplateObject->GetClass() : nullptr;
    }

    FDefaultValueOverrides GetDefaultValueOverrides() const
    {
        if (!TemplateObject)
        {
            return {};
        }

        const auto& Properties = TemplateObject->GetPropertiesArrayConstRef();

        FDefaultValueOverrides DefaultValueOverrides{};
        DefaultValueOverrides.reserve(Properties.size());

        for (const FProperty& Property : Properties)
        {
            FPropertySetData PropertySetData{};
            PropertySetData.PropertyName = Property.GetName();
            PropertySetData.SetValue = Property.GetAsString(TemplateObject);

            DefaultValueOverrides.emplace_back(std::move(PropertySetData));
        }

        return DefaultValueOverrides;
    }

    T* operator->() const
    {
        return TemplateObject;
    }

    operator bool() const
    {
        return TemplateObject != nullptr;
    }

private:
    template<class OtherT>
    void CopyFrom(const TObjectTemplate<OtherT>& OtherTemplate)
    {
        if (!OtherTemplate.GetClass())
        {
            TemplateObject.Reset(nullptr);
            return;
        }

        TemplateObject.Reset(OtherTemplate.NewObjectRaw(nullptr, true));
    }

    NUniquePtr<T> TemplateObject = nullptr;
};


#include "TypeHelpers.h"
