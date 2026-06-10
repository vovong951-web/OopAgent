#include "ollama_client.h"
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <stdexcept>

using json = nlohmann::json;

static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    output->append((char*)contents, size * nmemb);
    return size * nmemb;
}

OllamaClient::OllamaClient(const std::string& baseUrl, const std::string& model)
    : m_baseUrl(baseUrl), m_model(model) {}

void OllamaClient::setModel(const std::string& model) { m_model = model; }
void OllamaClient::setTemperature(float temp) { m_temperature = temp; }

std::string OllamaClient::httpPost(const std::string& url, const std::string& body) {
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("Failed to init curl");

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
        throw std::runtime_error(curl_easy_strerror(res));

    return response;
}

std::string OllamaClient::chat(const std::vector<Message>& messages) {
    json payload;
    payload["model"] = m_model;
    payload["temperature"] = m_temperature;
    payload["stream"] = false;

    json msgs = json::array();
    for (const auto& msg : messages) {
        msgs.push_back({{"role", msg.role}, {"content", msg.content}});
    }
    payload["messages"] = msgs;

    std::string response = httpPost(m_baseUrl + "/api/chat", payload.dump());

    json result = json::parse(response);
    return result["message"]["content"].get<std::string>();
}