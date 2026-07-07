#include "client/ollama_client.h"
#include "agent/skill_loader.h"
#include "agent/agent_loop.h"
#include "agent/loop_detector.h"
#include "tools/tool_registry.h"
#include "tools/calculator_tool.h"
#include "tools/file_tool.h"
#include "tools/exec_tool.h"
#include "tools/web_search_tool.h"
#include "tools/memory_tool.h"
#include "harness/harness_runner.h"
#include "harness/keyword_evaluator.h"
#include "harness/functional_evaluator.h"
#include "harness/step.h"
#include <windows.h>
#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>

// System prompt dùng chung cho tất cả test
const std::string SYSTEM_PROMPT = R"(
Ban la mot AI agent co the dung cac tool sau:
- calculator: tinh toan so hoc, args la bieu thuc vi du "15*17"
- file: doc/ghi file, args co dang "write:ten_file.txt:noi_dung" hoac "read:ten_file.txt"
- write: ghi file, args co dang "write:ten_file.txt:noi_dung"
- read: doc file, args co dang "read:ten_file.txt"
- exec: chay lenh shell, args la lenh vi du "ls"
- web_search: tim kiem web, args la tu khoa
- memory: luu/tim kiem bo nho, args co dang "save:key:value" hoac "search:key"

Khi can dung tool, tra ve DUY NHAT JSON theo format:
{"tool": "ten_tool", "args": "tham_so"}

Khi da hoan thanh task, tra ve DUY NHAT JSON theo format:
{"final_answer": "cau tra loi"}

KHONG viet them bat ky text nao ngoai JSON.
)";

// URL Ollama ngrok
const std::string OLLAMA_URL = "https://regime-tapeless-foam.ngrok-free.dev";
const std::string MODEL_NAME = "gemma4:e4b";

// Registry dùng chung
ToolRegistry makeRegistry() {
    ToolRegistry registry;
    registry.registerTool("calculator", [] { return std::make_unique<CalculatorTool>(); });
    registry.registerTool("file",       [] { return std::make_unique<FileTool>(); });
    registry.registerTool("write",      [] { return std::make_unique<FileTool>(); });
    registry.registerTool("read",       [] { return std::make_unique<FileTool>(); });
    registry.registerTool("exec",       [] { return std::make_unique<ExecTool>(); });
    registry.registerTool("web_search", [] { return std::make_unique<WebSearchTool>(); });
    registry.registerTool("memory",     [] { return std::make_unique<MemoryTool>("agent_memory.db"); });
    return registry;
}

// =============================================
// TEST 1: LoopDetector (không cần Ollama)
// =============================================
void testLoopDetector() {
    std::cout << "\n===== TEST LOOP DETECTOR =====\n";

    LoopDetector detector(3, 5);
    auto s1 = detector.check(ToolCallAction{"calculator", "1+1"});
    assert(s1 == LoopDetector::Status::Ok);
    std::cout << "Buoc 1: Ok\n";

    auto s2 = detector.check(ToolCallAction{"calculator", "1+1"});
    assert(s2 == LoopDetector::Status::Ok);
    std::cout << "Buoc 2: Ok\n";

    auto s3 = detector.check(ToolCallAction{"calculator", "1+1"});
    assert(s3 == LoopDetector::Status::Warning);
    std::cout << "Buoc 3: Warning\n";

    detector.check(ToolCallAction{"calculator", "1+1"});
    detector.check(ToolCallAction{"calculator", "1+1"});
    auto s55 = detector.check(ToolCallAction{"calculator", "1+1"});
    assert(s55 == LoopDetector::Status::Critical);
    std::cout << "Buoc 5: Critical\n";

    LoopDetector detector2(3, 5);
    detector2.check(ToolCallAction{"exec", "ls"});
    detector2.check(ToolCallAction{"calculator", "1+1"});
    detector2.check(ToolCallAction{"exec", "ls"});
    auto sp4 = detector2.check(ToolCallAction{"calculator", "1+1"});
    std::cout << "Ping-pong buoc 4: "
              << (sp4 == LoopDetector::Status::Warning ? "Warning" : "Other") << "\n";

    detector2.check(ToolCallAction{"exec", "ls"});
    auto sp6 = detector2.check(ToolCallAction{"calculator", "1+1"});
    std::cout << "Ping-pong buoc 6: "
              << (sp6 == LoopDetector::Status::Critical ? "Critical" : "Other") << "\n";

    LoopDetector detector3(3, 5);
    detector3.check(ToolCallAction{"calculator", "1+1"});
    detector3.check(ToolCallAction{"calculator", "1+1"});
    detector3.check(ToolCallAction{"calculator", "1+1"});
    detector3.reset();
    auto sr = detector3.check(ToolCallAction{"calculator", "1+1"});
    assert(sr == LoopDetector::Status::Ok);
    std::cout << "Sau reset: Ok\n";

    std::cout << "===== LOOP DETECTOR: PASS =====\n";
}

