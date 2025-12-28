#pragma once

#include <string>
#include <sstream>
#include <utility>
#include <filesystem>

#include "Object.h"
#include "PropertySetData.h"

// leaves only alphabetic, digits and punctuation
std::wstring RemoveUnnecessaryCharsFromString(const std::wstring& Input);

template <typename  T, typename Enable = void>
struct FPropertySetterFunction
{
    static bool Set(T* Property, const std::wstring& Value)
    {
        std::wstringstream(RemoveUnnecessaryCharsFromString(Value)) >> *Property;
        return true;
    }
};

bool ParsePropertiesSetData(const std::wstring& StringToParse, std::vector<FPropertySetData>& OutResult);

template<>
struct FPropertySetterFunction<std::vector<FPropertySetData>>
{
    static bool Set(std::vector<FPropertySetData>* Property, const std::wstring& Value)
    {
        return ParsePropertiesSetData(Value, *Property);
    }
};


template<typename T>
struct FPropertySetterFunction<T, std::enable_if_t<std::is_base_of_v<FStructWithProperties, T>>>
{
    static bool Set(FStructWithProperties* Property, const std::wstring& Value)
    {
        std::vector<FPropertySetData> PropertiesSetData;
        if (!FPropertySetterFunction<std::vector<FPropertySetData>>::Set(&PropertiesSetData, Value))
        {
            return false;
        }
        
        for (const auto& PropertySetData : PropertiesSetData)
        {
            Property->SetPropertyValue(PropertySetData.PropertyName, PropertySetData.SetValue);
        }
        
        return true;
    }
};

bool ParseCppStringLiteral(const std::wstring_view InData, std::wstring& OutParsedValue);

template<>
struct FPropertySetterFunction<std::wstring>
{
    static bool Set(std::wstring* Property, const std::wstring& Value)
    {
        return ParseCppStringLiteral(Value, *Property);
    }
};

template<>
struct FPropertySetterFunction<bool>
{
    static bool Set(bool* Property, const std::wstring& Value)
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
};

template<>
struct FPropertySetterFunction<class NClass*>
{
    static bool Set(NClass** Property, const std::wstring& Value)
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
};

template<class T>
struct FPropertySetterFunction<NSubClassOf<T>>
{
    static bool Set(NSubClassOf<T>* Property, const std::wstring& OutValue)
    {
        NClass* Class = nullptr;
        
        if (!FPropertySetterFunction<NClass*>::Set(&Class, OutValue))
        {
            return false;
        }
        
        *Property = Class;
        
        if (Class && !*Property)
        {
            Log(Error, L"NSubClassOf<T> setter failed, class is not a subclass of T.");
            return false;
        }

        return true;
    }
};

bool ParseStringArray(const std::wstring& InData, std::vector<std::wstring>& Result);

template<class T>
struct FPropertySetterFunction<std::vector<T>>
{
    static bool Set(std::vector<T>* Property, const std::wstring& Value)
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
            FPropertySetterFunction<T>::Set(&ConvertedElem, Elem);
            
            Property->push_back(std::move(ConvertedElem));
        }
        
        return true;
    }
};

bool ConvertStringToCanonicalPath(const std::wstring& Input, std::filesystem::path& OutResult);

template<>
struct FPropertySetterFunction<std::filesystem::path>
{
    static bool Set(std::filesystem::path* Property, const std::wstring& Value)
    {
        std::wstring ParsedString{};
        if (!FPropertySetterFunction<std::wstring>::Set(&ParsedString, Value))
        {
            return false;
        }

        return ConvertStringToCanonicalPath(ParsedString, *Property);
    }
};