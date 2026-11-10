module Reflection;

static std::vector<NClass*> AllClassesSortedByHierarchyAndName{};
static constinit NClass* FirstClass = nullptr;
static constinit FPropertyRegistration* FirstProperty = nullptr;
static constinit FCapabilityRegistrationNode* FirstCapability = nullptr;
static std::unordered_map<std::wstring, NClass*> AllClassesByName{};

static NObjectClass NObject_Class{ L"NObject", &GetReflectedSuperClass<NObject>, GetClassOperations<NObject>(), &ConvertToObject<NObject>, &GetInheritedProperties<NObject> };

NObjectClass* NObject::StaticClass()
{
    return &NObject_Class;
}

static NObjectClass NClass_Class{ L"NClass", &GetReflectedSuperClass<NClass>, GetClassOperations<NClass>(), &ConvertToObject<NClass>, &GetInheritedProperties<NClass> };

NObjectClass* NClass::StaticClass()
{
    return &NClass_Class;
}

NObjectClass* NClass::GetObjectClass() const
{
    return StaticClass();
}

static NObjectClass NObjectClass_Class{ L"NObjectClass", &GetReflectedSuperClass<NObjectClass>, GetClassOperations<NObjectClass>(), &ConvertToObject<NObjectClass>, &GetInheritedProperties<NObjectClass> };

NObjectClass* NObjectClass::StaticClass()
{
    return &NObjectClass_Class;
}

NObjectClass* NObjectClass::GetObjectClass() const
{
    return StaticClass();
}

static NObjectClass NCapabilityClass_Class{ L"NCapabilityClass", &GetReflectedSuperClass<NCapabilityClass>, GetClassOperations<NCapabilityClass>(), &ConvertToObject<NCapabilityClass>, &GetInheritedProperties<NCapabilityClass> };

NObjectClass* NCapabilityClass::StaticClass()
{
    return &NCapabilityClass_Class;
}

NObjectClass* NCapabilityClass::GetObjectClass() const
{
    return StaticClass();
}

static NCapabilityClass CCapability_Class{ L"CCapability", nullptr, GetClassOperations<CCapability>() };

NCapabilityClass* CCapability::StaticClass()
{
    return &CCapability_Class;
}

static void SortClassesByHierarchyAndName(std::vector<NClass*>& Classes)
{
    std::unordered_map<const NClass*, std::vector<const NClass*>> PathCache;
    PathCache.reserve(Classes.size());

    std::unordered_map<const NClass*, std::wstring> NameCache;
    NameCache.reserve(Classes.size());

    const auto GetPath = [&](const NClass* Class) -> const std::vector<const NClass*>&
    {
        if (auto It = PathCache.find(Class); It != PathCache.end())
        {
            return It->second;
        }

        std::vector<const NClass*> Path;

        for (const NClass* It = Class; It; It = It->GetSuper())
        {
            Path.push_back(It);
        }

        std::reverse(Path.begin(), Path.end());

        return PathCache.emplace(Class, std::move(Path)).first->second;
    };

    const auto GetName = [&](const NClass* Class) -> const std::wstring&
    {
        if (auto It = NameCache.find(Class); It != NameCache.end())
        {
            return It->second;
        }

        return NameCache.emplace(Class, Class->GetName()).first->second;
    };

    std::ranges::sort(Classes, [&](const NClass* A, const NClass* B)
    {
        if (A == B)
        {
            return false;
        }

        if (!A)
        {
            return true;
        }

        if (!B)
        {
            return false;
        }

        const auto& PA = GetPath(A);
        const auto& PB = GetPath(B);

        const std::size_t N = std::min(PA.size(), PB.size());

        for (std::size_t i = 0; i < N; ++i)
        {
            if (PA[i] == PB[i])
            {
                continue;
            }

            const std::wstring& NA = GetName(PA[i]);
            const std::wstring& NB = GetName(PB[i]);

            if (NA != NB)
            {
                return NA < NB;
            }

            return PA[i] < PB[i];
        }

        if (PA.size() != PB.size())
        {
            return PA.size() < PB.size();
        }

        const std::wstring& NA = GetName(A);
        const std::wstring& NB = GetName(B);

        if (NA != NB)
        {
            return NA < NB;
        }

        return A < B;
    });
}

