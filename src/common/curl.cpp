#include <curl/curl.h>
#include <string>

bool notifyDiscord(const std::string &message) {
    // Escape quotes/backslashes/newlines for JSON
    std::string webhook_url = "https://discord.com/api/webhooks/1558076376261857280/"
                              "kIDGJ1A0txp-ZC6_Jq79fiO4nkeSPdsj9n9fJ-UTW_PAkZzTNHttT4__TkYuhcByIW4U";
    std::string esc;
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