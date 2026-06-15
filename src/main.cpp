#include "client/ollama_client.h"
#include "tools/tool_registry.h"
#include "tools/calculator_tool.h"
#include "tools/file_tool.h"
#include "tools/exec_tool.h"
#include <iostream>

int main() {
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

    // Test listTools
    std::cout << "=== Danh sach tool ===" << std::endl;
    for (const auto& name : registry.listTools()) {
        std::cout << "- " << name << std::endl;
    }

    // Test buildToolDescriptions
    std::cout << "\n=== Mo ta tool ===" << std::endl;
    std::cout << registry.buildToolDescriptions() << std::endl;

    // Test CalculatorTool
    std::cout << "=== Test Calculator ===" << std::endl;
    auto calc = registry.getTool("calculator");
    auto result = calc->execute("15*17");
    std::cout << "15*17 = " << result.value_or("loi") << std::endl;

    // Test FileTool
    std::cout << "\n=== Test File ===" << std::endl;
    auto file = registry.getTool("file");
    file->execute("write:test.txt:Hello from FileTool");
    auto content = file->execute("read:test.txt");
    std::cout << "Doc file: " << content.value_or("loi") << std::endl;

    // Test ExecTool
    std::cout << "\n=== Test Exec ===" << std::endl;
    auto exec = registry.getTool("exec");
    auto output = exec->execute("echo Hello from ExecTool");
    std::cout << "Exec output: " << output.value_or("loi") << std::endl;

    // Test deny policy
    std::cout << "\n=== Test Deny Policy ===" << std::endl;
    registry.denyTool("exec");
    auto denied = registry.getTool("exec");
    std::cout << "Exec sau khi deny: " << (denied == nullptr ? "bi chan" : "van chay") << std::endl;

    return 0;
}