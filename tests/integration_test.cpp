// tests/integration_test.cpp
// Tuan 5 - Integration test: khong dung mock, goi tool that qua ToolRegistry
//
// LUU Y: File nay gia dinh header cua ban o cac duong dan ben duoi va
// FileTool::execute() nhan args dang "path|content" (phan tach boi ky tu '|').
// Neu FileTool cua ban dung format khac (vd JSON args), chi can sua lai
// bien writeArgs trong testFileToolIntegration() cho khop.

#include <iostream>
#include <cassert>
#include <optional>
#include <string>
#include <filesystem>
#include <fstream>
#include <memory>

#include "tools/tool_registry.h"
#include "tools/calculator_tool.h"
#include "tools/file_tool.h"

namespace fs = std::filesystem;

bool testCalculatorIntegration(ToolRegistry& registry) {
    std::cout << "[TEST] CalculatorTool integration...\n";

    auto&& calcHandle = registry.getTool("calculator");
    Tool* calc = calcHandle.get();
    if (calc == nullptr) {
        std::cerr << "  FAIL: khong tim thay tool 'calculator' trong registry\n";
        return false;
    }

    std::optional<std::string> result = calc->execute("15*17");

    if (!result.has_value()) {
        std::cerr << "  FAIL: calculator tra ve std::nullopt\n";
        return false;
    }

    if (result.value() != "255") {
        std::cerr << "  FAIL: ket qua sai, mong doi '255', nhan duoc '"
                   << result.value() << "'\n";
        return false;
    }

    std::cout << "  PASS: 15*17 = " << result.value() << "\n";
    return true;
}

bool testFileToolIntegration(ToolRegistry& registry) {
    std::cout << "[TEST] FileTool integration...\n";

    const std::string testPath = "integration_test_output.txt";
    const std::string testContent = "255";

    if (fs::exists(testPath)) {
        fs::remove(testPath);
    }

    auto&& fileToolHandle = registry.getTool("write_file");
    Tool* fileTool = fileToolHandle.get();
    if (fileTool == nullptr) {
        std::cerr << "  FAIL: khong tim thay tool 'write_file' trong registry\n";
        return false;
    }

    std::string writeArgs = "write:" + testPath + ":" + testContent;
    std::optional<std::string> writeResult = fileTool->execute(writeArgs);

    if (!writeResult.has_value()) {
        std::cerr << "  FAIL: write_file tra ve std::nullopt\n";
        return false;
    }

    if (!fs::exists(testPath)) {
        std::cerr << "  FAIL: file '" << testPath << "' khong duoc tao\n";
        return false;
    }

    std::ifstream inFile(testPath);
    std::string line;
    std::getline(inFile, line);
    inFile.close();

    if (line != testContent) {
        std::cerr << "  FAIL: noi dung file sai, mong doi '" << testContent
                   << "', nhan duoc '" << line << "'\n";
        fs::remove(testPath);
        return false;
    }

    std::cout << "  PASS: file '" << testPath << "' chua noi dung dung: "
               << line << "\n";
    fs::remove(testPath);
    return true;
}

int main() {
    ToolRegistry registry;

    registry.registerTool("calculator", []() {
        return std::make_unique<CalculatorTool>();
    });
    registry.registerTool("write_file", []() {
        return std::make_unique<FileTool>();
    });

    int passed = 0;
    int total = 2;

    if (testCalculatorIntegration(registry)) passed++;
    if (testFileToolIntegration(registry)) passed++;

    std::cout << "\n=== KET QUA: " << passed << "/" << total << " test PASS ===\n";

    return (passed == total) ? 0 : 1;
}