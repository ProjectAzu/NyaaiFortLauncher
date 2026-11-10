#pragma once

#define ReflectedClassBase(ClassName)                                                                                                                               \
public:                                                                                                                                                             \
    using Super = ThisClass;                                                                                                                                        \
    using ThisClass = ClassName;                                                                                                                                    \
    static inline NClass ClassName##_Class{ L#ClassName, &GetReflectedSuperClass<ClassName>, GetClassOperations<ClassName>(), &GetInheritedProperties<ClassName> }; \
    static constexpr NClass* StaticClass()                                                                                                                          \
    {                                                                                                                                                               \
        return &ClassName##_Class;                                                                                                                                  \
    }                                                                                                                                                               \
                                                                                                                                                                    \
private:

#define ObjectBase(ClassName) \
public: \
    using Super = ThisObjectClass; \
    using ThisClass = ClassName; \
    using ThisObjectClass = ClassName; \
    virtual ~ClassName() override = default; \
    static inline NObjectClass ClassName##_Class{L#ClassName, &GetReflectedSuperClass<ClassName>, GetClassOperations<ClassName>(), &ConvertToObject<ClassName>, &GetInheritedProperties<ClassName>}; \
    static constexpr NObjectClass* StaticClass() { return &ClassName##_Class; } \
    virtual NObjectClass* GetObjectClass() const override { return StaticClass(); } \
    NObjectClass* GetClass() const { return GetObjectClass(); } \
    virtual NObject* GetCapabilityOwner() override { return this; } \
    virtual const NObject* GetCapabilityOwner() const override { return this; } \
private:

#define CapabilityBase(ClassName) \
public: \
    using Super = ThisCapabilityClass; \
    using ThisClass = ClassName; \
    using ThisCapabilityClass = ClassName; \
    static inline NCapabilityClass ClassName##_Class{L#ClassName, &GetReflectedSuperClass<ClassName>, GetClassOperations<ClassName>(), &GetInheritedProperties<ClassName>}; \
    static constexpr NCapabilityClass* StaticClass() { return &ClassName##_Class; } \
    virtual NCapabilityClass* GetCapabilityClass() const override { return StaticClass(); } \
    NCapabilityClass* GetClass() const { return GetCapabilityClass(); } \
private:

#define ReflectedMember(MemberName) \
static FProperty MemberName##_MakeProperty() \
{ \
    return FProperty::Make<ThisClass, &ThisClass::MemberName>(L#MemberName); \
} \
static inline FPropertyRegistration MemberName##_Registration{ \
    []() -> NClass* { return ThisClass::StaticClass(); }, \
    &MemberName##_MakeProperty, \
    __LINE__ \
};

#define RegisterCapability(CapabilityName) \
static inline FCapabilityRegistrationNode CapabilityName##_Registration{ \
    &ThisClass::StaticClass, \
    []() { return MakeCapabilityRegistration<ThisClass, CapabilityName>(); } \
};