NClass::NClass(const wchar_t* Name, NClass* (*SuperClassGetter)(), FClassOperations Operations, std::vector<FProperty> (*InheritedPropertiesGetter)())
    : Name(Name), SuperClassGetter(SuperClassGetter), Operations(Operations), InheritedPropertiesGetter(InheritedPropertiesGetter)
{
    Next = FirstClass;
    FirstClass = this;
}

NObjectClass::NObjectClass(const wchar_t* Name, NClass* (*SuperClassGetter)(), FClassOperations Operations, NObject* (*ObjectConverter)(void*), std::vector<FProperty> (*InheritedPropertiesGetter)())
    : NClass(Name, SuperClassGetter, Operations, InheritedPropertiesGetter), ObjectConverter(ObjectConverter)
{
}

FPropertyRegistration::FPropertyRegistration(NClass* (*OwnerGetter)(), FProperty (*PropertyFactory)(), uint32 Order)
    : OwnerGetter(OwnerGetter), PropertyFactory(PropertyFactory), Order(Order)
{
    Next = FirstProperty;
    FirstProperty = this;
}

FCapabilityRegistrationNode::FCapabilityRegistrationNode(NObjectClass* (*OwnerGetter)(), FCapabilityRegistration (*Factory)())
    : OwnerGetter(OwnerGetter), Factory(Factory)
{
    Next = FirstCapability;
    FirstCapability = this;
}

void InitReflection()
{
    std::vector<NClass*> Classes{};

    for (NClass* Class = FirstClass; Class; Class = Class->Next)
    {
        Class->SuperClass = Class->SuperClassGetter ? Class->SuperClassGetter() : nullptr;
        Classes.push_back(Class);
    }

    SortClassesByHierarchyAndName(Classes);

    if (Classes.size() >= std::numeric_limits<uint16>::max())
    {
        throw std::runtime_error("Too many reflected classes");
    }

    std::vector<FPropertyRegistration*> Members{};

    for (auto Member = FirstProperty; Member; Member = Member->Next)
    {
        Members.push_back(Member);
    }

    std::ranges::sort(Members, {}, &FPropertyRegistration::Order);
    uint16 Id = 1;

    for (NClass* Class : Classes)
    {
        if (!AllClassesByName.emplace(Class->GetName(), Class).second)
        {
            throw std::runtime_error("Duplicate reflected class name");
        }

        Class->Id = Id++;

        if (Class->SuperClass)
        {
            Class->Properties = Class->InheritedPropertiesGetter ? Class->InheritedPropertiesGetter() : Class->SuperClass->Properties;
        }

        for (auto Member : Members)
        {
            if (Member->OwnerGetter() != Class)
            {
                continue;
            }

            Class->Properties.push_back(Member->PropertyFactory());
        }
    }

    for (auto Capability = FirstCapability; Capability; Capability = Capability->Next)
    {
        Capability->OwnerGetter()->Capabilities.push_back(Capability->Factory());
    }

    Classes.insert(Classes.begin(), nullptr);
    AllClassesSortedByHierarchyAndName = std::move(Classes);
}

const FCapabilityRegistration* NObjectClass::GetCapabilityRegistration(const NCapabilityClass* Capability) const
{
    for (const auto& Registration : Capabilities)
    {
        if (Registration.Class->IsSubclassOf(Capability))
        {
            return &Registration;
        }
    }

    return GetSuper() ? scast<const NObjectClass*>(GetSuper())->GetCapabilityRegistration(Capability) : nullptr;
}

