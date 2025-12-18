#include "WaitForBackendToRespondAction.h"

#include <thread>
#include <curl/curl.h>

#include "Utils/Utf8.h"
#include "Utils/WindowsInclude.h"

GENERATE_BASE_CPP(NWaitForBackendToRespondAction)

static bool IsUrlResponding(const std::wstring& InUrl)
{
    CURL* CurlHandle = curl_easy_init();
    if (!CurlHandle)
    {
        Log(Error, L"Failed to initialize CURL handle.");
        return false;
    }

    const std::string UrlUtf8 = WideToUtf8(InUrl);
    if (UrlUtf8.empty())
    {
        Log(Error, L"URL conversion to UTF-8 failed.");
        curl_easy_cleanup(CurlHandle);
        return false;
    }

    curl_easy_setopt(CurlHandle, CURLOPT_URL, UrlUtf8.c_str());
    curl_easy_setopt(CurlHandle, CURLOPT_NOBODY, 1L);             // HEAD request
    curl_easy_setopt(CurlHandle, CURLOPT_CONNECTTIMEOUT_MS, 2000L);
    curl_easy_setopt(CurlHandle, CURLOPT_TIMEOUT_MS, 2000L);

    // Often helpful in practice:
    curl_easy_setopt(CurlHandle, CURLOPT_FOLLOWLOCATION, 1L);     // handle redirects
    curl_easy_setopt(CurlHandle, CURLOPT_USERAGENT, "FortLauncher/1.0");
    curl_easy_setopt(CurlHandle, CURLOPT_NOSIGNAL, 1L);           // avoid signals/timeouts issues

    const CURLcode Result = curl_easy_perform(CurlHandle);

    bool bIsAlive = false;
    if (Result == CURLE_OK)
    {
        long ResponseCode = 0;
        curl_easy_getinfo(CurlHandle, CURLINFO_RESPONSE_CODE, &ResponseCode);
        bIsAlive = (ResponseCode != 0); // got an HTTP response
    }

    curl_easy_cleanup(CurlHandle);
    return bIsAlive;
}

void NWaitForBackendToRespondAction::Execute()
{
    Super::Execute();

    Log(Info, L"Waiting for backend at '{}' to respond", Url);
    
    while (true)
    {
        if (IsUrlResponding(Url))
        {
            Log(Info, L"Backend at '{}' responded successfully!", Url);
            break;
        }
        
        Log(Error, L"No response from '{}'. Will retry in 3 seconds...", Url);
        
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}

