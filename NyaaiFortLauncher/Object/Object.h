#pragma once

#include <string>
#include <vector>

#include "Utils/Log.h"
#include "IntegerTypes.h"
#include "DefaultValueOverrides.h"

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
FProperty Name##_Property{ \
        L#Name, this, &(Name), \
        reinterpret_cast<bool(*)(void*, const std::wstring&)>(&TTypeHelpers<decltype(Name)>::SetFromString), \
        &TTypeHelpers<decltype(Name)>::GetName, \
        &TTypeHelpers<decltype(Name)>::GetInfoOfStructsWithPropertiesUsedInType, \
        reinterpret_cast<std::wstring(*)(const void*)>(&TTypeHelpers<decltype(Name)>::ToString) \
    };

class NObject;
class NClass;

class FProperty
{
public:
    // this is so ugly lmao
    FProperty(const wchar_t* Name, NObject* OwningObject, void* Property,
        bool(*Setter)(void* PropertyPtr, const std::wstring& Value), std::wstring(*TypeNameGetter)(), 
        std::vector<struct FInfoOfStructWithPropertiesUsedInType>(*FInfoOfStructsWithPropertiesUsedInTypeGetter)(),
        std::wstring(*ValueToStringConverter)(const void* Value));
    
    FProperty(const wchar_t* Name, struct FStructWithProperties* OwningObject, void* Property, 
        bool(*Setter)(void* PropertyPtr, const std::wstring& Value), std::wstring(*TypeNameGetter)(), 
        std::vector<struct FInfoOfStructWithPropertiesUsedInType>(*FInfoOfStructsWithPropertiesUsedInTypeGetter)(),
        std::wstring(*ValueToStringConverter)(const void* Value));

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
    const wchar_t* Name;
    std::wstring(*TypeNameGetter)();
    std::vector<struct FInfoOfStructWithPropertiesUsedInType>(*InfoOfStructsWithPropertiesUsedInTypeGetter)();
    std::wstring(*ValueToStringConverter)(const void* Value);
    uint16 Offset;
    bool(*Setter)(void* PropertyPtr, const std::wstring& Value);
};

#define STRUCT_WITH_PROPERTIES_SIMPLE_NAME_GETTER(Name) static std::wstring GetName() { return L#Name; }

// all structs with properties should have a no arg constructor for the sample object and should implement GetName
struct FStructWithProperties
{
    void SetPropertyValue(const std::wstring& PropertyName, const std::wstring& Value);
    
    // Please implement this when deriving from FStructWithProperties
    static std::wstring GetName()
    {
        Log(Warning, L"GetName not implemented for this FStructWithProperties, please implement");
        return L"Struct with unknown name";
    }
    
    const std::vector<FProperty*>& GetPropertiesArrayConstRef() const { return Properties; }
    
private:
    friend class FProperty;

    template<typename, typename>
    friend struct TTypeHelpers;
    
    std::vector<FProperty*> Properties{};
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

    void SetPropertyValue(const std::wstring& PropertyName, const std::wstring& Value);

    static NClass* StaticClass();
    virtual NClass* GetClass() const;

    inline NObject* GetOuter() const { return OuterPrivate; }

    void Destroy();

    // The first function called on the object, right after it is created
    virtual void OnCreated();
    
    virtual void OnDestroyed();
    
    const std::vector<FProperty*>& GetPropertiesArrayConstRef() const { return Properties; }

private:
    friend class FProperty;
    std::vector<FProperty*> Properties{};

    friend class NClass;
    NObject* OuterPrivate = nullptr;

    bool bHasFinishedConstruction = false;
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
    inline operator T*() const { return Get(); }

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
    NObject* Object;
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
    
    NObject* NewObjectRaw(NObject* Outer = nullptr, const FDefaultValueOverrides& DefaultValueOverrides = {}, bool bDeferConstruction = false) const;
    
    template<class T>
    T* NewObjectRaw(NObject* Outer = nullptr, const FDefaultValueOverrides& DefaultValueOverrides = {}, bool bDeferConstruction = false) const
    {
        if (!IsSubclassOf(T::StaticClass()))
        {
            Log(Error, L"NewObject bad return type. {} is not a subclass of {}.", GetName(), T::StaticClass()->GetName());
            return nullptr;
        }
        
        return reinterpret_cast<T*>(NewObjectRaw(Outer, DefaultValueOverrides, bDeferConstruction));
    }
    
    NUniquePtr<NObject> NewObject(NObject* Outer = nullptr, const FDefaultValueOverrides& DefaultValueOverrides = {}, bool bDeferConstruction = false) const
    {
        return NewObjectRaw(Outer, DefaultValueOverrides, bDeferConstruction);
    }
    
    template<class T>
    NUniquePtr<T> NewObject(NObject* Outer = nullptr, const FDefaultValueOverrides& DefaultValueOverrides = {}, bool bDeferConstruction = false) const
    {
        return NewObjectRaw<T>(Outer, DefaultValueOverrides, bDeferConstruction);
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

#include "TypeHelpers.h"
