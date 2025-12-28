#include "PropertySetters.h"

#include <cwctype>

std::wstring RemoveUnnecessaryCharsFromString(const std::wstring& Input)
{
    std::wstring Result = Input;
    std::erase_if(Result, [](wchar_t Char)
    {
        return !std::iswalpha(Char) && !std::iswdigit(Char) && !std::iswpunct(Char);
    });
    return Result;
}

bool ParseStringArray(const std::wstring& InData, std::vector<std::wstring>& Result)
{
    enum class EParseState : uint8
    {
        SearchingForOpenBrace,
        GatheringElement
    };
    
    EParseState State = EParseState::SearchingForOpenBrace;
    int32_t BraceDepth = 0;
    std::wstring CurrentElement;

    for (size_t i = 0; i < InData.size(); )
    {
        const wchar_t Char = InData[i];

        switch (State)
        {
        case EParseState::SearchingForOpenBrace:
            {
                // Skip any whitespace
                if (std::iswspace(Char))
                {
                    ++i;
                }
                else if (Char == L'{')
                {
                    // Found the start of an element
                    BraceDepth = 1;
                    CurrentElement.clear();
                    State = EParseState::GatheringElement;
                    ++i;
                }
                else
                {
                    // Unexpected character while looking for '{'
                    Log(Error, L"ArrayElementsParser: Expected '{{' at position {}, but found '{}'.", i, Char);
                    return false;
                }
            }
            break;

        case EParseState::GatheringElement:
            {
                if (Char == L'{')
                {
                    // Nested brace - increase depth
                    ++BraceDepth;
                    CurrentElement.push_back(Char);
                    ++i;
                }
                else if (Char == L'}')
                {
                    --BraceDepth;
                    if (BraceDepth < 0)
                    {
                        // More closing braces than opening
                        Log(Error, L"ArrayElementsParser: Mismatched braces at position {}.", i);
                        return false;
                    }

                    if (BraceDepth == 0)
                    {
                        // This '}' closes the element
                        Result.push_back(CurrentElement);
                        CurrentElement.clear();
                        State = EParseState::SearchingForOpenBrace;
                    }
                    else
                    {
                        // Still inside nested braces
                        CurrentElement.push_back(Char);
                    }
                    ++i;
                }
                else
                {
                    // Normal character, part of the element
                    CurrentElement.push_back(Char);
                    ++i;
                }
            }
            break;
        }
    }

    // If the parser ended while still gathering an element,
    // it implies we're missing a closing brace.
    if (State == EParseState::GatheringElement)
    {
        Log(Error, L"ArrayElementsParser: Unexpected end of input while parsing element: missing '}}'.");
        return false;
    }
    
    return true;
}

bool ParsePropertiesSetData(const std::wstring& StringToParse, std::vector<FPropertySetData>& OutResult)
{
    OutResult.clear();

    const auto SkipWhitespace = [&](size_t& Index)
    {
        while (Index < StringToParse.size() && std::iswspace(StringToParse[Index]))
        {
            ++Index;
        }
    };

    size_t Index = 0;

    // Main parsing loop
    while (true)
    {
        // 1) Skip leading whitespace (if any)
        SkipWhitespace(Index);

        // If we've reached the end of the string, we're done
        if (Index >= StringToParse.size())
        {
            break;
        }

        // 2) Parse the property name (until we find a ':')
        //    We'll collect everything until the next colon or end-of-string.
        size_t NameStart = Index;
        size_t ColonPos  = std::wstring::npos;

        while (Index < StringToParse.size())
        {
            if (StringToParse[Index] == L':')
            {
                ColonPos = Index;
                break;
            }
            ++Index;
        }

        if (ColonPos == std::wstring::npos)
        {
            // No colon found => invalid syntax
            Log(Error, L"Expected ':' but none was found. Parsing aborted.");
            return false;
        }

        // Extract the raw property name
        std::wstring RawPropertyName = StringToParse.substr(NameStart, ColonPos - NameStart);
        // Clean it up
        RawPropertyName = RemoveUnnecessaryCharsFromString(RawPropertyName);

        // Move past the colon
        ++Index; 

        // 3) Skip whitespace between ':' and '{'
        SkipWhitespace(Index);

        // 4) Expect an opening brace
        if (Index >= StringToParse.size() || StringToParse[Index] != L'{')
        {
            Log(Error, L"Expected '{{' after property name '{}'.", RawPropertyName);
            return false;
        }

        // Start capturing value text
        ++Index; // consume the '{'

        // We'll parse the value by tracking brace depth
        int BraceDepth = 1;
        std::wstring CapturedValue;
        CapturedValue.reserve(128); // just a small reserve to reduce re-allocs

        while (Index < StringToParse.size() && BraceDepth > 0)
        {
            wchar_t C = StringToParse[Index];

            if (C == L'{')
            {
                ++BraceDepth;
                CapturedValue.push_back(C);
                ++Index;
            }
            else if (C == L'}')
            {
                --BraceDepth;
                if (BraceDepth > 0)
                {
                    // still inside nested braces
                    CapturedValue.push_back(C);
                }
                ++Index;
            }
            else
            {
                // Normal character
                CapturedValue.push_back(C);
                ++Index;
            }
        }

        if (BraceDepth != 0)
        {
            // We never closed all braces
            Log(Error, L"Missing '}}' for property '{}'.", RawPropertyName);
            return false;
        }

        // We have RawPropertyName and CapturedValue
        FPropertySetData NewData;
        NewData.PropertyName = RawPropertyName;
        NewData.SetValue     = CapturedValue;
        OutResult.push_back(NewData);
    }

    // Successfully parsed all key-value pairs
    return true;
}

