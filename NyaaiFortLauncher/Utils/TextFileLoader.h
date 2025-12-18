#pragma once

#include <filesystem>
#include <string>

namespace Utils
{
    // Loads a text file into UTF-16 std::wstring on Windows.
    // Supports: UTF-8 (with/without BOM), UTF-16LE BOM, UTF-16BE BOM.
    // If no BOM: tries strict UTF-8 first, then falls back to ANSI/ACP.
    std::wstring LoadTextFileToWString(const std::filesystem::path& path);
}
