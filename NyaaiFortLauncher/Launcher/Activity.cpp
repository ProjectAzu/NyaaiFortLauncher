#include "Activity.h"

GENERATE_BASE_CPP(NActivity)

void NActivity::OnCreated()
{
    Super::OnCreated();

    Log(Info, L"Hello from activity {}", GetClass()->GetName());
}

void NActivity::Tick(double DeltaTime)
{
}
