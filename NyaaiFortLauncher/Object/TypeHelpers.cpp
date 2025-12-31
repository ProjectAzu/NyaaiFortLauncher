#include "TypeHelpers.h"

#include <cwctype>

static bool IsHex(wchar_t c)
{
    return (c >= L'0' && c <= L'9') ||
           (c >= L'a' && c <= L'f') ||
           (c >= L'A' && c <= L'F');
}

static int32 HexVal(wchar_t c)
{
    if (c >= L'0' && c <= L'9') return int32(c - L'0');
    if (c >= L'a' && c <= L'f') return 10 + int32(c - L'a');
    return 10 + int32(c - L'A');
}

static bool IsOct(wchar_t c) { return c >= L'0' && c <= L'7'; }

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
    Result.clear();

    enum class EParseState : uint8
    {
        SearchingForOpenBrace,
        GatheringElement
    };

    EParseState State = EParseState::SearchingForOpenBrace;
    int32_t BraceDepth = 0;
    std::wstring CurrentElement;

    // Track whether we're inside a normal C++ "..." string literal (within an element)
    bool bInString = false;
    bool bEscape   = false;

    for (size_t i = 0; i < InData.size(); )
    {
        const wchar_t Char = InData[i];

        switch (State)
        {
        case EParseState::SearchingForOpenBrace:
        {
            bInString = false;
            bEscape   = false;

            if (std::iswspace(static_cast<wint_t>(Char)))
            {
                ++i;
            }
            else if (Char == L'{')
            {
                BraceDepth = 1;
                CurrentElement.clear();
                State = EParseState::GatheringElement;
                ++i; // consume '{'
            }
            else
            {
                Log(Error, L"ArrayElementsParser: Expected '{{' at position {}, but found '{}'.", i, Char);
                return false;
            }
        }
        break;

        case EParseState::GatheringElement:
        {
            if (bInString)
            {
                // Inside "..."
                CurrentElement.push_back(Char);

                if (bEscape)
                {
                    bEscape = false;
                }
                else if (Char == L'\\')
                {
                    bEscape = true;
                }
                else if (Char == L'"')
                {
                    bInString = false;
                }

                ++i;
                break;
            }

            // Not inside string
            if (Char == L'"')
            {
                bInString = true;
                bEscape   = false;
                CurrentElement.push_back(Char);
                ++i;
                break;
            }

            if (Char == L'{')
            {
                ++BraceDepth;
                CurrentElement.push_back(Char);
                ++i;
                break;
            }

            if (Char == L'}')
            {
                --BraceDepth;

                if (BraceDepth < 0)
                {
                    Log(Error, L"ArrayElementsParser: Mismatched braces at position {}.", i);
                    return false;
                }

                if (BraceDepth == 0)
                {
                    // Element closed
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
                break;
            }

            // Normal character
            CurrentElement.push_back(Char);
            ++i;
        }
        break;
        }
    }

    if (State == EParseState::GatheringElement)
    {
        if (bInString)
            Log(Error, L"ArrayElementsParser: Unexpected end of input: missing '\"' in string literal.");
        else if (bEscape)
            Log(Error, L"ArrayElementsParser: Unexpected end of input after '\\' in string literal.");
        else
            Log(Error, L"ArrayElementsParser: Unexpected end of input while parsing element: missing '}}'.");

        return false;
    }

    return true;
}

