#pragma once

#include "Object.h"

template<class T>
struct TObjectInitializeTemplate : FStructWithProperties
{
    TObjectInitializeTemplate() = default;

    template<class Other>
    TObjectInitializeTemplate(const TObjectInitializeTemplate<Other>& OtherTemplate)
        : Class(OtherTemplate.Class), DefaultValueOverrides(OtherTemplate.DefaultValueOverrides)
    {
    }
    
    static std::wstring GetName()
    { 
        return std::format(L"TObjectInitializeTemplate<{}>", T::StaticClass()->GetName());
    }
    
    T* NewObjectRaw(NObject* Outer = nullptr, bool bDeferConstruction = false) const
    {
        if (!Class)
        {
            Log(Error, L"Cannot initialize template, no class.");
            return nullptr;
        }
        
        return reinterpret_cast<T*>(Class->NewObjectRaw(Outer, DefaultValueOverrides, bDeferConstruction));
    }
    
    NUniquePtr<T> NewObject(NObject* Outer = nullptr, bool bDeferConstruction = false) const
    {
        return NewObjectRaw(Outer, bDeferConstruction);
    }
    
    NUniquePtr<T> MakeNativeTemplate()
    {
        if (!Class)
        {
            Log(Error, L"Cannot make native template, no class.");
            return nullptr;
        }
        
        // using deferred init and never finishing construction because it's a template object
        return reinterpret_cast<T*>(Class->NewObject(nullptr, DefaultValueOverrides, true));
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

    NPROPERTY(Class)
    NSubClassOf<T> Class = T::StaticClass();
    
protected:
    NPROPERTY(DefaultValueOverrides)
    FDefaultValueOverrides DefaultValueOverrides{};
};