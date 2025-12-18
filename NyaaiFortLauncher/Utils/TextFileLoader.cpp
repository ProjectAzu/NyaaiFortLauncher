#include "Utils/TextFileLoader.h"

#include "Utils/WindowsInclude.h"

#include <cstdint>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace
{
    static_assert(sizeof(wchar_t) == 2, "This loader expects Windows wchar_t (UTF-16).");

    std::vector<std::uint8_t> ReadAllBytes(const std::filesystem::path& path)
    {
        std::ifstream in(path, std::ios::binary | std::ios::ate);
        if (!in) return {};

        std::streamoff size = in.tellg();
        if (size <= 0) return {};

        in.seekg(0, std::ios::beg);

        std::vector<std::uint8_t> bytes(static_cast<size_t>(size));
        in.read(reinterpret_cast<char*>(bytes.data()), size);
        return bytes;
    }

    std::wstring Utf8ToWide(const char* data, int len, bool strict)
    {
        DWORD flags = strict ? MB_ERR_INVALID_CHARS : 0;

        int needed = MultiByteToWideChar(CP_UTF8, flags, data, len, nullptr, 0);
        if (needed <= 0) return {};

        std::wstring out(static_cast<size_t>(needed), L'\0');
        int written = MultiByteToWideChar(CP_UTF8, flags, data, len, out.data(), needed);
        if (written <= 0) return {};

        return out;
    }

    std::wstring AcpToWide(const char* data, int len)
    {
        int needed = MultiByteToWideChar(CP_ACP, 0, data, len, nullptr, 0);
        if (needed <= 0) return {};

        std::wstring out(static_cast<size_t>(needed), L'\0');
        int written = MultiByteToWideChar(CP_ACP, 0, data, len, out.data(), needed);
        if (written <= 0) return {};

        return out;
    }
}

namespace Utils
{
    std::wstring LoadTextFileToWString(const std::filesystem::path& path)
    {
        auto bytes = ReadAllBytes(path);
        if (bytes.empty()) return {};

        if (bytes.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
            throw std::runtime_error("File too large to convert");

        size_t offset = 0;

        // UTF-8 BOM: EF BB BF
        if (bytes.size() >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF)
        {
            offset = 3;
            return Utf8ToWide(reinterpret_cast<const char*>(bytes.data() + offset),
                              static_cast<int>(bytes.size() - offset),
                              /*strict*/ false);
        }

        // UTF-16LE BOM: FF FE
        if (bytes.size() >= 2 && bytes[0] == 0xFF && bytes[1] == 0xFE)
        {
            offset = 2;
            if (((bytes.size() - offset) % 2) != 0)
                throw std::runtime_error("UTF-16LE file has odd byte count");

            const wchar_t* wptr = reinterpret_cast<const wchar_t*>(bytes.data() + offset);
            size_t wlen = (bytes.size() - offset) / sizeof(wchar_t);
            return std::wstring(wptr, wptr + wlen);
        }

        // UTF-16BE BOM: FE FF
        if (bytes.size() >= 2 && bytes[0] == 0xFE && bytes[1] == 0xFF)
        {
            offset = 2;
            if (((bytes.size() - offset) % 2) != 0)
                throw std::runtime_error("UTF-16BE file has odd byte count");

            size_t wlen = (bytes.size() - offset) / 2;
            std::wstring out(wlen, L'\0');

            for (size_t i = 0; i < wlen; ++i)
            {
                std::uint8_t hi = bytes[offset + i * 2 + 0];
                std::uint8_t lo = bytes[offset + i * 2 + 1];
                out[i] = static_cast<wchar_t>((hi << 8) | lo);
            }
            return out;
        }

        // No BOM: try strict UTF-8, then fall back to ANSI/ACP.
        {
            auto w = Utf8ToWide(reinterpret_cast<const char*>(bytes.data()),
                                static_cast<int>(bytes.size()),
                                /*strict*/ true);
            if (!w.empty()) return w;

            return AcpToWide(reinterpret_cast<const char*>(bytes.data()),
                             static_cast<int>(bytes.size()));
        }
    }
}
