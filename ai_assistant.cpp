#include <iostream>
#include <string>
#include <curl/curl.h>
#include "json.hpp"

using json = nlohmann::json;

// Callback to capture response from curl
size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *output)
{
    size_t totalSize = size * nmemb;
    output->append((char *)contents, totalSize);
    return totalSize;
}

std::string askGemini(const std::string &prompt, const std::string &apiKey)
{
    CURL *curl;
    CURLcode res;
    std::string responseBuffer;

    std::string url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key=" + apiKey;

    json requestJson = {
        {"contents", {{{"role", "user"}, {"parts", {{{"text", prompt}}}}}}}};

    std::string requestBody = requestJson.dump();

    curl = curl_easy_init();
    if (curl)
    {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            curl_easy_cleanup(curl);
            curl_slist_free_all(headers);
            return "❌ Failed to reach Gemini API. Check your internet connection.";
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
    else
    {
        return "❌ Failed to initialize cURL.";
    }

    try
    {
        auto responseJson = json::parse(responseBuffer);

        // Check if candidates exist
        if (responseJson.contains("candidates") &&
            responseJson["candidates"].size() > 0 &&
            responseJson["candidates"][0]["content"]["parts"].size() > 0)
        {
            return responseJson["candidates"][0]["content"]["parts"][0]["text"];
        }
        else
        {
            return "⚠️ Gemini API did not return a valid response.";
        }
    }
    catch (...)
    {
        return "⚠️ Failed to parse the response. It might be malformed.";
    }
}

//Text to speech Gemini
void speakText(const std::string& text) {
std::string command = "echo \"" + text + "\" | festival --tts";
system(command.c_str());
}

int main()
{
    std::string apiKey = "AIzaSyAUFjvZ_0n1nnBkryA8iNS4ZAkmnCQ7Z1U"; // <- Replace this with your real Gemini API key
    std::string input;

    std::cout << "🤖 Luma Virtual Assistant (Type 'exit' to quit)\n\n";

    while (true)
    {
        std::cout << "You: ";
        std::getline(std::cin, input);

        if (input == "exit")
            break;

        std::string response = askGemini(input, apiKey);
        std::cout << "Gemini: " << response << "\n\n";

        // Speak it out
        speakText(response);
    }

    return 0;
}
