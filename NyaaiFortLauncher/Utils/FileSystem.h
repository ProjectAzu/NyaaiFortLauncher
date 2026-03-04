#pragma once

#include <filesystem>
#include <vector>

namespace Utils
{
    bool ConvertPathToAbsolutePath(
       const std::filesystem::path& InInput,
       const std::vector<std::filesystem::path>& InAbsoluteSearchRoots, // MUST be absolute paths
       std::filesystem::path& OutPath);   
}
