#include "Object.h"

#include <vector>
#include <ranges>
#include <algorithm>
#include <unordered_map>

static std::vector<NClass*>* AllClassesSortedByHierarchyAndName{};
static std::unordered_map<std::wstring, NClass*>* AllClassesByByName{};

bool FStructWithProperties::SetPropertyValue(const std::wstring& PropertyName, const std::wstring& Value)
{
    for (auto& Property : Properties)
    {
        if (Property.GetName() == PropertyName)
        {
            if (!Property.Set(this, Value))
            {
                Log(Error, L"Setting property {} to {} in a struct failed", PropertyName, Value);
                return false;
            }
            
            return true;
        }
    }

    Log(Error, L"SetPropertyValue: Failed to find property {} in a struct", PropertyName);
    return false;
}

NClass::NClass(const std::wstring& Name, NClass* SuperClass, NObject*(*NewObjectFactory)())
    : Name(Name)
    , SuperClass(SuperClass)
    , NewObjectFactory(NewObjectFactory)
{
    // like this because global variables are bugged in dlls or something idk but this fixes it
    if (!AllClassesSortedByHierarchyAndName) AllClassesSortedByHierarchyAndName = new std::vector<NClass*>{};
    if (!AllClassesByByName) AllClassesByByName = new std::unordered_map<std::wstring, NClass*>{};

    if (AllClassesSortedByHierarchyAndName->empty())
    {
        AllClassesSortedByHierarchyAndName->push_back(nullptr);
    }

    (*AllClassesByByName)[Name] = this;

    // Update Ids, Ids are based on the index inside the name sorted array
    AllClassesSortedByHierarchyAndName->push_back(this);
    std::ranges::sort(*AllClassesSortedByHierarchyAndName, [](const NClass* First, const NClass* Second)
    {
        if (!First)
        {
            return true;
        }

        if (!Second)
        {
            return false;
        }

        if (First->IsSubclassOf(Second))
        {
            return false;
        }

        if (Second->IsSubclassOf(First))
        {
            return true;
        }
        
        return First->GetName() < Second->GetName();
    });
	
    for(int32 i = 0; i < static_cast<int32>(AllClassesSortedByHierarchyAndName->size()); i++)
    {
        if (!(*AllClassesSortedByHierarchyAndName)[i]) continue;
        (*AllClassesSortedByHierarchyAndName)[i]->Id = static_cast<uint16>(i);
    }
}

const NObject* NClass::GetDefaultObject() const
{
    if (!DefaultObject)
    {
        const_cast<NClass*>(this)->DefaultObject = NewObjectFactory();
    }
    
    return DefaultObject;
}

bool NClass::IsSubclassOf(const NClass* Other) const
{
    for (const NClass* Class = this; Class; Class = Class->SuperClass)
    {
        if (Class == Other)
        {
            return true;
        }
    }
    
    return false;
}

FPropertyCreator::FPropertyCreator(const wchar_t* Name, NObject* OwningObject, void* NativeProperty,
    bool(* Setter)(void* PropertyPtr, const std::wstring& Value), std::wstring(* TypeNameGetter)(),
    std::vector<struct FInfoOfStructWithPropertiesUsedInType>(* FInfoOfStructsWithPropertiesUsedInTypeGetter)(),
    std::wstring(* ValueToStringConverter)(const void* Value))
{
    FProperty Property{};
    Property.Offset = static_cast<uint16>(reinterpret_cast<uint64>(NativeProperty) - reinterpret_cast<uint64>(OwningObject));
    Property.Name = Name;
    Property.InfoOfStructsWithPropertiesUsedInTypeGetter = FInfoOfStructsWithPropertiesUsedInTypeGetter;
    Property.ValueToStringConverter = ValueToStringConverter;
    Property.Setter = Setter;
    Property.TypeNameGetter = TypeNameGetter;
    
    OwningObject->Properties.emplace_back(Property);
}

FPropertyCreator::FPropertyCreator(const wchar_t* Name, struct FStructWithProperties* OwningObject, void* NativeProperty,
    bool(* Setter)(void* PropertyPtr, const std::wstring& Value), std::wstring(* TypeNameGetter)(),
    std::vector<struct FInfoOfStructWithPropertiesUsedInType>(* FInfoOfStructsWithPropertiesUsedInTypeGetter)(),
    std::wstring(* ValueToStringConverter)(const void* Value))
{
    FProperty Property{};
    Property.Offset = static_cast<uint16>(reinterpret_cast<uint64>(NativeProperty) - reinterpret_cast<uint64>(OwningObject));
    Property.Name = Name;
    Property.InfoOfStructsWithPropertiesUsedInTypeGetter = FInfoOfStructsWithPropertiesUsedInTypeGetter;
    Property.ValueToStringConverter = ValueToStringConverter;
    Property.Setter = Setter;
    Property.TypeNameGetter = TypeNameGetter;
    
    OwningObject->Properties.emplace_back(Property);
}

