module Engine;

void NActivity::Tick(double DeltaTime)
{
    TickNumber++;

    for (int32 i = scast<int32>(ChildActivities.size()) - 1; (i = std::min(i, scast<int32>(ChildActivities.size()) - 1)) >= 0; i--)
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
    while (!ChildActivities.empty())
    {
        ChildActivities.back()->Destroy();
    }

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
    Log(Info, L"Starting child activity '{}' in an object of class '{}'", ActivityTemplate.GetClass()->GetName(), GetClass()->GetName());

    NActivity* Activity = ActivityTemplate.NewObjectRaw(this, true);

    Activity->ChildActivitiesArrayIndex = scast<uint32>(ChildActivities.size());
    ChildActivities.emplace_back(Activity);

    Activity->FinishConstruction();

    return Activity;
}

std::vector<NActivity*> NActivity::GetChildActivitiesIncludingNested() const
{
    std::vector<const NActivity*> Stack{};

    for (const auto ChildActivity : ChildActivities)
    {
        Stack.push_back(ChildActivity);
    }

    std::vector<NActivity*> Result{};

    while (!Stack.empty())
    {
        auto Activity = Stack.back();
        Stack.pop_back();

        Result.push_back(ccast<NActivity*>(Activity));

        for (const auto ChildActivity : Activity->ChildActivities)
        {
            Stack.push_back(ChildActivity);
        }
    }

    return Result;
}

NActivity* NActivity::FindChildActivity(TSubClassOf<NActivity> Class, bool bSearchInNested) const
{
    std::vector<NActivity*> Activities = bSearchInNested ? GetChildActivitiesIncludingNested() : GetChildActivities();

    for (const auto Activity : Activities)
    {
        if (Activity->GetClass()->IsSubclassOf(Class))
        {
            return Activity;
        }
    }

    return nullptr;
}