// =============================================
// TEST 2: AgentLoop end-to-end (cần Ollama)
// =============================================
void testAgentLoop() {
    std::cout << "\n===== TEST AGENT LOOP =====\n";

    OllamaClient client(OLLAMA_URL, MODEL_NAME);
    ToolRegistry registry = makeRegistry();
    AgentLoop agent(client, registry, SYSTEM_PROMPT, MODEL_NAME);

    agent.setStepHook([](const Step& s) {
        std::cout << "\n[Step " << s.step_id << "] (" << s.latency_ms << "ms)\n";
        std::cout << "LLM: "     << s.thought     << "\n";
        std::cout << "Action: "  << s.action      << "\n";
        std::cout << "Ket qua: " << s.tool_result << "\n";
    });

    std::string task = "Tinh 15*17 va luu ket qua vao file result.txt";
    std::cout << "Task: " << task << "\n";

    std::string answer;
    try {
        answer = agent.run(task, 10);
    } catch (const std::exception& e) {
        std::cout << "Loi: " << e.what() << "\n";
        std::cout << "===== AGENT LOOP: FAIL (exception) =====\n";
        return;
    }
    std::cout << "\nKet qua cuoi: " << answer << "\n";

    if (std::filesystem::exists("result.txt")) {
        std::ifstream f("result.txt");
        std::string content((std::istreambuf_iterator<char>(f)),
                             std::istreambuf_iterator<char>());
        std::cout << "result.txt noi dung: " << content << "\n";
        if (content.find("255") != std::string::npos) {
            std::cout << "===== AGENT LOOP: PASS =====\n";
        } else {
            std::cout << "===== AGENT LOOP: FAIL (khong co 255) =====\n";
        }
    } else {
        std::cout << "===== AGENT LOOP: FAIL (result.txt khong ton tai) =====\n";
    }
}

// =============================================
// TEST 3: HarnessRunner
// =============================================
void testHarness() {
    std::cout << "\n===== TEST HARNESS =====\n";

    OllamaClient client(OLLAMA_URL, MODEL_NAME);
    ToolRegistry registry = makeRegistry();
    AgentLoop agent(client, registry, SYSTEM_PROMPT, MODEL_NAME);

    Task task;
    task.id          = "task_001";
    task.description = "Tinh 15*17";
    task.instruction = "Tinh 15 nhan 17, tra ve ket qua";
    task.eval_type   = "keyword";
    task.eval_config = "255";
    task.max_steps   = 10;

    HarnessRunner harness(std::make_unique<KeywordEvaluator>());
    Trajectory traj = harness.run(agent, task);

    std::cout << "Success: " << (traj.success ? "PASS" : "FAIL") << "\n";
    std::cout << "Steps: "   << traj.steps.size() << "\n";
    std::cout << "Time: "    << traj.total_time_ms << "ms\n";
    std::cout << "===== HARNESS: DONE =====\n";
}

int main() {
    SetConsoleOutputCP(CP_UTF8);

    // Test tool đơn lẻ
    ToolRegistry registry = makeRegistry();

    std::cout << "\n=== Test WebSearchTool ===\n";
    auto search = registry.getTool("web_search");
    auto searchResult = search->execute("Albert Einstein");
    std::cout << "Ket qua tim kiem: " << searchResult.value_or("khong co ket qua") << "\n";

    std::cout << "\n=== Test MemoryTool (save) ===\n";
    auto memory = registry.getTool("memory");
    auto saveResult = memory->execute("save:ten_du_an:OopAgent");
    std::cout << saveResult.value_or("loi") << "\n";

    std::cout << "\n=== Test MemoryTool (search) ===\n";
    auto searchMemResult = memory->execute("search:ten_du_an");
    std::cout << searchMemResult.value_or("loi") << "\n";

    std::cout << "\n=== Test SkillLoader ===\n";
    SkillLoader skillLoader("../skills");
    skillLoader.loadAll();
    std::cout << "Danh sach skill da load:\n";
    for (const auto& name : skillLoader.listSkills()) {
        std::cout << "- " << name << "\n";
    }

    auto prompt1 = skillLoader.buildSystemPromptForTask("Tinh 15*17 va luu vao file");
    std::cout << "\nChon skill cho task 'Tinh 15*17': "
              << (prompt1.empty() ? "Khong co skill phu hop."
                  : "Da chon skill, do dai: " + std::to_string(prompt1.size()) + " ky tu") << "\n";

    auto prompt2 = skillLoader.buildSystemPromptForTask("Loi khi doc file, can xu ly");
    std::cout << "Chon skill cho task 'Loi doc file': "
              << (prompt2.empty() ? "Khong co skill phu hop."
                  : "Da chon skill, do dai: " + std::to_string(prompt2.size()) + " ky tu") << "\n";

    testLoopDetector();
    testAgentLoop();
    testHarness();

    return 0;
}