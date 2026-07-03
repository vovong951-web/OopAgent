#pragma once
#include <string>
#include <nlohmann/json.hpp>

struct Step {
    int         step_id    = 0;
    std::string thought;
    std::string action;
    std::string tool_result;
    int         tokens_used = 0;
    long long   latency_ms  = 0;

    nlohmann::json toJson() const {
        return {
            {"step_id",     step_id},
            {"thought",     thought},
            {"action",      action},
            {"tool_result", tool_result},
            {"tokens_used", tokens_used},
            {"latency_ms",  latency_ms}
        };
    }
};