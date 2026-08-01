# OopAgent — AI Agent Framework với Ollama API

**Sinh viên:** Võ Văn Vọng — MSSV: 25127251  
**Môn:** Lập trình Hướng Đối Tượng — Năm 2026

---

## Yêu cầu hệ thống

- Windows 10/11 + MSYS2 UCRT64
- GCC 16.1.0+, CMake 4.3.3+, Ninja
- libcurl 8.20.0, nlohmann-json 3.12.0, sqlite3 3.53.2

---

## Build

    cd /c/Users/dell/OopAgent
    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build build

---

## Cấu hình Ollama (Google Colab)

1. Mở notebook: https://colab.research.google.com/drive/1chSCUNnLCj4Gj4daq6Erp7h8tF0C28lE
2. Nhấn **Chạy tất cả** — chờ 10-15 phút
3. Lấy URL ngrok từ output:
       * ngrok tunnel https://xxx.ngrok-free.dev -> http://127.0.0.1:11434
4. Mở src/main.cpp, sửa dòng:
       const std::string OLLAMA_URL = "https://xxx.ngrok-free.dev";
5. Build lại: cmake --build build

---

## Chạy agent

    cd build
    ./OopAgent.exe

Output chạy lần lượt:
- Test WebSearchTool, MemoryTool, SkillLoader
- TEST LOOP DETECTOR
- TEST AGENT LOOP (cần Ollama)
- TEST HARNESS (cần Ollama)

---

## Chạy benchmark 10 task

    cp /c/Users/dell/OopAgent/benchmark/tasks.json /c/Users/dell/OopAgent/build/
    cd build
    ./OopAgent.exe
    cat benchmark/batch_summary.json

---

## Cấu trúc thư mục

    OopAgent/
    ├── CMakeLists.txt
    ├── README.md
    ├── src/
    │   ├── agent/          # AgentLoop, LoopDetector, SkillLoader
    │   ├── client/         # LLMClient, OllamaClient
    │   ├── tools/          # Tool, ToolRegistry, 5 tool implementations
    │   └── harness/        # HarnessRunner, Trajectory, Evaluators
    ├── skills/             # .md skill files
    ├── benchmark/          # tasks.json, run_eval.cpp
    ├── tests/              # integration tests
    └── docs/               # UML diagrams

---

## Design Patterns

| Pattern | Ở đâu | Class liên quan |
|---|---|---|
| Strategy | Evaluator hierarchy | KeywordEvaluator, FunctionalEvaluator |
| Template Method | AgentLoop::run() skeleton | AgentLoop |
| Registry/Factory | Đăng ký tool theo tên | ToolRegistry |
| Observer/Hook | HarnessRunner inject step_hook | HarnessRunner, AgentLoop |

---

## C++ Features

| Standard | Feature | Dùng ở đâu |
|---|---|---|
| C++17 | std::optional | Tool::execute return |
| C++17 | std::unique_ptr | ToolRegistry instances |
| C++17 | std::function + Lambda | registerTool factory |
| C++17 | std::variant | Action type trong AgentLoop |
| C++17 | std::filesystem | SkillLoader, FileTool |
| C++17 | std::visit | Xử lý Action types |
| C++17 | Structured bindings | Duyệt map trong registry |
| C++17 | Abstract class | LLMClient, Tool, Evaluator |
| C++20 | std::ranges::views | Filter task trong HarnessRunner |
| C++20 | Concepts | Template constraint Registry |
| C++23 | std::expected | ToolResult type |
| C++26 | std::execution | Async tool execution |

---

## Kết quả benchmark

- Model: gemma4:e4b
- Tổng task: 10 (4 easy, 4 medium, 2 hard)
- Success rate: cập nhật sau khi chạy đủ