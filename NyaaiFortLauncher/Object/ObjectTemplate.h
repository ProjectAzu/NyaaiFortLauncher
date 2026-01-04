#pragma once

#include "Object.h"

template<class T>
struct TObjectTemplate : FStructWithProperties
{
    TObjectTemplate() = default;

    template<class Other>
    TObjectTemplate(const TObjectTemplate<Other>& OtherTemplate)
        : Class(OtherTemplate.GetClass()), DefaultValueOverrides(OtherTemplate.GetDefaultValueOverrides())
    {
    }
    
    TObjectTemplate(NSubClassOf<T> Class) : Class(Class)
    {
    }
    
    TObjectTemplate(NClass* Class) : Class(Class)
    {
    }
    
    static std::wstring GetName()
    { 
        return std::format(L"TObjectTemplate<{}>", T::StaticClass()->GetName());
    }
    
    T* NewObjectRaw(NObject* Outer = nullptr, bool bDeferConstruction = false) const
    {
        if (!Class)
        {
            Log(Error, L"{}::NewObjectRaw:, Cannot initialize template, no class.", GetName());
            return nullptr;
        }
        
        return reinterpret_cast<T*>(Class->NewObjectRaw(Outer, bDeferConstruction, DefaultValueOverrides));
    }
    
    NUniquePtr<T> NewObject(NObject* Outer = nullptr, bool bDeferConstruction = false) const
    {
        return NewObjectRaw(Outer, bDeferConstruction);
    }
    
    template<class ReturnType>
    NUniquePtr<ReturnType> MakeNativeTemplate() const
    {
        if (!Class)
        {
            Log(Error, L"{}::MakeNativeTemplate<{}>: Cannot make native template, no class is set.", 
                GetName(), ReturnType::StaticClass()->GetName());
            
            return nullptr;
        }
        
        if (!ReturnType::StaticClass()->IsSubclassOf(Class))
        {
            Log(Error, L"{}::MakeNativeTemplate<{}>: Cannot make native template, '{}' is not a subclass of the set class '{}'.", 
                GetName(), ReturnType::StaticClass()->GetName(), ReturnType::StaticClass()->GetName(), Class->GetName());
            
            return nullptr;
        }
        
        // using deferred init and never finishing construction because it's a template object
        return reinterpret_cast<ReturnType*>(Class->NewObject(nullptr, DefaultValueOverrides, true));
    }
    
    NUniquePtr<T> MakeNativeTemplate() const
    {
        return MakeNativeTemplate<T>();
    }
    
    void SetFromNativeTemplate(const T* NativeTemplate)
    {
        DefaultValueOverrides.clear();
        
        if (!NativeTemplate)
        {
            Class = nullptr;
            return;
        }
        
        Class = NativeTemplate->GetClass();
        
        for (const FProperty* Property : NativeTemplate->GetPropertiesArrayConstRef())
        {
            FPropertySetData PropertySetData{};
            PropertySetData.PropertyName = Property->GetName();
            PropertySetData.SetValue = Property->GetAsString(NativeTemplate);
            
            DefaultValueOverrides.emplace_back(std::move(PropertySetData));
        }
    }
    
    void Reset(NSubClassOf<T> InClass)
    {
        DefaultValueOverrides.clear();
        Class = InClass;
    }
    
    // Modifies the currently set class while preserving the default value overrides
    void ModifyClass(NSubClassOf<T> InClass)
    {
        if (!InClass)
        {
            Class = nullptr;
            DefaultValueOverrides.clear();
            
            Log(Error, L"{}::ModifyClass: InClass can't be nullptr. If this is intended, use Reset(nullptr).", GetName());
            
            return;
        }
        
        if (Class && !InClass->IsSubclassOf(Class))
        {
            DefaultValueOverrides.clear();
            
            Log(Error, L"{}::ModifyClass: InClass '{}' has to be a subclass of the currently set class '{}'.", 
                GetName(), InClass->GetName(), Class->GetName());
        }
        
        Class = InClass;
    }
    
    NSubClassOf<T> GetClass() const { return Class; }
    FDefaultValueOverrides GetDefaultValueOverrides() const { return DefaultValueOverrides; }
    
    operator bool() const
    {
        return Class.Get() != nullptr;
    }
    
protected:
    NPROPERTY(Class)
    NSubClassOf<T> Class = nullptr;
    
    NPROPERTY(DefaultValueOverrides)
    FDefaultValueOverrides DefaultValueOverrides{};
};