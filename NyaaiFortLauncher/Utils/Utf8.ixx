export module Utf8;

export import CoreTypes;

export struct FUtf8StreamDecoder
{
    void Append(const char* data, size_t len);
    std::wstring ConsumeAll();

private:
    std::string Pending{};
};

export std::string WideToUtf8(const std::wstring& w);
export std::wstring Utf8ToWide(std::string_view Utf8);

export struct FUtf16StreamDecoder
{
    void Append(const char* Data, size_t Length);
    std::wstring ConsumeAll();

private:
    std::string Pending{};
    bool bAtStart = true;
};