bool ParseCppStringLiteral(const std::wstring_view InData, std::wstring& OutParsedValue)
{
    OutParsedValue.clear();

    auto SkipWhitespace = [&](size_t& i)
    {
        while (i < InData.size() && std::iswspace(InData[i]))
            ++i;
    };

    // Find the first quote
    size_t i = InData.find(L'\"');
    if (i == std::wstring_view::npos)
    {
        Log(Error, L"String parser: No opening '\"' found in input.");
        return false;
    }

    // Parse one or more adjacent string literals and concatenate them.
    while (true)
    {
        SkipWhitespace(i);

        if (i >= InData.size() || InData[i] != L'\"')
        {
            // We parsed at least one literal (first one is required); stop joining.
            return !OutParsedValue.empty();
        }

        // Consume opening quote
        ++i;

        bool escape = false;
        while (i < InData.size())
        {
            wchar_t c = InData[i];

            if (escape)
            {
                // Handle escapes after backslash
                switch (c)
                {
                case L'\\': OutParsedValue.push_back(L'\\'); break;
                case L'\"': OutParsedValue.push_back(L'\"'); break;
                case L'n':  OutParsedValue.push_back(L'\n'); break;
                case L'r':  OutParsedValue.push_back(L'\r'); break;
                case L't':  OutParsedValue.push_back(L'\t'); break;
                case L'b':  OutParsedValue.push_back(L'\b'); break;
                case L'f':  OutParsedValue.push_back(L'\f'); break;
                case L'v':  OutParsedValue.push_back(L'\v'); break;
                default:
                    Log(Warning, L"String parser: Unknown escape sequence '\\{}'. Storing as literal.", c);
                    OutParsedValue.push_back(c);
                    break;
                }

                escape = false;
                ++i;
                continue;
            }

            if (c == L'\\')
            {
                escape = true;
                ++i;
                continue;
            }

            if (c == L'\"')
            {
                // End of this literal
                ++i;
                break;
            }

            OutParsedValue.push_back(c);
            ++i;
        }

        if (escape)
        {
            Log(Error, L"String parser: Unexpected end of input after '\\'.");
            return false;
        }

        if (i > InData.size())
        {
            // defensive; normally unreachable
            Log(Error, L"String parser: Unexpected parsing state.");
            return false;
        }

        // If we ended because we ran out of input without seeing a closing quote:
        if (i == InData.size() && (InData.size() == 0 || InData[InData.size() - 1] != L'\"'))
        {
            Log(Error, L"String parser: No matching closing '\"' found for string literal.");
            return false;
        }

        // Loop again: if next token is another quote (after whitespace), we'll append it.
        // Otherwise we return at top of loop.
    }
}

#include "Utils/WindowsInclude.h"
#include <filesystem>
#include <string>
#include <stdexcept>

bool ConvertStringToCanonicalPath(const std::wstring& Input, std::filesystem::path& OutResult)
{
    wchar_t buffer[MAX_PATH];

    DWORD length = SearchPathW(
        nullptr,
        Input.c_str(),
        nullptr,
        static_cast<DWORD>(std::size(buffer)),
        buffer,
        nullptr
    );

    if (length == 0 || length > MAX_PATH)
    {
        // Not found in PATH
        Log(Error, 
            L"Unable to locate file '{}' as a direct path or in the system PATH.", Input
        );

        return false;
    }
    
    try
    {
        std::filesystem::path foundPath(buffer);
        OutResult = std::filesystem::canonical(foundPath);
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        auto What = std::string(e.what());
        
        // Very unlikely to fail here, but just in case:
        Log(Error, 
            L"Found file in PATH but failed to canonicalize. Reason: {}", std::wstring(What.begin(), What.end())
        );

        return false;
    }

    return true;
}