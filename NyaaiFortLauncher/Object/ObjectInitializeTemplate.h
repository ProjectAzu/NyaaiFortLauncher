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
    
    T* NewObject(NObject* Outer = nullptr, bool bDeferConstruction = false) const
    {
        if (!Class)
        {
            Log(Error, L"Cannot initialize template, no class.");
            return nullptr;
        }
        
        return reinterpret_cast<T*>(Class->NewObject(Outer, DefaultValueOverrides, bDeferConstruction));
    }
    
    NPROPERTY(Class)
    NSubClassOf<T> Class = T::StaticClass();

    NPROPERTY(DefaultValueOverrides)
    std::vector<FPropertySetData> DefaultValueOverrides{};
};
