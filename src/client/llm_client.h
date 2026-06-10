#pragma once
#include <string>
#include <vector>

struct Message {
    std::string role;
    std::string content;
};

class LLMClient {
public:
    virtual ~LLMClient() = default;
    virtual std::string chat(const std::vector<Message>& messages) = 0;
    virtual void setModel(const std::string& model) = 0;
    virtual void setTemperature(float temp) = 0;
};