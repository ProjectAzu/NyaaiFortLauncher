#include "PropertySetters.h"

std::string RemoveUnnecessaryCharsFromString(const std::string& Input)
{
    std::string Result = Input;
    std::erase_if(Result, [](char Char)
    {
        return !std::isalpha(Char) && !std::isdigit(Char) && !std::ispunct(Char);
    });
    return Result;
}

bool ParseStringArray(const std::string& InData, std::vector<std::string>& Result)
{
    enum class EParseState : uint8
    {
        SearchingForOpenBrace,
        GatheringElement
    };
    
    EParseState State = EParseState::SearchingForOpenBrace;
    int32_t BraceDepth = 0;
    std::string CurrentElement;

    for (size_t i = 0; i < InData.size(); )
    {
        const char Char = InData[i];

        switch (State)
        {
        case EParseState::SearchingForOpenBrace:
            {
                // Skip any whitespace
                if (std::isspace(static_cast<unsigned char>(Char)))
                {
                    ++i;
                }
                else if (Char == '{')
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
                    Log(Error, "Expected '{{' at position {}, but found '{}'.", i, Char);
                    return false;
                }
            }
            break;

        case EParseState::GatheringElement:
            {
                if (Char == '{')
                {
                    // Nested brace - increase depth
                    ++BraceDepth;
                    CurrentElement.push_back(Char);
                    ++i;
                }
                else if (Char == '}')
                {
                    --BraceDepth;
                    if (BraceDepth < 0)
                    {
                        // More closing braces than opening
                        Log(Error, "Mismatched braces at position {}.", i);
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
        Log(Error, "Unexpected end of input while parsing element: missing '}}'.");
        return false;
    }
    
    return true;
}

bool ParsePropertiesSetData(const std::string& StringToParse, std::vector<FPropertySetData>& OutResult)
{
    OutResult.clear();

    const auto SkipWhitespace = [&](size_t& Index)
    {
        while (Index < StringToParse.size() && std::isspace(static_cast<unsigned char>(StringToParse[Index])))
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
        size_t ColonPos  = std::string::npos;

        while (Index < StringToParse.size())
        {
            if (StringToParse[Index] == ':')
            {
                ColonPos = Index;
                break;
            }
            ++Index;
        }

        if (ColonPos == std::string::npos)
        {
            // No colon found => invalid syntax
            Log(Error, "Expected ':' but none was found. Parsing aborted.");
            return false;
        }

        // Extract the raw property name
        std::string RawPropertyName = StringToParse.substr(NameStart, ColonPos - NameStart);
        // Clean it up
        RawPropertyName = RemoveUnnecessaryCharsFromString(RawPropertyName);

        // Move past the colon
        ++Index; 

        // 3) Skip whitespace between ':' and '{'
        SkipWhitespace(Index);

        // 4) Expect an opening brace
        if (Index >= StringToParse.size() || StringToParse[Index] != '{')
        {
            Log(Error, "Expected '{{' after property name '{}'.", RawPropertyName);
            return false;
        }

        // Start capturing value text
        ++Index; // consume the '{'

        // We'll parse the value by tracking brace depth
        int BraceDepth = 1;
        std::string CapturedValue;
        CapturedValue.reserve(128); // just a small reserve to reduce re-allocs

        while (Index < StringToParse.size() && BraceDepth > 0)
        {
            char C = StringToParse[Index];

            if (C == '{')
            {
                ++BraceDepth;
                CapturedValue.push_back(C);
                ++Index;
            }
            else if (C == '}')
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
            Log(Error, "Missing '}}' for property '{}'.", RawPropertyName);
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

bool ParseCppStringLiteral(const std::string_view InData, std::string& OutParsedValue)
{
    OutParsedValue.clear();

    // 1. Find the first double quote
    const size_t StartPos = InData.find('\"');
    if (StartPos == std::string_view::npos)
    {
        Log(Error, "No opening '\"' found in input.");
        return false;
    }

    // We will parse from the character immediately after the opening quote
    size_t CurrentIndex = StartPos + 1;

    while (CurrentIndex < InData.size())
    {
        const char CurrentChar = InData[CurrentIndex];

        // 2. Check if this is the closing quote
        if (CurrentChar == '\"')
        {
            // Found the matching quote; done
            // We do not consume this quote in the output
            ++CurrentIndex;
            return true; // successfully parsed
        }

        // 3. If it's an escape character
        if (CurrentChar == '\\')
        {
            // Look ahead to see what is escaped
            if (CurrentIndex + 1 >= InData.size())
            {
                // No next character => malformed escape at end
                Log(Error, "Unexpected end of input after '\\'.");
                return false;
            }

            const char NextChar = InData[CurrentIndex + 1];
            switch (NextChar)
            {
            case '\\':
                OutParsedValue.push_back('\\');
                break;
            case '\"':
                OutParsedValue.push_back('\"');
                break;
            case 'n':
                OutParsedValue.push_back('\n');
                break;
            case 'r':
                OutParsedValue.push_back('\r');
                break;
            case 't':
                OutParsedValue.push_back('\t');
                break;
            case 'b':
                OutParsedValue.push_back('\b');
                break;
            case 'f':
                OutParsedValue.push_back('\f');
                break;
            case 'v':
                OutParsedValue.push_back('\v');
                break;
            default:
                // Unknown escape => we can choose to log a warning or handle it
                // For now, let's just add the character as-is:
                Log(Warning, "Unknown escape sequence '\\{}'. Storing as literal.", NextChar);
                OutParsedValue.push_back(NextChar);
                break;
            }
            CurrentIndex += 2; // Skip the backslash and the escaped character
        }
        else
        {
            // Normal character
            OutParsedValue.push_back(CurrentChar);
            ++CurrentIndex;
        }
    }

    // We reached the end of the string without finding a closing quote
    Log(Error, "No matching closing '\"' found for string literal starting at position {}.", StartPos);
    return false;
}

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <filesystem>
#include <string>
#include <stdexcept>

bool ConvertStringToCanonicalPath(const std::string& Input, std::filesystem::path& OutResult)
{
    std::wstring wideInput(Input.begin(), Input.end());

    wchar_t buffer[MAX_PATH];

    DWORD length = SearchPathW(
        nullptr,
        wideInput.c_str(),
        nullptr,
        static_cast<DWORD>(std::size(buffer)),
        buffer,
        nullptr
    );

    if (length == 0 || length > MAX_PATH)
    {
        // Not found in PATH
        Log(Error, 
            "Unable to locate file '{}' as a direct path or in the system PATH.", Input
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
        // Very unlikely to fail here, but just in case:
        Log(Error, 
            "Found file in PATH but failed to canonicalize. Reason: {}", std::string(e.what())
        );

        return false;
    }

    return true;
}