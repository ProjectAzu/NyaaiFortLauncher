#include "WaitForBackendToRespondAction.h"

#include <thread>
#include <curl/curl.h>

GENERATE_BASE_CPP(NWaitForBackendToRespondAction)

static bool IsUrlResponding(const std::string& InUrl)
{
    CURL* CurlHandle = curl_easy_init();
    if (!CurlHandle)
    {
        Log(Error, "Failed to initialize CURL handle.");
        return false;
    }

    // Configure it
    curl_easy_setopt(CurlHandle, CURLOPT_URL, InUrl.c_str());
    curl_easy_setopt(CurlHandle, CURLOPT_NOBODY, 1L);          // HEAD request
    curl_easy_setopt(CurlHandle, CURLOPT_CONNECTTIMEOUT, 2L); // 2-second connect timeout
    curl_easy_setopt(CurlHandle, CURLOPT_TIMEOUT, 2L);        // 2-second total timeout

    // Perform the request
    const CURLcode Result = curl_easy_perform(CurlHandle);
    bool bIsAlive = false;
    if (Result == CURLE_OK)
    {
        // We got some response; check the HTTP response code
        long ResponseCode = 0;
        curl_easy_getinfo(CurlHandle, CURLINFO_RESPONSE_CODE, &ResponseCode);

        // If the server sent a real HTTP code (200, 404, 500, etc.), we consider it "responding"
        if (ResponseCode != 0)
        {
            bIsAlive = true;
        }
    }

    curl_easy_cleanup(CurlHandle);
    return bIsAlive;
}

void NWaitForBackendToRespondAction::Execute()
{
    Super::Execute();

    Log(Info, "Waiting for backend at '{}' to respond", Url);
    
    while (true)
    {
        if (IsUrlResponding(Url))
        {
            Log(Info, "Backend at '{}' responded successfully!", Url);
            break;
        }
        
        Log(Error, "No response from '{}'. Will retry in 3 seconds...", Url);
        
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}

