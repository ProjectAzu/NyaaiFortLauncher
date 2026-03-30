#include "FileSystem.h"

#include <filesystem>
#include <string>
#include <vector>
#include <system_error>
#include <cwctype>
#include "Utils/WindowsInclude.h"

#include "CommandLine/CommandLine.h"

namespace Utils
{
    static bool ExpandEnvironmentVariablesPath(const std::filesystem::path& InPath, std::filesystem::path& OutPath)
    {
        OutPath.clear();

        if (InPath.empty())
        {
            return true;
        }

        const std::wstring Input = InPath.wstring();

        // Preserve old behavior exactly when there is nothing to expand.
        if (Input.find(L'%') == std::wstring::npos)
        {
            OutPath = InPath;
            return true;
        }

        SetLastError(ERROR_SUCCESS);
        const DWORD RequiredChars =
            ::ExpandEnvironmentStringsW(Input.c_str(), nullptr, 0);
        if (RequiredChars == 0)
        {
            return false;
        }

        std::wstring Buffer(RequiredChars, L'\0');

        SetLastError(ERROR_SUCCESS);
        const DWORD WrittenChars =
            ::ExpandEnvironmentStringsW(Input.c_str(), Buffer.data(), RequiredChars);

        if (WrittenChars == 0 || WrittenChars > RequiredChars)
        {
            return false;
        }

        if (!Buffer.empty() && Buffer.back() == L'\0')
        {
            Buffer.pop_back();
        }

        OutPath = std::filesystem::path(std::move(Buffer));
        return true;
    }

    bool ConvertPathToAbsolutePath(
        const std::filesystem::path& InInput,
        const std::vector<std::filesystem::path>& InSearchRoots, // MUST be absolute paths
        std::filesystem::path& OutPath)
    {
        if (InInput.empty())
        {
            OutPath.clear();
            return true;
        }

        std::filesystem::path CandidatePath(InInput);

        const bool bIsBareName =
            !CandidatePath.has_root_name() &&
            !CandidatePath.has_root_directory() &&
            !CandidatePath.has_parent_path();

        std::error_code ec{};

        // 1) Search in provided absolute roots first (priority order), only for relative inputs.
        if (CandidatePath.is_relative())
        {
            for (const std::filesystem::path& RootAbs : InSearchRoots)
            {
                if (!RootAbs.is_absolute())
                    continue;

                const std::filesystem::path TryPath = RootAbs / CandidatePath;

                ec.clear();
                if (std::filesystem::exists(TryPath, ec) && !ec)
                {
                    CandidatePath = TryPath;
                    break;
                }
            }
        }

        // 2) If it's still a bare name, fall back to SearchPathW (CWD + system + PATH)
        if (bIsBareName &&
            !CandidatePath.has_root_name() &&
            !CandidatePath.has_root_directory() &&
            !CandidatePath.has_parent_path())
        {
            SetLastError(ERROR_SUCCESS);
            const DWORD RequiredChars = ::SearchPathW(nullptr, CandidatePath.c_str(), nullptr, 0, nullptr, nullptr);
            if (RequiredChars == 0)
            {
                Log(Error, L"SearchPathW failed to resolve '{}' (GetLastError={})", InInput.wstring(), GetLastError());
                return false;
            }

            std::wstring Buffer(RequiredChars, L'\0');

            SetLastError(ERROR_SUCCESS);
            const DWORD WrittenChars = ::SearchPathW(nullptr, CandidatePath.c_str(), nullptr, RequiredChars, Buffer.data(), nullptr);
            if (WrittenChars == 0 || WrittenChars >= RequiredChars)
            {
                Log(Error,
                    L"SearchPathW returned an unexpected length for '{}' (Written={}, Required={}, GetLastError={})",
                    InInput.wstring(), WrittenChars, RequiredChars, GetLastError());
                return false;
            }

            Buffer.resize(WrittenChars);
            CandidatePath = std::filesystem::path(std::move(Buffer));
        }

        // 3) Expand %ENVVARS% only now.
        //    This preserves prior behavior for non-% inputs like "11.50\\" exactly.
        {
            std::filesystem::path ExpandedPath;
            if (!ExpandEnvironmentVariablesPath(CandidatePath, ExpandedPath))
            {
                Log(Error,
                    L"ExpandEnvironmentStringsW failed for '{}' (GetLastError={})",
                    CandidatePath.wstring(), ::GetLastError());
                return false;
            }

            CandidatePath = std::move(ExpandedPath);
        }

        // 4) Make absolute, verify existence, canonicalize / normalize
        ec.clear();
        const std::filesystem::path AbsolutePath = std::filesystem::absolute(CandidatePath, ec);
        if (ec)
        {
            Log(Error, L"absolute() failed for '{}' (ec={}, path='{}')", InInput.wstring(), ec.value(), CandidatePath.wstring());
            return false;
        }

        ec.clear();
        if (!std::filesystem::exists(AbsolutePath, ec) || ec)
        {
            Log(Error, L"Path does not exist or is not accessible: '{}' (from '{}')", AbsolutePath.wstring(), InInput.wstring());
            return false;
        }

        ec.clear();
        std::filesystem::path CanonicalPath = std::filesystem::weakly_canonical(AbsolutePath, ec);
        if (ec)
        {
            Log(Warning,
                L"weakly_canonical() failed for '{}' (ec={}). Using cleaned absolute path instead",
                AbsolutePath.wstring(), ec.value());

            OutPath = AbsolutePath.lexically_normal();
            return true;
        }

        OutPath = CanonicalPath.lexically_normal();
        return true;
    }
}
