#include "agent_loop.h"
#include <nlohmann/json.hpp>
#include <regex>
#include <chrono>

AgentLoop::AgentLoop(LLMClient& client, ToolRegistry& registry,
                     std::string systemPrompt, std::string modelName)
    : m_client(client)
    , m_registry(registry)
    , m_systemPrompt(std::move(systemPrompt))
    , m_modelName(std::move(modelName))
{}

std::string AgentLoop::run(const std::string& task, int maxSteps) {
    m_history.clear();
    std::string fullSystemPrompt = m_systemPrompt + "\n\n" + m_registry.buildToolDescriptions();
    m_loopDetector.reset();
    m_history.push_back({"system", fullSystemPrompt});
    m_history.push_back({"user", task});

    for (int step = 0; step < maxSteps; ++step) {
        auto t0 = std::chrono::steady_clock::now();

        std::string thought = think();
        Action action = parseToolCall(thought);

        if (auto* done = std::get_if<DoneAction>(&action)) {
            // Tạo Step cho bước Done rồi notify hook
            Step s;
            s.step_id    = step;
            s.thought    = thought;
            s.action     = "done";
            s.tool_result = done->finalAnswer;
            s.tokens_used = 0;
            s.latency_ms  = 0;
            notifyHook(s);  // Observer pattern — gọi callback HarnessRunner đã inject
            return done->finalAnswer;
        }

        auto status = m_loopDetector.check(action);
        if (status == LoopDetector::Status::Critical) {
            m_history.push_back({"system", "Phát hiện loop nghiêm trọng, dừng agent."});
            return "Agent dừng do phát hiện loop lặp lại liên tục.";
        }
        if (status == LoopDetector::Status::Warning) {
            m_history.push_back({"system", "Cảnh báo: có dấu hiệu lặp hành động, hãy thử cách khác."});
        }

        std::optional<std::string> result = act(action);

        auto t1 = std::chrono::steady_clock::now();
        auto latencyMs = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

        std::string resultStr = result.value_or("(không có kết quả)");
        observe(resultStr);

        // Tạo Step object rồi gọi notifyHook — đồng bộ với StepHook = std::function<void(const Step&)>
        Step s;
        s.step_id     = step;
        s.thought     = thought;
        // Lưu action dưới dạng string: nếu là ToolCallAction thì ghi "toolName(args)"
        if (auto* tc = std::get_if<ToolCallAction>(&action)) {
            s.action = tc->toolName + "(" + tc->args + ")";
        } else if (std::get_if<ErrorAction>(&action)) {
            s.action = "error";
        }
        s.tool_result = resultStr;
        s.tokens_used = 0;       // TODO: lấy từ LLM response nếu API trả về usage
        s.latency_ms  = latencyMs;

        notifyHook(s); // notify HarnessRunner ghi vào Trajectory
    }

    return "Đạt giới hạn max_steps (" + std::to_string(maxSteps) + ") mà chưa hoàn thành task.";
}

std::string AgentLoop::think() {
    return m_client.chat(m_history);
}

Action AgentLoop::parseToolCall(const std::string& llmResponse) {
    m_history.push_back({"assistant", llmResponse});

    // Tìm TẤT CẢ JSON blocks, lấy cái cuối cùng
    static const std::regex jsonBlock(R"(\{[^{}]*\})");
    
    std::string lastMatch;
    auto begin = std::sregex_iterator(llmResponse.begin(), llmResponse.end(), jsonBlock);
    auto end   = std::sregex_iterator();
    
    for (auto it = begin; it != end; ++it) {
        lastMatch = it->str(); // lấy match cuối cùng
    }

    if (lastMatch.empty()) {
        return ErrorAction{"Không tìm thấy JSON tool call trong response."};
    }

    try {
        auto j = nlohmann::json::parse(lastMatch);
        if (j.contains("final_answer")) {
            return DoneAction{j.at("final_answer").get<std::string>()};
        }
        if (j.contains("tool") && j.contains("args")) {
            auto toolName = j.at("tool").get<std::string>();
            auto args = j.at("args").is_string()
                          ? j.at("args").get<std::string>()
                          : j.at("args").dump();
            return ToolCallAction{toolName, args};
        }
        return ErrorAction{"JSON thiếu field 'tool'/'args' hoặc 'final_answer'."};
    } catch (const nlohmann::json::parse_error& e) {
        return ErrorAction{std::string("Lỗi parse JSON: ") + e.what()};
    }
}

std::optional<std::string> AgentLoop::act(const Action& action) {
    return std::visit([this](auto&& a) -> std::optional<std::string> {
        using T = std::decay_t<decltype(a)>;
        if constexpr (std::is_same_v<T, ToolCallAction>) {
            std::unique_ptr<Tool> tool = m_registry.getTool(a.toolName);
            if (!tool) {
                return "Lỗi: tool '" + a.toolName + "' không tồn tại hoặc bị deny.";
            }
            return tool->execute(a.args);
        } else if constexpr (std::is_same_v<T, ErrorAction>) {
            return "Lỗi: " + a.message;
        } else {
            return std::nullopt;
        }
    }, action);
}

void AgentLoop::observe(const std::string& result) {
    m_history.push_back({"tool", result});
}