#pragma once

#include "Object.h"

template<class T>
struct TObjectTemplate : FStructWithProperties
{
    TObjectTemplate() = default;

    template<class Other>
    TObjectTemplate(const TObjectTemplate<Other>& OtherTemplate)
        : Class(OtherTemplate.Class), DefaultValueOverrides(OtherTemplate.DefaultValueOverrides)
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
            Log(Error, L"Cannot initialize template, no class.");
            return nullptr;
        }
        
        return reinterpret_cast<T*>(Class->NewObjectRaw(Outer, DefaultValueOverrides, bDeferConstruction));
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
            Log(Error, L"Cannot make native template, no class.");
            return nullptr;
        }
        
        if (!ReturnType::StaticClass()->IsSubclassOf(Class))
        {
            Log(Error, L"Cannot make native template, bad return type.");
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

    NPROPERTY(Class)
    NSubClassOf<T> Class = T::StaticClass();
    
protected:
    NPROPERTY(DefaultValueOverrides)
    FDefaultValueOverrides DefaultValueOverrides{};
};