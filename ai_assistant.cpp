#include <iostream>
#include <string>
#include <curl/curl.h>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

// Callback to capture response from curl
size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

// Function to send prompt to Gemini and get a response
string askGemini(const string& prompt, const string& apiKey) {
    CURL* curl;
    CURLcode res;
    string responseBuffer;

    string url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key=" + apiKey;

    json requestJson = {
        {"contents", {
            {
                {"role", "user"},
                {"parts", { {{"text", prompt}} }}
            }
        }}
    };

    curl = curl_easy_init();
    if (!curl) return "❌ Failed to initialize cURL.";

    string requestBody = requestJson.dump();

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);

    res = curl_easy_perform(curl);

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        return "❌ Request failed. Check your internet or API key.";
    }

    try {
        auto responseJson = json::parse(responseBuffer);
        if (responseJson.contains("candidates") &&
            !responseJson["candidates"].empty() &&
            !responseJson["candidates"][0]["content"]["parts"].empty()) {

            return responseJson["candidates"][0]["content"]["parts"][0]["text"];
        }
        return "⚠️ No valid response from Gemini.";
    } catch (...) {
        return "⚠️ Could not parse Gemini's response.";
    }
}

// Text-to-speech using Festival
void speakText(const string& text) {
    string command = "echo \"" + text + "\" | festival --tts";
    system(command.c_str());
}

// Main loop
int main() {
    string apiKey = "AIzaSyAUFjvZ_0n1nnBkryA8iNS4ZAkmnCQ7Z1U"; // Replace with your real API key
    string input;

    cout << "🤖 Luma Virtual Assistant (type 'exit' to quit)\n\n";

    while (true) {
        cout << "You: ";
        getline(cin, input);

        if (input == "exit") break;

        string response = askGemini(input, apiKey);

        // Replace '*' with space (optional)
        for (char& c : response) {
            if (c == '*') c = ' ';
        }

        cout << "Gemini: " << response << "\n\n";
        speakText(response);
    }

    return 0;
}
