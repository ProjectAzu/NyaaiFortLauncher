#pragma once

#include <string>
#include <sstream>
#include <utility>
#include <filesystem>
#include <memory>
#include <unordered_set>

#include "Object.h"

// leaves only alphabetic, digits and punctuation
std::wstring RemoveUnnecessaryCharsFromString(const std::wstring& Input);

struct FInfoOfStructWithPropertiesUsedInType
{
    std::unique_ptr<FStructWithProperties, void(*)(FStructWithProperties*)> SampleObjectHolder{nullptr, [](FStructWithProperties*)
    {
        Log(Error, L"FStructWithProperties Sample deleter not provided");
    }};
    
    std::wstring TypeName{};
};

template <typename T, typename Enable = void>
struct TTypeHelpers
{
    // SetFromString and ToString should be compatible
    static bool SetFromString(T* Property, const std::wstring& Value)
    {
        Log(Warning, L"Using default TTypeHelpers, please add TTypeHelpers for this type.");
        std::wstringstream(RemoveUnnecessaryCharsFromString(Value)) >> *Property;
        return true;
    }
    
    // SetFromString and ToString should be compatible
    static std::wstring ToString(const T* Value)
    {
        Log(Warning, L"Using default TTypeHelpers, please add TTypeHelpers for this type.");
        
        std::wstringstream Stream{};
        Stream << *Value;
        return Stream.str();
    }
    
    static std::wstring GetName()
    {
        Log(Warning, L"Using default TTypeHelpers, please add TTypeHelpers for this type.");
        return L"Unknown";
    }
    
    static std::vector<FInfoOfStructWithPropertiesUsedInType> GetInfoOfStructsWithPropertiesUsedInType()
    {
        Log(Warning, L"Using default TTypeHelpers, please add TTypeHelpers for this type.");
        return {};
    }
};

template<>
struct TTypeHelpers<uint8>
{
    static bool SetFromString(uint8* Property, const std::wstring& Value)
    {
        std::wstringstream ss(RemoveUnnecessaryCharsFromString(Value));
        
        uint32 Uint32 = 0;
        ss >> Uint32;
        
        *Property = static_cast<uint8>(Uint32);
        
        return true;
    }
    
    static std::wstring ToString(const uint8* Value)
    {
        uint32 Uint32 = static_cast<uint32>(*Value);
        
        std::wstringstream Stream{};
        Stream << Uint32;
        return Stream.str();
    }
    
    static std::wstring GetName() { return L"uint8"; }
    static std::vector<FInfoOfStructWithPropertiesUsedInType> GetInfoOfStructsWithPropertiesUsedInType() { return {}; }
};

template<>
struct TTypeHelpers<uint16>
{
    static bool SetFromString(uint16* Property, const std::wstring& Value)
    {
        std::wstringstream(RemoveUnnecessaryCharsFromString(Value)) >> *Property;
        return true;
    }
    
    static std::wstring ToString(const uint16* Value)
    {
        std::wstringstream Stream{};
        Stream << *Value;
        return Stream.str();
    }
    
    static std::wstring GetName() { return L"uint16"; }
    static std::vector<FInfoOfStructWithPropertiesUsedInType> GetInfoOfStructsWithPropertiesUsedInType() { return {}; }
};

template<>
struct TTypeHelpers<uint32>
{
    static bool SetFromString(uint32* Property, const std::wstring& Value)
    {
        std::wstringstream(RemoveUnnecessaryCharsFromString(Value)) >> *Property;
        return true;
    }
    
    static std::wstring ToString(const uint32* Value)
    {
        std::wstringstream Stream{};
        Stream << *Value;
        return Stream.str();
    }
    
    static std::wstring GetName() { return L"uint32"; }
    static std::vector<FInfoOfStructWithPropertiesUsedInType> GetInfoOfStructsWithPropertiesUsedInType() { return {}; }
};

template<>
struct TTypeHelpers<uint64>
{
    static bool SetFromString(uint64* Property, const std::wstring& Value)
    {
        std::wstringstream(RemoveUnnecessaryCharsFromString(Value)) >> *Property;
        return true;
    }
    
    static std::wstring ToString(const uint64* Value)
    {
        std::wstringstream Stream{};
        Stream << *Value;
        return Stream.str();
    }
    
    static std::wstring GetName() { return L"uint64"; }
    static std::vector<FInfoOfStructWithPropertiesUsedInType> GetInfoOfStructsWithPropertiesUsedInType() { return {}; }
};