NObject* NClass::NewObjectRaw(NObject* Outer, bool bDeferConstruction, const FDefaultValueOverrides& DefaultValueOverrides) const
{
    NObject* NewObject = NewObjectFactory();

    NewObject->OuterPrivate = Outer;

    for (const auto& Override : DefaultValueOverrides)
    {
        NewObject->SetPropertyValue(Override.PropertyName, Override.SetValue);
    }

    if (!bDeferConstruction)
    {
        NewObject->FinishConstruction();
    }
    
    return NewObject;
}

NClass* NClass::GetClassById(uint16 Id)
{
    if (Id < AllClassesSortedByHierarchyAndName->size())
    {
        return (*AllClassesSortedByHierarchyAndName)[Id];
    }

    return nullptr;
}

NClass* NClass::GetClassByName(const std::wstring& Name)
{
    if (auto Search = AllClassesByByName->find(Name); Search != AllClassesByByName->end())
    {
        return Search->second;
    }

    Log(Error, L"GetClassByName: Failed to find class '{}'", Name);

    return nullptr;
}

std::vector<NClass*> NClass::GetAllDerivedClasses() const
{
    std::vector<NClass*> Result{};
    
    for (const auto Class : *AllClassesSortedByHierarchyAndName)
    {
        if (Class != this && Class->IsSubclassOf(this))
        {
            Result.push_back(Class);
        }
    }

    return Result;
}

static NClass NObject_Class{L"NObject", nullptr, []()
{
    return reinterpret_cast<NObject*>(new NObject{});
}};

void NObject::FinishConstruction()
{
    if (bHasFinishedConstruction)
    {
        Log(Error, L"Finish construction was called on an object that has already finished construction {}", GetClass()->GetName());
        return;
    }
    
    bIsInsideOnCreated = true;
    OnCreated();
    bIsInsideOnCreated = false;

    bHasFinishedConstruction = true;
}

bool NObject::SetPropertyValue(const std::wstring& PropertyName, const std::wstring& Value)
{
    for (auto& Property : Properties)
    {
        if (Property.GetName() == PropertyName)
        {
            if (!Property.Set(this, Value))
            {
                Log(Error, L"Setting property {} to {} in an object of class {} failed",
                    PropertyName, Value, GetClass()->GetName());
                
                return false;
            }
            
            return true;
        }
    }

    Log(Error, L"SetPropertyValue: Failed to find property {} in class {}",
        PropertyName, GetClass()->GetName());
    
    return false;
}

FDefaultValueOverrides NObject::GetDefaultValueOverrides() const
{
    FDefaultValueOverrides DefaultValueOverrides{};
    DefaultValueOverrides.reserve(Properties.size());

    for (const FProperty& Property : Properties)
    {
        FPropertySetData PropertySetData{};
        PropertySetData.PropertyName = Property.GetName();
        PropertySetData.SetValue = Property.GetAsString(this);

        DefaultValueOverrides.emplace_back(std::move(PropertySetData));
    }

    return DefaultValueOverrides;
}

NClass* NObject::StaticClass() { return &NObject_Class; }
NClass* NObject::GetClass() const { return &NObject_Class; }

NObject* NObject::GetObjectOfTypeFromOuterChain(NClass* Class, bool bIgnoreSelf) const
{
    for (NObject* Object = const_cast<NObject*>(bIgnoreSelf ? GetOuter() : this); Object; Object = Object->GetOuter())
    {
        if (Object->GetClass()->IsSubclassOf(Class))
        {
            return Object;
        }
    }
    
    return nullptr;
}

bool NObject::IsObjectPartOfOuterChain(const NObject* InObject) const
{
    for (NObject* Object = const_cast<NObject*>(this); Object; Object = Object->GetOuter())
    {
        if (Object == InObject)
        {
            return true;
        }
    }
    
    return false;
}

void NObject::Destroy()
{
    if (bIsInsideOnCreated)
    {
        Log(Error, L"{}::Destroy() was called inside {}::OnCreated(). This is not allowed", GetClass()->GetName(), GetClass()->GetName());
        __debugbreak();
    }

    if (bHasFinishedConstruction)
    {
        OnDestroyed();
    }

    delete this;
}

void NObject::OnCreated()
{
}

void NObject::OnDestroyed()
{
}