const std::vector<FProperty>& NObject::GetPropertiesArrayConstRef() const
{
    return GetClass()->GetProperties();
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

NObject* NObjectClass::NewObjectRaw(NObject* Outer, bool bDeferConstruction, const FDefaultValueOverrides& DefaultValueOverrides) const
{
    if (!CanDefaultConstructObject())
    {
        return nullptr;
    }

    NObject* NewObject = ObjectConverter(MakeDefaultConstructedObject());

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
    if (Id < AllClassesSortedByHierarchyAndName.size())
    {
        return AllClassesSortedByHierarchyAndName[Id];
    }

    return nullptr;
}

NClass* NClass::GetClassByName(const std::wstring& Name)
{
    if (auto Search = AllClassesByName.find(Name); Search != AllClassesByName.end())
    {
        return Search->second;
    }

    Log(Error, L"GetClassByName: Failed to find class '{}'", Name);

    return nullptr;
}

std::vector<NClass*> NClass::GetAllDerivedClasses(bool bIncludeSelf) const
{
    std::vector<NClass*> Result{};

    for (const auto Class : AllClassesSortedByHierarchyAndName)
    {
        if (!Class || (!bIncludeSelf && Class == this))
        {
            continue;
        }

        if (Class->IsSubclassOf(this))
        {
            Result.push_back(Class);
        }
    }

    return Result;
}

std::span<NClass* const> NClass::GetAllClasses()
{
    return std::span<NClass* const>{ AllClassesSortedByHierarchyAndName }.subspan(1);
}

void NObject::FinishConstruction()
{
    if (bHasFinishedConstruction)
    {
        Log(Error, L"Finish construction was called on an object that has already finished construction {}", GetClass()->GetName());

        return;
    }

    bIsInsideOnCreated = true;
    OnCreated();
    bIsInsideOnCreated = false;

    bHasFinishedConstruction = true;
}

bool NObject::SetPropertyValue(const std::wstring& PropertyName, const std::wstring& Value)
{
    return GetClass()->SetPropertyValue(this, PropertyName, Value);
}

bool NClass::SetPropertyValue(void* Instance, const std::wstring& PropertyName, const std::wstring& Value) const
{
    for (const auto& Property : Properties)
    {
        if (Property.GetName() != PropertyName)
        {
            continue;
        }

        return Property.Set(Instance, Value);
    }

    Log(Error, L"SetPropertyValue: Failed to find property {} in class {}", PropertyName, GetName());
    return false;
}

FDefaultValueOverrides NObject::GetDefaultValueOverrides() const
{
    FDefaultValueOverrides DefaultValueOverrides{};
    DefaultValueOverrides.reserve(GetPropertiesArrayConstRef().size());

    for (const FProperty& Property : GetPropertiesArrayConstRef())
    {
        FPropertySetData PropertySetData{};
        PropertySetData.PropertyName = Property.GetName();
        PropertySetData.SetValue = Property.GetAsString(this);

        DefaultValueOverrides.emplace_back(std::move(PropertySetData));
    }

    return DefaultValueOverrides;
}

NObjectClass* NObject::GetObjectClass() const
{
    return StaticClass();
}

NObject* NObject::GetObjectOfTypeFromOuterChain(NClass* Class, bool bIgnoreSelf) const
{
    for (NObject* Object = ccast<NObject*>(bIgnoreSelf ? GetOuter() : this); Object; Object = Object->GetOuter())
    {
        if (Object->GetClass()->IsSubclassOf(Class))
        {
            return Object;
        }
    }

    return nullptr;
}

bool NObject::IsObjectPartOfOuterChain(const NObject* InObject) const
{
    for (NObject* Object = ccast<NObject*>(this); Object; Object = Object->GetOuter())
    {
        if (Object == InObject)
        {
            return true;
        }
    }

    return false;
}

void NObject::Destroy()
{
    if (bIsInsideOnCreated)
    {
        Log(Error, L"{}::Destroy() was called inside {}::OnCreated(). This is not allowed", GetClass()->GetName(), GetClass()->GetName());
        __debugbreak();
    }

    if (bHasFinishedConstruction)
    {
        OnDestroyed();
    }

    delete this;
}

void NObject::OnCreated()
{
}

void NObject::OnDestroyed()
{
}

void PrintClassesInfo(const NClass* BaseClass)
{
    Log(Info, L"Printing reflected classes and their properties with default values:");

    for (NClass* Class : NClass::GetAllClasses())
    {
        if (BaseClass && !Class->IsSubclassOf(BaseClass))
        {
            continue;
        }

        LogRaw(L"\n");
        LogRaw(std::format(L"class {}{}\n{{\n", Class->GetName(), Class->GetSuper() ? L" : " + Class->GetSuper()->GetName() : L""));

        for (const auto& Property : Class->GetProperties())
        {
            LogRaw(std::format(L"    {} {} = {{{}}};\n", Property.GetTypeName(), Property.GetName(), Property.GetDefaultValue().value_or(L"<no default constructor>")));
        }

        LogRaw(L"}\n");
    }

    LogRaw(L"\n");
}