template<>
struct TTypeHelpers<int8>
{
    static bool SetFromString(int8* Property, const std::wstring& Value)
    {
        std::wstringstream ss(RemoveUnnecessaryCharsFromString(Value));
        
        int32 Int32 = 0;
        ss >> Int32;
        
        *Property = static_cast<int8>(Int32);
        
        return true;
    }
    
    static std::wstring ToString(const int8* Value)
    {
        int32 Int32 = static_cast<int32>(*Value);
        
        std::wstringstream Stream{};
        Stream << Int32;
        return Stream.str();
    }
    
    static std::wstring GetName() { return L"int8"; }
    static std::vector<FInfoOfStructWithPropertiesUsedInType> GetInfoOfStructsWithPropertiesUsedInType() { return {}; }
};

template<>
struct TTypeHelpers<int16>
{
    static bool SetFromString(int16* Property, const std::wstring& Value)
    {
        std::wstringstream(RemoveUnnecessaryCharsFromString(Value)) >> *Property;
        return true;
    }
    
    static std::wstring ToString(const int16* Value)
    {
        std::wstringstream Stream{};
        Stream << *Value;
        return Stream.str();
    }
    
    static std::wstring GetName() { return L"int16"; }
    static std::vector<FInfoOfStructWithPropertiesUsedInType> GetInfoOfStructsWithPropertiesUsedInType() { return {}; }
};

template<>
struct TTypeHelpers<int32>
{
    static bool SetFromString(int32* Property, const std::wstring& Value)
    {
        std::wstringstream(RemoveUnnecessaryCharsFromString(Value)) >> *Property;
        return true;
    }
    
    static std::wstring ToString(const int32* Value)
    {
        std::wstringstream Stream{};
        Stream << *Value;
        return Stream.str();
    }
    
    static std::wstring GetName() { return L"int32"; }
    static std::vector<FInfoOfStructWithPropertiesUsedInType> GetInfoOfStructsWithPropertiesUsedInType() { return {}; }
};

template<>
struct TTypeHelpers<int64>
{
    static bool SetFromString(int64* Property, const std::wstring& Value)
    {
        std::wstringstream(RemoveUnnecessaryCharsFromString(Value)) >> *Property;
        return true;
    }
    
    static std::wstring ToString(const int64* Value)
    {
        std::wstringstream Stream{};
        Stream << *Value;
        return Stream.str();
    }
    
    static std::wstring GetName() { return L"int64"; }
    static std::vector<FInfoOfStructWithPropertiesUsedInType> GetInfoOfStructsWithPropertiesUsedInType() { return {}; }
};

template<>
struct TTypeHelpers<float>
{
    static bool SetFromString(float* Property, const std::wstring& Value)
    {
        std::wstringstream(RemoveUnnecessaryCharsFromString(Value)) >> *Property;
        return true;
    }
    
    static std::wstring ToString(const float* Value)
    {
        std::wstringstream Stream{};
        Stream << *Value;
        return Stream.str();
    }
    
    static std::wstring GetName() { return L"float"; }
    static std::vector<FInfoOfStructWithPropertiesUsedInType> GetInfoOfStructsWithPropertiesUsedInType() { return {}; }
};

template<>
struct TTypeHelpers<double>
{
    static bool SetFromString(double* Property, const std::wstring& Value)
    {
        std::wstringstream(RemoveUnnecessaryCharsFromString(Value)) >> *Property;
        return true;
    }
    
    static std::wstring ToString(const double* Value)
    {
        std::wstringstream Stream{};
        Stream << *Value;
        return Stream.str();
    }
    
    static std::wstring GetName() { return L"double"; }
    static std::vector<FInfoOfStructWithPropertiesUsedInType> GetInfoOfStructsWithPropertiesUsedInType() { return {}; }
};

template<>
struct TTypeHelpers<wchar_t>
{
    static bool SetFromString(wchar_t* Property, const std::wstring& Value)
    {
        std::wstringstream(Value) >> *Property;
        return true;
    }
    
    static std::wstring ToString(const wchar_t* Value)
    {
        std::wstringstream Stream{};
        Stream << *Value;
        return Stream.str();
    }
    
    static std::wstring GetName() { return L"wchar_t"; }
    static std::vector<FInfoOfStructWithPropertiesUsedInType> GetInfoOfStructsWithPropertiesUsedInType() { return {}; }
};

bool ParsePropertiesSetData(const std::wstring& StringToParse, FDefaultValueOverrides& OutResult);

template<>
struct TTypeHelpers<FDefaultValueOverrides>
{
    static bool SetFromString(FDefaultValueOverrides* Property, const std::wstring& Value)
    {
        return ParsePropertiesSetData(Value, *Property);
    }
    
