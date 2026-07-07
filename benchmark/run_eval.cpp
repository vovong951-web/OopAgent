#include "../src/client/ollama_client.h"
#include "../src/agent/agent_loop.h"
#include "../src/tools/tool_registry.h"
#include "../src/tools/calculator_tool.h"
#include "../src/tools/file_tool.h"
#include "../src/tools/web_search_tool.h"
#include "../src/tools/memory_tool.h"
#include "../src/harness/harness_runner.h"
#include "../src/harness/keyword_evaluator.h"
#include "../src/harness/functional_evaluator.h"
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

int main() {
    // Setup client
    OllamaClient client(
        "https://regime-tapeless-foam.ngrok-free.dev",  // thay URL ngrok
        "gemma4:e4b"
    );

    // Setup registry
    ToolRegistry registry;
    registry.registerTool("calculator", [] {
        return std::make_unique<CalculatorTool>();
    });
    registry.registerTool("file", [] {
        return std::make_unique<FileTool>();
    });
    registry.registerTool("web_search", [] {
        return std::make_unique<WebSearchTool>();
    });
    registry.registerTool("memory", [] {
        return std::make_unique<MemoryTool>("eval_memory.db");
    });

    std::string systemPrompt = R"(
Ban la mot AI agent. Khi can tool tra ve JSON:
{"tool": "ten_tool", "args": "tham_so"}
Khi xong tra ve: {"final_answer": "ket qua"}
KHONG viet gi ngoai JSON.
)";

    AgentLoop agent(client, registry, systemPrompt, "gemma4:e4b");

    // Đọc tasks.json — dùng std::ranges để lọc task hard (max_steps > 10)
    std::ifstream ifs("tasks.json");
    nlohmann::json json_tasks;
    ifs >> json_tasks;

    // C++20 ranges: in danh sách task hard trước khi chạy
    std::vector<std::string> hard_ids;
    for (const auto& jt : json_tasks) {
        if (jt.value("max_steps", 10) > 10) {
            hard_ids.push_back(jt["id"].get<std::string>());
        }
    }
    auto hard_view = hard_ids | std::views::transform([](const std::string& id) {
        return "  [HARD] " + id;
    });
    std::cout << "Hard tasks:\n";
    for (const auto& s : hard_view) std::cout << s << "\n";

    // Chạy batch với KeywordEvaluator làm mặc định
    // FunctionalEvaluator dùng cho task có eval_type = "functional"
    HarnessRunner harness(std::make_unique<KeywordEvaluator>());
    harness.runBatch(agent, "tasks.json");

    return 0;
}