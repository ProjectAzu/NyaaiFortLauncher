export module ObjectTemplate;

export import Reflection;
export import SubClassOf;

export template <class T>
struct TObjectTemplate
{
    TObjectTemplate() = default;

    TObjectTemplate(const TObjectTemplate& OtherTemplate)
    {
        CopyFrom(OtherTemplate);
    }

    template <class OtherT>
    TObjectTemplate(const TObjectTemplate<OtherT>& OtherTemplate)
    {
        CopyFrom(OtherTemplate);
    }

    TObjectTemplate(TSubClassOf<T> Class)
    {
        Reset(Class);
    }

    TObjectTemplate(NClass* Class)
    {
        Reset(Class);
    }

    TObjectTemplate(TObjectTemplate&&) noexcept = default;
    TObjectTemplate& operator=(TObjectTemplate&&) noexcept = default;

    TObjectTemplate& operator=(const TObjectTemplate& OtherTemplate)
    {
        CopyFrom(OtherTemplate);

        return *this;
    }

    template <class OtherT>
    TObjectTemplate& operator=(const TObjectTemplate<OtherT>& OtherTemplate)
    {
        CopyFrom(OtherTemplate);

        return *this;
    }

    TObjectTemplate(TSubClassOf<T> Class, const FDefaultValueOverrides& DefaultValueOverrides)
    {
        if (!Class)
        {
            TemplateObject.Reset(nullptr);

            return;
        }

        TemplateObject.Reset(Class->template NewObjectRaw<T>(nullptr, true, DefaultValueOverrides));
    }

    T* NewObjectRaw(NObject* Outer = nullptr, bool bDeferConstruction = false) const
    {
        if (!TemplateObject)
        {
            Log(Error, L"TObjectTemplate<T>::NewObjectRaw:, Cannot initialize template, no TemplateObject");

            return nullptr;
        }

        return TemplateObject->GetClass()->template NewObjectRaw<T>(Outer, bDeferConstruction, GetDefaultValueOverrides());
    }

    TUniqueObjectPtr<T> NewObject(NObject* Outer = nullptr, bool bDeferConstruction = false) const
    {
        return NewObjectRaw(Outer, bDeferConstruction);
    }

    void Reset(TSubClassOf<T> InClass)
    {
        if (!InClass)
        {
            TemplateObject.Reset(nullptr);

            return;
        }

        TemplateObject = InClass->template NewObject<T>(nullptr, true);
    }

    // Modifies the currently set class while preserving the default value overrides
    void ModifyClass(TSubClassOf<T> InClass)
    {
        if (!InClass)
        {
            TemplateObject.Reset(nullptr);

            Log(Error, L"TObjectTemplate<T>::ModifyClass: InClass can't be nullptr. If this is intended, use Reset(nullptr)");

            return;
        }

        if (TemplateObject && !InClass->IsSubclassOf(TemplateObject->GetClass()))
        {
            Log(Error, L"TObjectTemplate<T>::ModifyClass: InClass '{}' has to be a subclass of the currently set class '{}'", InClass->GetName(), TemplateObject->GetClass()->GetName());

            Reset(InClass);

            return;
        }

        TemplateObject.Reset(InClass->template NewObjectRaw<T>(nullptr, true, GetDefaultValueOverrides()));
    }

    FDefaultValueOverrides GetDefaultValueOverrides() const
    {
        if (!TemplateObject)
        {
            return {};
        }

        return TemplateObject->GetDefaultValueOverrides();
    }

    TSubClassOf<T> GetClass() const
    {
        return TemplateObject ? TemplateObject->GetClass() : nullptr;
    }

    T* operator->() const
    {
        return TemplateObject;
    }

    operator bool() const
    {
        return TemplateObject != nullptr;
    }

private:
    template <class OtherT>
    void CopyFrom(const TObjectTemplate<OtherT>& OtherTemplate)
    {
        if (!OtherTemplate.GetClass())
        {
            TemplateObject.Reset(nullptr);

            return;
        }

        TemplateObject.Reset(OtherTemplate.NewObjectRaw(nullptr, true));
    }

    TUniqueObjectPtr<T> TemplateObject = nullptr;
};

template <class T>
struct TTypeHelpers<TObjectTemplate<T>>
{
    static bool SetFromString(TObjectTemplate<T>* Property, const std::wstring& Value)
    {
        *Property = {};

        std::wstring ClassString{};

        int32 OpeningBraceIndex = -1;

        for (int32 i = 0; i < scast<int32>(Value.size()); i++)
        {
            if (Value[i] == L'{')
            {
                OpeningBraceIndex = i;
                break;
            }

            ClassString += Value[i];
        }

        std::wstring DefaultValueOverridesString{};

        if (OpeningBraceIndex >= 0)
        {
            auto CleanString = RemoveUnnecessaryCharsFromString(Value);

            if (CleanString.empty())
            {
                Log(Error, L"{} Setter failed, no closing brace at end", GetName());
            }

            if (CleanString.back() != L'}')
            {
                Log(Error, L"{} Setter failed, no closing brace at end, instead found: {}", GetName(), ClassString.back());

                return false;
            }

            int32 ClosingBraceIndex = -1;

            for (int32 i = scast<int32>(Value.size()) - 1; i > OpeningBraceIndex; i--)
            {
                if (Value[i] == L'}')
                {
                    ClosingBraceIndex = i;
                    break;
                }
            }

            if (ClosingBraceIndex > OpeningBraceIndex)
            {
                DefaultValueOverridesString = std::wstring(Value.begin() + OpeningBraceIndex + 1, Value.begin() + ClosingBraceIndex);
            }
        }

        TSubClassOf<T> Class{};

        if (!TTypeHelpers<TSubClassOf<T>>::SetFromString(&Class, ClassString))
        {
            return false;
        }

        if (!Class)
        {
            return true;
        }

        FDefaultValueOverrides DefaultValueOverrides{};

        if (!TTypeHelpers<FDefaultValueOverrides>::SetFromString(&DefaultValueOverrides, DefaultValueOverridesString))
        {
            return false;
        }

        *Property = TObjectTemplate<T>(Class, DefaultValueOverrides);

        return true;
    }

    static std::wstring ToString(const TObjectTemplate<T>* Value)
    {
        if (!Value->GetClass())
        {
            return {};
        }

        FDefaultValueOverrides DefaultValueOverrides = Value->GetDefaultValueOverrides();

        return std::format(L"{}{{{}}}",
            Value->GetClass()->GetName(),
            TTypeHelpers<FDefaultValueOverrides>::ToString(&DefaultValueOverrides));
    }

    static std::wstring GetName() { return std::format(L"TObjectTemplate<{}>", T::StaticClass()->GetName()); }
};