    static std::wstring ToString(const FDefaultValueOverrides* Value)
    {
        std::wstring Result{};
        
        bool bIsFirst = true;
        
        for (const auto& Override : *Value)
        {
            if (!bIsFirst)
            {
                Result += L" ";
            }
            
            bIsFirst = false;
            
            Result += std::format(L"{}: {{{}}}", Override.PropertyName, Override.SetValue);
        }
        
        return Result;
    }
    
    static std::wstring GetName() { return L"FDefaultValueOverrides"; }
    static std::vector<FInfoOfStructWithPropertiesUsedInType> GetInfoOfStructsWithPropertiesUsedInType() { return {}; }
};

template<typename T>
struct TTypeHelpers<T, std::enable_if_t<std::is_base_of_v<FStructWithProperties, T>>>
{
    static bool SetFromString(FStructWithProperties* StructWithProperties, const std::wstring& Value)
    {
        FDefaultValueOverrides PropertiesSetData;
        if (!TTypeHelpers<FDefaultValueOverrides>::SetFromString(&PropertiesSetData, Value))
        {
            return false;
        }
        
        for (const auto& PropertySetData : PropertiesSetData)
        {
            StructWithProperties->SetPropertyValue(PropertySetData.PropertyName, PropertySetData.SetValue);
        }
        
        return true;
    }
    
    static std::wstring ToString(const FStructWithProperties* StructWithProperties)
    {
        FDefaultValueOverrides DefaultValueOverrides{};
        
        for (const auto Property : StructWithProperties->GetPropertiesArrayConstRef())
        {
            FPropertySetData PropertySetData{};
            PropertySetData.PropertyName = Property->GetName();
            PropertySetData.SetValue = Property->GetAsString(StructWithProperties);
            
            DefaultValueOverrides.emplace_back(std::move(PropertySetData));
        }
        
        return TTypeHelpers<FDefaultValueOverrides>::ToString(&DefaultValueOverrides);
    }
    
    static std::wstring GetName() { return T::GetName(); }
    
    static std::vector<FInfoOfStructWithPropertiesUsedInType> GetInfoOfStructsWithPropertiesUsedInType()
    {
        std::unordered_set<std::wstring> TypeNamesInResult{};
        std::vector<FInfoOfStructWithPropertiesUsedInType> Result{};
        
        FInfoOfStructWithPropertiesUsedInType Elem{};
        Elem.TypeName = GetName();
        Elem.SampleObjectHolder = {new T{}, [](FStructWithProperties* Struct)
        {
            delete static_cast<T*>(Struct);
        }};
        
        TypeNamesInResult.insert(Elem.TypeName);
        
        for (auto Property : Elem.SampleObjectHolder->Properties)
        {
            for (auto& Info : Property->GetInfoOfStructsWithPropertiesUsedInType())
            {
                if (TypeNamesInResult.contains(Info.TypeName))
                {
                    continue;
                }
                
                TypeNamesInResult.insert(Info.TypeName);
                Result.emplace_back(std::move(Info));
            }
        }
        
        Result.emplace_back(std::move(Elem));
        
        return Result;
    }
};

bool ParseCppStringLiteral(const std::wstring_view InData, std::wstring& OutParsedValue);
std::wstring EscapeForCppWideStringLiteral(std::wstring_view s);

template<>
struct TTypeHelpers<std::wstring>
{
    static bool SetFromString(std::wstring* Property, const std::wstring& Value)
    {
        return ParseCppStringLiteral(Value, *Property);
    }
    
    static std::wstring ToString(const std::wstring* Property)
    {
        const std::wstring escaped = EscapeForCppWideStringLiteral(*Property);
        return std::format(L"\"{}\"", escaped);
    }
    
    static std::wstring GetName() { return L"std::wstring"; }
    static std::vector<FInfoOfStructWithPropertiesUsedInType> GetInfoOfStructsWithPropertiesUsedInType() { return {}; }
};

template<>
struct TTypeHelpers<bool>
{
    static bool SetFromString(bool* Property, const std::wstring& Value)
    {
        std::wstring CleanString = RemoveUnnecessaryCharsFromString(Value);
        
        if (CleanString == L"0" || CleanString == L"false" || CleanString == L"FALSE" || CleanString == L"False")
        {
            *Property = false;
        }
        else if (CleanString == L"1" || CleanString == L"true" || CleanString == L"TRUE" || CleanString == L"True")
        {
            *Property = true;
        }
        else
        {
            Log(Error, L"Bool property setter failed.");
            return false;
        }

        return true;
    }
    
    static std::wstring ToString(const bool* Property)
    {
        return *Property ? L"true" : L"false";
    }
    