bool ParsePropertiesSetData(const std::wstring& StringToParse, FDefaultValueOverrides& OutResult)
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
        CapturedValue.reserve(128);

        // Track whether we are inside a normal "..." string literal
        bool bInString = false;
        bool bEscape   = false;

        while (Index < StringToParse.size() && BraceDepth > 0)
        {
            wchar_t C = StringToParse[Index];

            if (bInString)
            {
                // Inside "..."
                CapturedValue.push_back(C);

                if (bEscape)
                {
                    bEscape = false; // whatever it was, it's escaped
                }
                else if (C == L'\\')
                {
                    bEscape = true;  // next char is escaped
                }
                else if (C == L'"')
                {
                    bInString = false; // end of string literal
                }

                ++Index;
                continue;
            }

            // Not in string literal
            if (C == L'"')
            {
                bInString = true;
                bEscape   = false;
                CapturedValue.push_back(C);
                ++Index;
                continue;
            }

            if (C == L'{')
            {
                ++BraceDepth;
                CapturedValue.push_back(C);
                ++Index;
                continue;
            }

            if (C == L'}')
            {
                --BraceDepth;
                if (BraceDepth > 0)
                {
                    CapturedValue.push_back(C); // keep nested closing braces
                }
                ++Index;
                continue;
            }

            // Normal character
            CapturedValue.push_back(C);
            ++Index;
        }

        if (bInString)
        {
            Log(Error, L"Missing closing '\"' inside value for property '{}'.", RawPropertyName);
            return false;
        }

        if (bEscape)
        {
            Log(Error, L"Unexpected end of value after '\\' inside string for property '{}'.", RawPropertyName);
            return false;
        }

        if (BraceDepth != 0)
        {
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
        while (i < InData.size() && std::iswspace(static_cast<wint_t>(InData[i])))
            ++i;
    };

    // Find the first quote
    size_t i = InData.find(L'"');
    if (i == std::wstring_view::npos)
    {
        Log(Error, L"String parser: No opening '\"' found in input.");
        return false;
    }

    // Parse one or more adjacent string literals and concatenate them.
    while (true)
    {
        SkipWhitespace(i);

        if (i >= InData.size() || InData[i] != L'"')
        {
            // Stop joining. Succeed only if we parsed at least one literal.
            return !OutParsedValue.empty() || (InData.find(L"\"\"") != std::wstring_view::npos);
        }

        ++i; // consume opening quote

        while (i < InData.size())
        {
            wchar_t c = InData[i];

            if (c == L'"')
            {
                ++i; // consume closing quote
                break;
            }

            if (c != L'\\')
            {
                OutParsedValue.push_back(c);
                ++i;
                continue;
            }

            // Backslash escape
            ++i;
            if (i >= InData.size())
            {
                Log(Error, L"String parser: Unexpected end of input after '\\'.");
                return false;
            }

            wchar_t e = InData[i];

            switch (e)
            {
            case L'\\': OutParsedValue.push_back(L'\\'); ++i; break;
            case L'"':  OutParsedValue.push_back(L'"');  ++i; break;
            case L'n':  OutParsedValue.push_back(L'\n'); ++i; break;
            case L'r':  OutParsedValue.push_back(L'\r'); ++i; break;
            case L't':  OutParsedValue.push_back(L'\t'); ++i; break;
            case L'b':  OutParsedValue.push_back(L'\b'); ++i; break;
            case L'f':  OutParsedValue.push_back(L'\f'); ++i; break;
            case L'v':  OutParsedValue.push_back(L'\v'); ++i; break;
            case L'a':  OutParsedValue.push_back(L'\a'); ++i; break;
            case L'0':  // could be just \0 or start of octal
            default:
                if (IsOct(e))
                {
                    // Octal escape: up to 3 octal digits (already have 1)
                    unsigned int val = 0;
                    int count = 0;
                    while (i < InData.size() && count < 3 && IsOct(InData[i]))
                    {
                        val = (val * 8u) + unsigned(InData[i] - L'0');
                        ++i;
                        ++count;
                    }
                    OutParsedValue.push_back(static_cast<wchar_t>(val));
                }
                else if (e == L'x')
                {
                    // Hex escape: \x + 1+ hex digits
                    ++i; // consume 'x'
                    if (i >= InData.size() || !IsHex(InData[i]))
                    {
                        Log(Error, L"String parser: Invalid hex escape '\\x' (no digits).");
                        return false;
                    }
                    unsigned int val = 0;
                    while (i < InData.size() && IsHex(InData[i]))
                    {
                        val = (val * 16u) + unsigned(HexVal(InData[i]));
                        ++i;
                    }
                    OutParsedValue.push_back(static_cast<wchar_t>(val));
                }
                else if (e == L'u' || e == L'U')
                {
                    const int digits = (e == L'u') ? 4 : 8;
                    ++i; // consume 'u'/'U'
                    if (i + digits > InData.size())
                    {
                        Log(Error, L"String parser: Incomplete universal character name.");
                        return false;
                    }
                    unsigned int val = 0;
                    for (int k = 0; k < digits; ++k)
                    {
                        wchar_t h = InData[i + k];
                        if (!IsHex(h))
                        {
                            Log(Error, L"String parser: Invalid universal character name (non-hex digit).");
                            return false;
                        }
                        val = (val * 16u) + unsigned(HexVal(h));
                    }
                    i += digits;

                    // Store as wchar_t (Windows wchar_t is 16-bit UTF-16 code unit).
                    // If val > 0xFFFF, you may want to emit surrogate pairs instead.
                    if (val <= 0xFFFFu)
                    {
                        OutParsedValue.push_back(static_cast<wchar_t>(val));
                    }
                    else
                    {
                        // Surrogate pair for UTF-16
                        val -= 0x10000u;
                        wchar_t high = static_cast<wchar_t>(0xD800u + (val >> 10));
                        wchar_t low  = static_cast<wchar_t>(0xDC00u + (val & 0x3FFu));
                        OutParsedValue.push_back(high);
                        OutParsedValue.push_back(low);
                    }
                }
                else
                {
                    Log(Warning, L"String parser: Unknown escape sequence '\\{}'. Storing as literal.", e);
                    OutParsedValue.push_back(e);
                    ++i;
                }
                break;
            }
        }

        // If we ran out of input without a closing quote, error.
        if (i > InData.size())
        {
            Log(Error, L"String parser: Unexpected parsing state.");
            return false;
        }
        if (i == InData.size() && (InData.empty() || InData.back() != L'"'))
        {
            Log(Error, L"String parser: No matching closing '\"' found for string literal.");
            return false;
        }

        // Loop again: if next token is another quote (after whitespace), we'll append it.
    }
}

