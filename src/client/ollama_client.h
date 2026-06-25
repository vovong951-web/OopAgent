#pragma once
#include "llm_client.h"
#include <string>
#include <vector>

class OllamaClient : public LLMClient {
public:
    OllamaClient(const std::string& baseUrl, const std::string& model);
    //                         địa chỉ ollama server, model dùng

    std::string chat(const std::vector<Message>& messages) override;
    void setModel(const std::string& model) override;
    void setTemperature(float temp) override;

private:
    std::string m_baseUrl; // http://localhost:11434
    std::string m_model; // gemma4, qwen3...
    float m_temperature = 0.7f; // mặc định 0.7
    int m_maxTokens = 2048;   // giới hạn độ dài response(phản ứng)

    std::string httpPost(const std::string& url, const std::string& body); 
    // url → địa chỉ gửi request: http://localhost:11434/api/chat
    // body → nội dung JSON gửi lên: gồm (messages, model, temperature...)
};