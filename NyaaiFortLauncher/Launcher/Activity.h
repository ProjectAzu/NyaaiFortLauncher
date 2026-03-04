#pragma once

#include "Launcher/EngineObject.h"

class NActivity : public NEngineObject
{
    GENERATE_BASE_H(NActivity)
    
public:
    virtual void Tick(double DeltaTime);
    virtual void OnDestroyed() override;
    
    NActivity* StartChildActivity(const TObjectTemplate<NActivity>& ActivityTemplate);
    template<class T> T* StartChildActivity(const TObjectTemplate<T>& ActivityTemplate)
    {
        return reinterpret_cast<T*>(StartChildActivity(TObjectTemplate<NActivity>{ActivityTemplate}));
    }
    
    const std::vector<NActivity*>& GetChildActivities() const { return ChildActivities; }
    std::vector<NActivity*> GetChildActivitiesIncludingNested() const;
    
    NActivity* FindChildActivity(NSubClassOf<NActivity> Class, bool bSearchInNested = true) const;
    template<class T> T* FindChildActivity(bool bSearchInNested = true) const
    {
        return reinterpret_cast<T*>(FindChildActivity(T::StaticClass(), bSearchInNested));
    }
    
private:
    std::vector<NActivity*> ChildActivities{};
    uint32 ChildActivitiesArrayIndex = 0;
    
    uint64 TickNumber = 0;
    uint64 ParentTickNumber = 0;
};
