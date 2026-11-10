module;
#include "Object/ReflectionMacros.h"

export module Engine:Activity;

export import ObjectTemplate;
export import :EngineObject;

export class NActivity : public NEngineObject
{
    ObjectBase(NActivity)

public:
    virtual void Tick(double DeltaTime);
    virtual void OnDestroyed() override;

    NActivity* StartChildActivity(const TObjectTemplate<NActivity>& ActivityTemplate);
    template <class T>
    T* StartChildActivity(const TObjectTemplate<T>& ActivityTemplate)
    {
        return rcast<T*>(StartChildActivity(TObjectTemplate<NActivity>{ ActivityTemplate }));
    }

    const std::vector<NActivity*>& GetChildActivities() const { return ChildActivities; }
    std::vector<NActivity*> GetChildActivitiesIncludingNested() const;

    NActivity* FindChildActivity(TSubClassOf<NActivity> Class, bool bSearchInNested = true) const;
    template <class T>
    T* FindChildActivity(bool bSearchInNested = true) const
    {
        return rcast<T*>(FindChildActivity(T::StaticClass(), bSearchInNested));
    }

private:
    std::vector<NActivity*> ChildActivities{};
    uint32 ChildActivitiesArrayIndex = 0;

    uint64 TickNumber = 0;
    uint64 ParentTickNumber = 0;
};
