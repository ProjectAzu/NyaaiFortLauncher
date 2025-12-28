#pragma once

#include <string>
#include <vector>

struct FPropertySetData
{
    std::wstring PropertyName{};
    std::wstring SetValue;
};

typedef std::vector<FPropertySetData> FDefaultValueOverrides;