module;
#include "Utils/WindowsInclude.h"

module Utf8;

void FUtf16StreamDecoder::Append(const char* Data, size_t Length)
{
    Pending.append(Data, Length);
}

std::wstring FUtf16StreamDecoder::ConsumeAll()
{
    std::wstring Result{};
    size_t Offset = 0;

    const auto ReadUnit = [this](size_t Index)
    {
        return scast<uint16>(scast<uint8>(Pending[Index]) | (scast<uint8>(Pending[Index + 1]) << 8));
    };

    while (Offset + 1 < Pending.size())
    {
        const uint16 Unit = ReadUnit(Offset);

        if (Unit >= 0xD800 && Unit <= 0xDBFF)
        {
            if (Offset + 3 >= Pending.size())
            {
                break;
            }

            const uint16 Next = ReadUnit(Offset + 2);

            if (Next >= 0xDC00 && Next <= 0xDFFF)
            {
                Result += scast<wchar_t>(Unit);
                Result += scast<wchar_t>(Next);
                Offset += 4;
                bAtStart = false;
                continue;
            }

            Result += L'\uFFFD';
        }
        else if (Unit >= 0xDC00 && Unit <= 0xDFFF)
        {
            Result += L'\uFFFD';
        }
        else if (!bAtStart || Unit != 0xFEFF)
        {
            Result += scast<wchar_t>(Unit);
        }

        bAtStart = false;
        Offset += 2;
    }

    Pending.erase(0, Offset);

    return Result;
}

void FUtf8StreamDecoder::Append(const char* data, size_t len)
{
    Pending.append(data, len);
}

std::wstring FUtf8StreamDecoder::ConsumeAll()
{
    std::wstring out;
    out.reserve(Pending.size()); // rough

    const unsigned char* s = rcast<const unsigned char*>(Pending.data());
    size_t n = Pending.size();
    size_t i = 0;

    auto emit_u16 = [&](uint32 cp)
    {
        if (cp <= 0xFFFF)
        {
            out.push_back(scast<wchar_t>(cp));
        }
        else
        {
            cp -= 0x10000;
            out.push_back(scast<wchar_t>(0xD800 + (cp >> 10)));
            out.push_back(scast<wchar_t>(0xDC00 + (cp & 0x3FF)));
        }
    };

    auto invalid = [&]()
    {
        emit_u16(0xFFFD);
        ++i; // consume 1 byte to resync (always progresses)
    };

    while (i < n)
    {
        unsigned char b0 = s[i];

        // ASCII

        if (b0 < 0x80)
        {
            emit_u16(b0);
            ++i;
            continue;
        }

        // Determine expected length and basic validity of lead byte
        int len = 0;

        if (b0 >= 0xC2 && b0 <= 0xDF)
        {
            len = 2;
        }
        else if (b0 >= 0xE0 && b0 <= 0xEF)
        {
            len = 3;
        }
        else if (b0 >= 0xF0 && b0 <= 0xF4)
        {
            len = 4;
        }
        else
        {
            invalid();
            continue;
        }

        // Incomplete sequence at end: keep it for next call

        if (i + scast<size_t>(len) > n)
        {
            break;
        }

        unsigned char b1 = s[i + 1];

        if ((b1 & 0xC0) != 0x80)
        {
            invalid();
            continue;
        }

        uint32 cp = 0;

        if (len == 2)
        {
            cp = ((b0 & 0x1F) << 6) | (b1 & 0x3F);
            // b0>=0xC2 already prevents overlongs
            emit_u16(cp);
            i += 2;
            continue;
        }

        unsigned char b2 = s[i + 2];

        if ((b2 & 0xC0) != 0x80)
        {
            invalid();
            continue;
        }

        if (len == 3)
        {
            // Overlong and surrogate checks via lead-specific constraints

            if (b0 == 0xE0 && b1 < 0xA0)
            {
                invalid();
                continue;
            } // overlong

            if (b0 == 0xED && b1 > 0x9F)
            {
                invalid();
                continue;
            } // surrogates

            cp = ((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);

            // Reject UTF-16 surrogate range explicitly

            if (cp >= 0xD800 && cp <= 0xDFFF)
            {
                invalid();
                continue;
            }

            emit_u16(cp);
            i += 3;
            continue;
        }

        // len == 4
        unsigned char b3 = s[i + 3];

        if ((b3 & 0xC0) != 0x80)
        {
            invalid();
            continue;
        }

        // Overlong / >U+10FFFF constraints

        if (b0 == 0xF0 && b1 < 0x90)
        {
            invalid();
            continue;
        } // overlong

        if (b0 == 0xF4 && b1 > 0x8F)
        {
            invalid();
            continue;
        } // > 0x10FFFF

        cp = ((b0 & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);

        if (cp > 0x10FFFF)
        {
            invalid();
            continue;
        }

        emit_u16(cp);
        i += 4;
    }

    // Keep only the undecoded tail (0..3 bytes)

    if (i > 0)
    {
        Pending.erase(0, i);
    }

    return out;
}

std::string WideToUtf8(const std::wstring& w)
{
    if (w.empty())
    {
        return {};
    }

    int needed = WideCharToMultiByte(CP_UTF8, 0, w.data(), scast<int>(w.size()), nullptr, 0, nullptr, nullptr);

    if (needed <= 0)
    {
        return {};
    }

    std::string out(scast<size_t>(needed), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), scast<int>(w.size()), out.data(), needed, nullptr, nullptr);

    return out;
}

std::wstring Utf8ToWide(std::string_view Utf8)
{
    if (Utf8.empty())
    {
        return {};
    }

    int RequiredWchars = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, Utf8.data(), scast<int>(Utf8.size()), nullptr, 0);

    if (RequiredWchars <= 0)
    {
        // Fallback if there are invalid sequences.
        RequiredWchars = MultiByteToWideChar(
            CP_UTF8, 0, Utf8.data(), scast<int>(Utf8.size()), nullptr, 0);
    }

    if (RequiredWchars <= 0)
    {
        return {};
    }

    std::wstring Wide(RequiredWchars, L'\0');
    MultiByteToWideChar(
        CP_UTF8, 0, Utf8.data(), scast<int>(Utf8.size()), Wide.data(), RequiredWchars);

    return Wide;
}