    static std::wstring GetName() { return L"bool"; }
    static std::vector<FInfoOfStructWithPropertiesUsedInType> GetInfoOfStructsWithPropertiesUsedInType() { return {}; }
};

template<>
struct TTypeHelpers<class NClass*>
{
    static bool SetFromString(NClass** Property, const std::wstring& Value)
    {
        auto CleanString = RemoveUnnecessaryCharsFromString(Value);
        
        if (CleanString.empty() || CleanString == L"nullptr" || CleanString == L"null" || CleanString == L"NULL" || CleanString == L"0")
        {
            *Property = nullptr;
            return true;
        }
        
        *Property = NClass::GetClassByName(CleanString);

        if (!*Property)
        {
            Log(Error, L"NClass property setter failed, did not find class.");
            return false;
        }

        return true;
    }
    
    static std::wstring ToString(NClass* const* Property)
    {
        if (*Property)
        {
            return (*Property)->GetName();
        }
        
        return L"nullptr";
    }
    
    static std::wstring GetName() { return L"NClass*"; }
    static std::vector<FInfoOfStructWithPropertiesUsedInType> GetInfoOfStructsWithPropertiesUsedInType() { return {}; }
};

template<class T>
struct TTypeHelpers<NSubClassOf<T>>
{
    static bool SetFromString(NSubClassOf<T>* Property, const std::wstring& OutValue)
    {
        NClass* Class = nullptr;
        
        if (!TTypeHelpers<NClass*>::SetFromString(&Class, OutValue))
        {
            return false;
        }
        
        *Property = Class;
        
        if (Class && !*Property)
        {
            Log(Error, L"{} setter failed, class is not a subclass of {}.", GetName(), T::StaticClass()->GetName());
            return false;
        }

        return true;
    }
    
    static std::wstring ToString(const NSubClassOf<T>* Property)
    {
        if (*Property)
        {
            return (*Property)->GetName();
        }
        
        return L"nullptr";
    }
    
    static std::wstring GetName()
    {
        return std::format(L"NSubClassOf<{}>", T::StaticClass()->GetName());
    }
    static std::vector<FInfoOfStructWithPropertiesUsedInType> GetInfoOfStructsWithPropertiesUsedInType() { return {}; }
};

bool ParseStringArray(const std::wstring& InData, std::vector<std::wstring>& Result);

template<class T>
struct TTypeHelpers<std::vector<T>>
{
    static bool SetFromString(std::vector<T>* Property, const std::wstring& Value)
    {
        Property->clear();
        
        std::vector<std::wstring> ParsedElements;
        if (!ParseStringArray(Value, ParsedElements))
        {
            return false;
        }
        
        for (const auto& Elem : ParsedElements)
        {
            T ConvertedElem;
            if (!TTypeHelpers<T>::SetFromString(&ConvertedElem, Elem))
            {
                return false;
            }
            
            Property->push_back(std::move(ConvertedElem));
        }
        
        return true;
    }
    
    static std::wstring ToString(const std::vector<T>* Array)
    {
        std::wstring Result{};
        
        bool bIsFirstElem = true;
        
        for (const auto& Elem : *Array)
        {
            if (!bIsFirstElem)
            {
                Result += L" ";
            }
            
            bIsFirstElem = false;
            
            Result += std::format(L"{{{}}}", TTypeHelpers<T>::ToString(&Elem));
        }
        
        return Result;
    }
    
    static std::wstring GetName()
    {
        return std::format(L"std::vector<{}>", TTypeHelpers<T>::GetName());
    }
    
    static std::vector<FInfoOfStructWithPropertiesUsedInType> GetInfoOfStructsWithPropertiesUsedInType()
    {
        return TTypeHelpers<T>::GetInfoOfStructsWithPropertiesUsedInType();
    }
};

bool ConvertStringToCleanAbsolutePath(const std::wstring& Input, std::filesystem::path& OutResult);

template<>
struct TTypeHelpers<std::filesystem::path>
{
    static bool SetFromString(std::filesystem::path* Property, const std::wstring& Value)
    {
        std::wstring ParsedString{};
        if (!TTypeHelpers<std::wstring>::SetFromString(&ParsedString, Value))
        {
            return false;
        }

        return ConvertStringToCleanAbsolutePath(ParsedString, *Property);
    }
    
    static std::wstring ToString(const std::filesystem::path* Property)
    {
        std::wstring String = Property->wstring();
        return TTypeHelpers<std::wstring>::ToString(&String);
    }
    
    static std::wstring GetName()
    {
        return L"std::filesystem::path";
    }
    static std::vector<FInfoOfStructWithPropertiesUsedInType> GetInfoOfStructsWithPropertiesUsedInType() { return {}; }
};