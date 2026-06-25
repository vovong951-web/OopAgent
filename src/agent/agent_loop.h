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


// Template Method pattern: run() là skeleton cố định,
// think()/parseToolCall()/act()/observe() là các bước con có thể override
class AgentLoop {
public:
    AgentLoop(LLMClient& client, ToolRegistry& registry, std::string systemPrompt);
    virtual ~AgentLoop() = default;

    std::string run(const std::string& task, int maxSteps = 10);

    // Hook để Harness (tuần 6) gắn vào ghi Trajectory — AgentLoop không biết Harness là gì
    using StepHook = std::function<void(int stepId, const std::string& thought,
                                         const Action& action,
                                         const std::optional<std::string>& result,
                                         long long latencyMs)>;
    // std::function<void(...)> — "hộp chứa hàm" nhận 5 tham số, không trả về gì
    /*
    int stepId — số thứ tự bước (0, 1, 2...)
    const std::string& thought — nội dung LLM vừa trả về ở bước này
    const Action& action — action được parse ra từ response LLM
    const std::optional<std::string>& result — kết quả tool (hoặc nullopt nếu là DoneAction)
    long long latencyMs — thời gian bước này mất bao nhiêu millisecond
    
    Toàn bộ dòng này chỉ là định nghĩa kiểu — chưa tạo object nào
    */
    void setStepHook(StepHook hook) { m_stepHook = std::move(hook); }
    

protected:
    virtual std::string think();//think() — gửi history đến LLM, nhận response text

    virtual Action parseToolCall(const std::string& llmResponse);//parseToolCall() — phân tích response text thành Action

    virtual std::optional<std::string> act(const Action& action); //act() — thực thi action (gọi tool hoặc xử lý lỗi)

    virtual void observe(const std::string& result); // observe() — ghi kết quả tool vào history để LLM đọc bước tiếp theo


private:
    LLMClient& m_client; 
    ToolRegistry& m_registry; 
    std::string m_systemPrompt;
    std::vector<Message> m_history;
    StepHook m_stepHook;
    LoopDetector m_loopDetector;
    /*
    LLMClient      → nói chuyện AI
    ToolRegistry   → dùng công cụ
    systemPrompt   → luật chơi
    history        → ký ức
    StepHook       → điểm can thiệp: làm các việ như đo thời gian, thêm code...
    LoopDetector   → chống lặp vô hạn

     hook:
     Agent chạy mỗi bước:

    Think
    Act
    Observe

    Có thể muốn:

    in log
    hiện UI
    lưu file
    debug

    nhưng không muốn viết cứng trong AgentLoop.

    Nên AgentLoop cho người khác "cắm" hàm vào.



    */
};