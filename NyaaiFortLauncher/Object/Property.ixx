export module Reflection:Property;

export import TypeHelpers;

export class NObject;

export struct FProperty
{
public:
    template <class Owner, auto Member>
    static FProperty Make(const wchar_t* Name)
    {
        using Value = std::remove_cvref_t<decltype(std::declval<Owner>().*Member)>;
        using Base = std::conditional_t<std::derived_from<Owner, NObject>, NObject, Owner>;
        FProperty Property{};
        Property.Name = Name;
        Property.Accessor = [](void* Object) -> void*
        {
            return &(scast<Owner*>(scast<Base*>(Object))->*Member);
        };
        Property.ConstAccessor = [](const void* Object) -> const void*
        {
            return &(scast<const Owner*>(scast<const Base*>(Object))->*Member);
        };
        Property.Setter = [](void* Address, const std::wstring& Text)
        {
            return TTypeHelpers<Value, void>::SetFromString(scast<Value*>(Address), Text);
        };
        Property.ValueToStringConverter = [](const void* Address)
        {
            return TTypeHelpers<Value, void>::ToString(scast<const Value*>(Address));
        };
        Property.TypeNameGetter = &TTypeHelpers<Value, void>::GetName;

        if constexpr (std::default_initializable<Owner>)
        {
            Property.DefaultValueGetter = []()
            {
                const Owner Default{};
                return TTypeHelpers<Value, void>::ToString(&(Default.*Member));
            };
        }

        return Property;
    }

    template <class Derived, class Base>
    void Rebase()
    {
        Accessor = [Previous = Accessor](void* Object)
        {
            return Previous(scast<Base*>(scast<Derived*>(Object)));
        };
        ConstAccessor = [Previous = ConstAccessor](const void* Object)
        {
            return Previous(scast<const Base*>(scast<const Derived*>(Object)));
        };
    }

    template <class Owner>
    void SetDefaultOwner()
    {
        DefaultValueGetter = nullptr;

        if constexpr (std::default_initializable<Owner>)
        {
            DefaultValueGetter = [GetAddress = ConstAccessor, Convert = ValueToStringConverter]()
            {
                const Owner Default{};
                using Base = std::conditional_t<std::derived_from<Owner, NObject>, NObject, Owner>;
                return Convert(GetAddress(scast<const Base*>(&Default)));
            };
        }
    }

    std::optional<std::wstring> GetDefaultValue() const
    {
        return DefaultValueGetter ? std::optional(DefaultValueGetter()) : std::nullopt;
    }

    const wchar_t* GetName() const { return Name; }
    std::wstring GetTypeName() const { return TypeNameGetter(); }

    void* GetNativePropertyPtr(void* Object) const
    {
        return Accessor(Object);
    }

    const void* GetNativePropertyPtr(const void* Object) const
    {
        return ConstAccessor(Object);
    }

    inline bool Set(void* Object, const std::wstring& Value) const
    {
        return Setter(GetNativePropertyPtr(Object), Value);
    }

    std::wstring GetAsString(const void* Object) const
    {
        return ValueToStringConverter(GetNativePropertyPtr(Object));
    }

private:
    const wchar_t* Name = nullptr;
    std::wstring (*TypeNameGetter)() = nullptr;
    std::wstring (*ValueToStringConverter)(const void* Value) = nullptr;
    std::function<void*(void*)> Accessor;
    std::function<const void*(const void*)> ConstAccessor;
    std::function<std::wstring()> DefaultValueGetter;
    bool (*Setter)(void* PropertyPtr, const std::wstring& Value) = nullptr;
};
