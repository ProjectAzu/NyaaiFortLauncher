#pragma once

#include <string>

struct FUtf8StreamDecoder
{
    void Append(const char* data, size_t len);
    std::wstring ConsumeAll();
    
private:
    std::string Pending{};
};

std::string WideToUtf8(const std::wstring& w);
std::wstring Utf8ToWide(std::string_view Utf8);