export module TypeHelpers;

export import Core;

export struct FPropertySetData
{
    std::wstring PropertyName{};
    std::wstring SetValue{};
};

export typedef std::vector<FPropertySetData> FDefaultValueOverrides;

// leaves only alphabetic, digits and punctuation
export std::wstring RemoveUnnecessaryCharsFromString(const std::wstring& Input);

export template <typename T, typename Enable = void>
struct TTypeHelpers
{
    // SetFromString and ToString should be compatible
    static bool SetFromString(T* Property, const std::wstring& Value)
    {
        Log(Warning, L"Using default TTypeHelpers, please add TTypeHelpers for this type");
        std::wstringstream(RemoveUnnecessaryCharsFromString(Value)) >> *Property;

        return true;
    }

    // SetFromString and ToString should be compatible
    static std::wstring ToString(const T* Value)
    {
        Log(Warning, L"Using default TTypeHelpers, please add TTypeHelpers for this type");

        std::wstringstream Stream{};
        Stream << *Value;

        return Stream.str();
    }

    static std::wstring GetName()
    {
        Log(Warning, L"Using default TTypeHelpers, please add TTypeHelpers for this type");

        return L"Unknown";
    }
};

template <>
struct TTypeHelpers<uint8>
{
    static bool SetFromString(uint8* Property, const std::wstring& Value)
    {
        std::wstringstream ss(RemoveUnnecessaryCharsFromString(Value));

        uint32 Uint32 = 0;
        ss >> Uint32;

        *Property = scast<uint8>(Uint32);

        return true;
    }

    static std::wstring ToString(const uint8* Value)
    {
        uint32 Uint32 = scast<uint32>(*Value);

        std::wstringstream Stream{};
        Stream << Uint32;

        return Stream.str();
    }

    static std::wstring GetName() { return L"uint8"; }
};

template <>
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
};

template <>
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
};

template <>
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
};

template <>
struct TTypeHelpers<int8>
{
    static bool SetFromString(int8* Property, const std::wstring& Value)
    {
        std::wstringstream ss(RemoveUnnecessaryCharsFromString(Value));

        int32 Int32 = 0;
        ss >> Int32;

        *Property = scast<int8>(Int32);

        return true;
    }

    static std::wstring ToString(const int8* Value)
    {
        int32 Int32 = scast<int32>(*Value);

        std::wstringstream Stream{};
        Stream << Int32;

        return Stream.str();
    }

    static std::wstring GetName() { return L"int8"; }
};

template <>
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
};

template <>
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
};

template <>
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
};

template <>
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
};

template <>
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
};

template <>
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
};

export bool ParsePropertiesSetData(const std::wstring& StringToParse, FDefaultValueOverrides& OutResult);

template <>
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
};

export bool ParseCppStringLiteral(const std::wstring_view InData, std::wstring& OutParsedValue);
export std::wstring EscapeForCppWideStringLiteral(std::wstring_view s);

template <>
struct TTypeHelpers<std::wstring>
{
    static bool SetFromString(std::wstring* Property, const std::wstring& Value)
    {
        if (Value.empty())
        {
            *Property = {};

            return true;
        }

        return ParseCppStringLiteral(Value, *Property);
    }

    static std::wstring ToString(const std::wstring* Property)
    {
        if (Property->empty())
        {
            return {};
        }

        const std::wstring escaped = EscapeForCppWideStringLiteral(*Property);

        return std::format(L"\"{}\"", escaped);
    }

    static std::wstring GetName() { return L"std::wstring"; }
};

template <>
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
            Log(Error, L"Bool property setter failed");

            return false;
        }

        return true;
    }

    static std::wstring ToString(const bool* Property)
    {
        return *Property ? L"true" : L"false";
    }

    static std::wstring GetName() { return L"bool"; }
};

export bool ParseStringArray(const std::wstring& InData, std::vector<std::wstring>& Result);

template <class T>
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
};

template <>
struct TTypeHelpers<std::filesystem::path>
{
    static bool SetFromString(std::filesystem::path* Property, const std::wstring& Value)
    {
        std::wstring ParsedString{};

        if (!TTypeHelpers<std::wstring>::SetFromString(&ParsedString, Value))
        {
            return false;
        }

        std::wstring StringWithoutWhitespaces = ParsedString;
        std::erase_if(StringWithoutWhitespaces, [](wchar_t Char)
        {
            return std::iswspace(Char) != 0;
        });

        if (StringWithoutWhitespaces.empty())
        {
            *Property = std::filesystem::path{};

            return true;
        }

        *Property = ParsedString;

        return true;
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
};
