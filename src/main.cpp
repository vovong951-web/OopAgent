#include "client/ollama_client.h"
#include "agent/skill_loader.h"
#include "agent/agent_loop.h"      // ← THÊM
#include "agent/loop_detector.h"   // ← THÊM
#include "tools/tool_registry.h"
#include "tools/calculator_tool.h"
#include "tools/file_tool.h"
#include "tools/exec_tool.h"
#include "tools/web_search_tool.h"
#include "tools/memory_tool.h"
#include <windows.h>
#include <iostream>
#include <cassert>       // ← THÊM
#include <filesystem>    // ← THÊM
#include <fstream>       // ← THÊM
// =============================================
// TEST 1: LoopDetector (không cần Ollama)
// =============================================
void testLoopDetector() {
    std::cout << "\n===== TEST LOOP DETECTOR =====\n";

    // Generic Repeat
    LoopDetector detector(3, 5);
    auto s1 = detector.check(ToolCallAction{"calculator", "1+1"});// ktr và trả về status
    assert(s1 == LoopDetector::Status::Ok);
    // nếu không phải ok thì crash
    std::cout << "Buoc 1: Ok\n";

    auto s2 = detector.check(ToolCallAction{"calculator", "1+1"});
    assert(s2 == LoopDetector::Status::Ok);
    std::cout << "Buoc 2: Ok\n";

    auto s3 = detector.check(ToolCallAction{"calculator", "1+1"});
    assert(s3 == LoopDetector::Status::Warning);
    std::cout << "Buoc 3: Warning\n";

    auto s5 = detector.check(ToolCallAction{"calculator", "1+1"});
    detector.check(ToolCallAction{"calculator", "1+1"});
    auto s55 = detector.check(ToolCallAction{"calculator", "1+1"});
    assert(s55 == LoopDetector::Status::Critical);
    std::cout << "Buoc 5: Critical\n";

    // Ping-pong
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

    // Reset
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

    OllamaClient client(
        "https://regime-tapeless-foam.ngrok-free.dev",  // ← thay URL tunnel Colab của bạn
        "gemma4:e4b"
    );

    // Dùng lại registry đã có sẵn tool
    ToolRegistry registry;
    registry.registerTool("calculator", [] {
        return std::make_unique<CalculatorTool>();
    });
    registry.registerTool("file", [] {
        return std::make_unique<FileTool>();
    });

    std::string systemPrompt = R"(
Ban la mot AI agent co the dung tool de hoan thanh task.

Khi can dung tool, tra ve DUY NHAT JSON theo format:
{"tool": "ten_tool", "args": "tham_so"}

Khi da hoan thanh task, tra ve DUY NHAT JSON theo format:
{"final_answer": "cau tra loi"}

KHONG viet them bat ky text nao ngoai JSON.
)";

    AgentLoop agent(client, registry, systemPrompt);

    // Hook in từng bước ra console
    agent.setStepHook([](int step, const std::string& thought,
                          const Action&,
                          const std::optional<std::string>& result,
                          long long ms) {
        std::cout << "\n[Step " << step << "] (" << ms << "ms)\n";
        std::cout << "LLM: " << thought << "\n";
        std::cout << "Ket qua: " << result.value_or("(none)") << "\n";
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

    // Verify file result.txt
    if (std::filesystem::exists("result.txt")) {
        std::ifstream f("result.txt");
        std::string content((std::istreambuf_iterator<char>(f)), // đầu file
                             std::istreambuf_iterator<char>());  // cuối file
    // Đọc tất cả ký tự từ đầu file đến cuối file rồi lưu vào content.
    // string(InputIt first, InputIt last);
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


int main() {
    SetConsoleOutputCP(CP_UTF8);     // MỚI - sửa lỗi hiển thị tiếng Việt trong console

    ToolRegistry registry;

    // Đăng ký các tool
    registry.registerTool("calculator", []() {
        return std::make_unique<CalculatorTool>();
    });

    registry.registerTool("file", []() {
        return std::make_unique<FileTool>();
    });

    registry.registerTool("exec", []() {
        return std::make_unique<ExecTool>();
    });

    // MỚI: đăng ký web_search
    registry.registerTool("web_search", []() {
        return std::make_unique<WebSearchTool>();
    });

    // MỚI: đăng ký memory
    registry.registerTool("memory", []() {
        return std::make_unique<MemoryTool>("agent_memory.db");
    });

    // ... (giữ nguyên phần test listTools, buildToolDescriptions, Calculator, File, Exec, Deny Policy cũ)

    // MỚI: Test WebSearchTool
    std::cout << "\n=== Test WebSearchTool ===" << std::endl;
    auto search = registry.getTool("web_search");
    auto searchResult = search->execute("Albert Einstein");
    std::cout << "Ket qua tim kiem: " << searchResult.value_or("khong co ket qua") << std::endl;

    // MỚI: Test MemoryTool - save
    std::cout << "\n=== Test MemoryTool (save) ===" << std::endl;
    auto memory = registry.getTool("memory");
    auto saveResult = memory->execute("save:ten_du_an:OopAgent");
    std::cout << saveResult.value_or("loi") << std::endl;

    // MỚI: Test MemoryTool - search
    std::cout << "\n=== Test MemoryTool (search) ===" << std::endl;
    auto searchMemResult = memory->execute("search:ten_du_an");
    std::cout << searchMemResult.value_or("loi") << std::endl;
    
    // === Test SkillLoader ===
    std::cout << "\n=== Test SkillLoader ===" << std::endl;
    SkillLoader skillLoader("../skills"); // đường dẫn tương đối từ build/ tới skills/
    skillLoader.loadAll();

    std::cout << "Danh sach skill da load:" << std::endl;
    for (const auto& name : skillLoader.listSkills()) {
        std::cout << "- " << name << std::endl;
    }

    std::cout << "\nChon skill cho task: 'Tinh 15*17 va luu vao file'" << std::endl;
    auto prompt1 = skillLoader.buildSystemPromptForTask("Tinh 15*17 va luu vao file");
    std::cout << (prompt1.empty() ? "Khong co skill phu hop." : "Da chon skill, do dai prompt: " + std::to_string(prompt1.size()) + " ky tu") << std::endl;

    std::cout << "\nChon skill cho task: 'Loi khi doc file, can xu ly'" << std::endl;
    auto prompt2 = skillLoader.buildSystemPromptForTask("Loi khi doc file, can xu ly");
    std::cout << (prompt2.empty() ? "Khong co skill phu hop." : "Da chon skill, do dai prompt: " + std::to_string(prompt2.size()) + " ky tu") << std::endl;
    
    testLoopDetector();   // test tuần 4 - không cần Ollama
    testAgentLoop();      // test tuần 4 - cần Ollama đang chạy

    return 0;
}