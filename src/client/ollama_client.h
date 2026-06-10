#pragma once
#include "llm_client.h"
#include <string>
#include <vector>

class OllamaClient : public LLMClient {
public:
    OllamaClient(const std::string& baseUrl, const std::string& model);

    std::string chat(const std::vector<Message>& messages) override;
    void setModel(const std::string& model) override;
    void setTemperature(float temp) override;

private:
    std::string m_baseUrl;
    std::string m_model;
    float m_temperature = 0.7f;
    int m_maxTokens = 2048;

    std::string httpPost(const std::string& url, const std::string& body);
};