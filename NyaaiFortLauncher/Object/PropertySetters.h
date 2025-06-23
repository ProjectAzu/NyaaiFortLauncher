#pragma once

#include <string>
#include <sstream>
#include <utility>
#include <filesystem>

#include "Object.h"

// leaves only alphabetic, digits and punctuation
std::string RemoveUnnecessaryCharsFromString(const std::string& Input);

template <typename  T, typename Enable = void>
struct FPropertySetterFunction
{
    static bool Set(T* Property, const std::string& Value)
    {
        std::stringstream(RemoveUnnecessaryCharsFromString(Value)) >> *Property;
        return true;
    }
};

bool ParsePropertiesSetData(const std::string& StringToParse, std::vector<FPropertySetData>& OutResult);

template<>
struct FPropertySetterFunction<std::vector<FPropertySetData>>
{
    static bool Set(std::vector<FPropertySetData>* Property, const std::string& Value)
    {
        return ParsePropertiesSetData(Value, *Property);
    }
};


template<typename T>
struct FPropertySetterFunction<T, std::enable_if_t<std::is_base_of_v<FStructWithProperties, T>>>
{
    static bool Set(FStructWithProperties* Property, const std::string& Value)
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

bool ParseCppStringLiteral(const std::string_view InData, std::string& OutParsedValue);

template<>
struct FPropertySetterFunction<std::string>
{
    static bool Set(std::string* Property, const std::string& Value)
    {
        return ParseCppStringLiteral(Value, *Property);
    }
};

template<>
struct FPropertySetterFunction<bool>
{
    static bool Set(bool* Property, const std::string& Value)
    {
        std::string GoodString = RemoveUnnecessaryCharsFromString(Value);
        
        if (GoodString == "0")
            *Property = false;
        else if (GoodString == "1")
            *Property = true;
        else if (GoodString == "false")
            *Property = false;
        else if (GoodString == "true")
            *Property = true;
        else
        {
            Log(Error, "Bool property setter failed.");
            return false;
        }

        return true;
    }
};

template<>
struct FPropertySetterFunction<class NClass*>
{
    static bool Set(NClass** Property, const std::string& Value)
    {
        auto GoodString = RemoveUnnecessaryCharsFromString(Value);
        
        if (GoodString.empty() || GoodString == "nullptr" || GoodString == "null" || GoodString == "NULL" || GoodString == "0")
        {
            *Property = nullptr;
            return true;
        }
        
        *Property = NClass::GetClassByName(GoodString);

        if (!*Property)
        {
            Log(Error, "NClass property setter failed, did not find class.");
            return false;
        }

        return true;
    }
};

template<class T>
struct FPropertySetterFunction<NSubClassOf<T>>
{
    static bool Set(NSubClassOf<T>* Property, const std::string& OutValue)
    {
        NClass* Class = nullptr;
        
        if (!FPropertySetterFunction<NClass*>::Set(&Class, OutValue))
        {
            return false;
        }
        
        *Property = Class;
        
        if (Class && !*Property)
        {
            Log(Error, "NSubClassOf<T> setter failed, class is not a subclass of T.");
            return false;
        }

        return true;
    }
};

bool ParseStringArray(const std::string& InData, std::vector<std::string>& Result);

template<class T>
struct FPropertySetterFunction<std::vector<T>>
{
    static bool Set(std::vector<T>* Property, const std::string& Value)
    {
        Property->clear();
        
        std::vector<std::string> ParsedElements;
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

bool ConvertStringToCanonicalPath(const std::string& Input, std::filesystem::path& OutResult);

template<>
struct FPropertySetterFunction<std::filesystem::path>
{
    static bool Set(std::filesystem::path* Property, const std::string& Value)
    {
        std::string ParsedString{};
        if (!FPropertySetterFunction<std::string>::Set(&ParsedString, Value))
        {
            return false;
        }

        return ConvertStringToCanonicalPath(ParsedString, *Property);
    }
};