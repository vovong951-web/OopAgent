#pragma once
#include "../client/llm_client.h"
#include "../tools/tool_registry.h"
#include "loop_detector.h"
#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <functional>
#include "action.h"
#include "../harness/step.h"

// Template Method pattern: run() là skeleton cố định,
// think()/parseToolCall()/act()/observe() là các bước con có thể override
class AgentLoop {
public:
    // Thêm modelName vào constructor
    AgentLoop(LLMClient& client, ToolRegistry& registry,
              std::string systemPrompt, std::string modelName = "gemma4");
    virtual ~AgentLoop() = default;

    std::string run(const std::string& task, int maxSteps = 10);

    // StepHook dùng Step& — đồng bộ với HarnessRunner::StepHook
    using StepHook = std::function<void(const Step&)>;

    void setStepHook(StepHook hook) { m_stepHook = std::move(hook); }

    std::string getModelName() const { return m_modelName; }

protected:
    virtual std::string think();
    virtual Action parseToolCall(const std::string& llmResponse);
    virtual std::optional<std::string> act(const Action& action);
    virtual void observe(const std::string& result);

    // Gọi hàm này cuối mỗi bước trong run() để notify Harness
    void notifyHook(const Step& step) {
        if (m_stepHook) {       // std::function rỗng = false
            m_stepHook(step);   // gọi callback HarnessRunner đã inject
        }
    }

private:
    LLMClient&           m_client;
    ToolRegistry&        m_registry;
    std::string          m_systemPrompt;
    std::string          m_modelName;      // lưu tên model để
    std::vector<Message> m_history;
    StepHook             m_stepHook;
    LoopDetector         m_loopDetector;
};