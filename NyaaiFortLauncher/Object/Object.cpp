#include "Object.h"

#include <vector>
#include <ranges>
#include <algorithm>
#include <unordered_map>

static std::vector<NClass*>* AllClassesSortedByHierarchyAndName{};
static std::unordered_map<std::string, NClass*>* AllClassesByByName{};

void FStructWithProperties::SetPropertyValue(const std::string& PropertyName, const std::string& Value)
{
    for (const auto Property : Properties)
    {
        if (Property->GetName() == PropertyName)
        {
            if (!Property->Set(this, Value))
            {
                Log(Error, "Setting property {} to {} in a struct failed.", PropertyName, Value);
            }
            
            return;
        }
    }

    Log(Error, "SetPropertyValue: Failed to find property {} a struct.", PropertyName);
}

NClass::NClass(const std::string& Name, NClass* SuperClass, NObject*(*NewObjectFactory)())
    : Name(Name)
    , SuperClass(SuperClass)
    , NewObjectFactory(NewObjectFactory)
{
    // like this because global variables are bugged in dlls or something idk but this fixes it
    if (!AllClassesSortedByHierarchyAndName) AllClassesSortedByHierarchyAndName = new std::vector<NClass*>{};
    if (!AllClassesByByName) AllClassesByByName = new std::unordered_map<std::string, NClass*>{};

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

FProperty::FProperty(const std::string& Name, NObject* OwningObject, void* Property, bool(*Setter)(void* PropertyPtr, const std::string& Value))
    : Name(Name)
    , Setter(Setter)
{
    Offset = static_cast<uint16>(reinterpret_cast<uint64>(Property) - reinterpret_cast<uint64>(OwningObject));
    OwningObject->Properties.push_back(this);
}

FProperty::FProperty(const std::string& Name, struct FStructWithProperties* OwningObject, void* Property,
    bool(* Setter)(void* PropertyPtr, const std::string& Value))
    : Name(Name)
    , Setter(Setter)
{
    Offset = static_cast<uint16>(reinterpret_cast<uint64>(Property) - reinterpret_cast<uint64>(OwningObject));
    OwningObject->Properties.push_back(this);
}

NObject* NClass::NewObject(NObject* Outer, const std::vector<FPropertySetData>& DefaultValueOverrides, bool bDeferConstruction) const
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

NClass* NClass::GetClassByName(const std::string& Name)
{
    if (auto Search = AllClassesByByName->find(Name); Search != AllClassesByByName->end())
    {
        return Search->second;
    }

    Log(Error, "GetClassByName: Failed to find class {}.", Name);

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

static NClass NObject_Class{"NObject", nullptr, []()
{
    return reinterpret_cast<NObject*>(new NObject{});
}};

void NObject::FinishConstruction()
{
    if (bHasFinishedConstruction)
    {
        Log(Error, "Finish construction was called on an object that has already finished construction {}.", GetClass()->GetName());
        return;
    }
    
    OnCreated();
    bHasFinishedConstruction = true;
}

void NObject::SetPropertyValue(const std::string& PropertyName, const std::string& Value)
{
    for (const auto Property : Properties)
    {
        if (Property->GetName() == PropertyName)
        {
            if (!Property->Set(this, Value))
            {
                Log(Error, "Setting property {} to {} in an object of class {} failed.",
                    PropertyName, Value, GetClass()->GetName());
            }
            
            return;
        }
    }

    Log(Error, "SetPropertyValue: Failed to find property {} in class {}.",
        PropertyName, GetClass()->GetName());
}

NClass* NObject::StaticClass() { return &NObject_Class; }
NClass* NObject::GetClass() const { return &NObject_Class; }

void NObject::Destroy()
{
    OnDestroyed();
    delete this;
}

void NObject::OnCreated()
{
}

void NObject::OnDestroyed()
{
}
