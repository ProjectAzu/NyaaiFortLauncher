#include "Activity.h"

GENERATE_BASE_CPP(NActivity)

void NActivity::Tick(double DeltaTime)
{
    TickNumber++;
    
    for (int32 i = static_cast<int32>(ChildActivities.size()) - 1; (i = std::min(i, static_cast<int32>(ChildActivities.size()) - 1)) >= 0; i--)
    {
        auto Activity = ChildActivities[i];

        if (Activity->ParentTickNumber == TickNumber)
        {
            continue;
        }
        
        Activity->ParentTickNumber = TickNumber;
        Activity->Tick(DeltaTime);
    }
}

void NActivity::OnDestroyed()
{
    for (const auto Activity : ChildActivities)
    {
        Activity->Destroy();
    }
    ChildActivities.clear();
    
    if (auto ParentActivity = GetObjectOfTypeFromOuterChain<NActivity>(true))
    {
        ParentActivity->ChildActivities[ChildActivitiesArrayIndex] = ParentActivity->ChildActivities.back();
        ParentActivity->ChildActivities[ChildActivitiesArrayIndex]->ChildActivitiesArrayIndex = ChildActivitiesArrayIndex;
        ParentActivity->ChildActivities.pop_back();
    }

    Log(Info, L"Activity '{}' is being destroyed", GetClass()->GetName());
    
    Super::OnDestroyed();
}

NActivity* NActivity::StartChildActivity(const TObjectTemplate<NActivity>& ActivityTemplate)
{
    Log(Info, L"Starting child activity '{}' in an object of class '{}'",
        ActivityTemplate.GetClass()->GetName(),
        GetClass()->GetName()
        );
    
    NActivity* Activity = ActivityTemplate.NewObjectRaw(this, true);
    
    Activity->ChildActivitiesArrayIndex = static_cast<uint32>(ChildActivities.size());
    ChildActivities.emplace_back(Activity);
    
    Activity->FinishConstruction();
    
    return Activity;
}
