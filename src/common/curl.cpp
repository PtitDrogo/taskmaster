#include <cstdlib>
#include <curl/curl.h>
#include <iostream>
#include <string>

std::string getEnv(const char *name, const std::string &fallback = "") {
    const char *v = std::getenv(name);
    return v ? v : fallback;
}

bool notifyDiscord(const std::string &message) {
    // Escape quotes/backslashes/newlines for JSON
    std::string webhook_url = getEnv("DISCORD_WEBHOOK");
    std::string esc;
    if (webhook_url.empty()) {
        std::cout << "You didnt't set the discord webhook token" << std::endl;
        return false;
    }
    for (char c : message) {
        switch (c) {
        case '"':
            esc += "\\\"";
            break;
        case '\\':
            esc += "\\\\";
            break;
        case '\n':
            esc += "\\n";
            break;
        default:
            esc += c;
        }
    }
    std::string payload = "{\"content\":\"" + esc + "\"}";

    CURL *curl = curl_easy_init();
    if (!curl)
        return false;

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, webhook_url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return res == CURLE_OK;
}