std::wstring EscapeForCppWideStringLiteral(std::wstring_view s)
{
    std::wstring out;
    out.reserve(s.size() + 2);

    for (wchar_t ch : s)
    {
        switch (ch)
        {
        case L'\\': out += L"\\\\"; break;
        case L'"':  out += L"\\\""; break;
        case L'\n': out += L"\\n";  break;
        case L'\r': out += L"\\r";  break;
        case L'\t': out += L"\\t";  break;
        case L'\b': out += L"\\b";  break;
        case L'\f': out += L"\\f";  break;
        case L'\v': out += L"\\v";  break;
        case L'\a': out += L"\\a";  break;
        case L'\0': out += L"\\0";  break;
        default:
            {
                // Emit non-printables as UCNs so parsing is unambiguous and portable.
                if (!std::iswprint(static_cast<wint_t>(ch)))
                {
                    // Use \uXXXX when possible, else \UXXXXXXXX.
                    unsigned int u = static_cast<unsigned int>(ch);
                    if (u <= 0xFFFFu)
                        out += std::format(L"\\u{:04X}", u);
                    else
                        out += std::format(L"\\U{:08X}", u);
                }
                else
                {
                    out.push_back(ch);
                }
                break;
            }
        }
    }

    return out;
}

#include "Utils/WindowsInclude.h"
#include <filesystem>
#include <string>

bool ConvertStringToCleanAbsolutePath(const std::wstring& input, std::filesystem::path& out)
{
    std::filesystem::path p(input);

    // If it doesn't look like a path (no root, no parent), treat it as a bare name and search.
    // Examples that go to PATH search: "git", "tool.exe"
    // Examples treated as paths: ".\\tool.exe", "C:\\bin\\tool.exe", "..\\tool.exe", "\\server\\share\\x"
    if (!p.has_root_name() && !p.has_root_directory() && !p.has_parent_path())
    {
        DWORD required = SearchPathW(nullptr, input.c_str(), nullptr, 0, nullptr, nullptr);
        if (required == 0) 
        {
            return false;
        }

        std::wstring buf(required, L'\0'); // required includes null terminator in this probe pattern
        DWORD written = SearchPathW(nullptr, input.c_str(), nullptr, required, buf.data(), nullptr);
        if (written == 0 || written >= required) 
        {
            return false;
        }

        buf.resize(written);
        p = std::filesystem::path(buf);
    }

    std::error_code ec;

    std::filesystem::path abs = std::filesystem::absolute(p, ec);
    if (ec) 
    {
        return false;
    }

    // Best-effort: if the target exists, weakly_canonical tends to produce a nicer path,
    // but won't throw if parts are missing (unlike canonical()).
    std::filesystem::path canon = std::filesystem::weakly_canonical(abs, ec);
    if (ec)
    {
        canon = abs;
    }

    out = canon.lexically_normal();
    return true;
}