#include "agent_loop.h"
#include <nlohmann/json.hpp> // dùng trong parseToolCall() để đọc response LLM. Đã được
#include <regex> // Thư viện regular expression — dùng trong parseToolCall() để tìm khối JSON trong response LLM
#include <chrono> // Thư viện đo thời gian — dùng trong run() để đo latencyMs (bước này mất bao nhiêu millisecond), truyền vào StepHook cho HarnessRunner

AgentLoop::AgentLoop(LLMClient& client, ToolRegistry& registry, std::string systemPrompt)
    : m_client(client), m_registry(registry), m_systemPrompt(std::move(systemPrompt)) {}
// chỉ move dc string vì string chỉ tạm , còn mấy còn lại move thì dữ liệu ban đầu có thể bị rỗng nên khi cần dùng lại thì ko dc
std::string AgentLoop::run(const std::string& task, int maxSteps) {
    m_history.clear();
    // Ghép system prompt riêng + mô tả tool tự động lấy từ registry
    std::string fullSystemPrompt = m_systemPrompt + "\n\n" + m_registry.buildToolDescriptions();
    m_loopDetector.reset();
    m_history.push_back({"system", fullSystemPrompt});
    m_history.push_back({"user", task});

    for (int step = 0; step < maxSteps; ++step) {
        auto t0 = std::chrono::steady_clock::now(); // lấy thời điểm hiện tại theo đồng hồ đơn điệu

        std::string thought = think();
        Action action = parseToolCall(thought);
        // parseToolCall của thư viện 
        /*
        Text từ LLM
            ↓
        parseToolCall
            ↓
        ToolCallAction / DoneAction / ErrorAction
        */  

        if (auto* done = std::get_if<DoneAction>(&action)) { // hử lấy con trỏ đến DoneAction bên trong action- đúng thì trả sai thì nullptr
            if (m_stepHook) m_stepHook(step, thought, action, done->finalAnswer, 0);
            //    ^  Có ai gắn callback chưa?
            return done->finalAnswer;
        }

        /*
        if (m_stepHook) — kiểm tra hook có được gắn vào chưa (empty std::function tự convert thành false)
        m_stepHook(step, thought, action, done->finalAnswer, 0) — gọi hook, truyền thông tin bước này
        done->finalAnswer — truy cập field finalAnswer của DoneAction qua con trỏ done
        0 — latency = 0 vì bước Done không chạy tool, không cần đo thời gian thật
        */
        auto status = m_loopDetector.check(action);
        if (status == LoopDetector::Status::Critical) {
            m_history.push_back({"system", "Phát hiện loop nghiêm trọng, dừng agent."});
            return "Agent dừng do phát hiện loop lặp lại liên tục.";
        }
        if (status == LoopDetector::Status::Warning) {
            m_history.push_back({"system", "Cảnh báo: có dấu hiệu lặp hành động, hãy thử cách khác."});
        }

        std::optional<std::string> result = act(action);
        auto t1 = std::chrono::steady_clock::now(); // t1 — thời điểm kết thúc bước (sau khi tool chạy xong)
        auto latencyMs = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        /*
        duration_cast<std::chrono::milliseconds>(...) — ép kiểu duration sang milliseconds
        .count() — lấy giá trị số nguyên (ví dụ 890 nghĩa là 890ms)
        latencyMs — kiểu long long
        */

        std::string resultStr = result.value_or("(không có kết quả)");
        observe(resultStr); // observe(resultStr) — push kết quả vào m_history với role "tool" để LLM đọc ở bước tiếp theo
        /*
        Tool chạy
        ↓
        Có kết quả
        ↓
        Ghi kết quả vào lịch sử hội thoại
        ↓
        LLM đọc được ở vòng sau
        */
        if (m_stepHook) m_stepHook(step, thought, action, result, latencyMs);
        /*
        step	Bước thứ mấy
        thought	LLM vừa nghĩ/gửi gì
        action	ToolCallAction, DoneAction hay ErrorAction
        result	Kết quả tool trả về
        latencyMs	Bước này mất bao nhiêu ms
        
        ý nghĩa: 
        "Nếu có ai muốn theo dõi agent thì gửi cho họ thông tin của bước hiện tại."
        */
    }

    return "Đạt giới hạn max_steps (" + std::to_string(maxSteps) + ") mà chưa hoàn thành task.";
}
/*
run() là skeleton của toàn bộ vòng lặp ReAct (Observe→Think→Act).
Nó điều phối theo thứ tự cố định: setup history → lặp tối đa maxSteps lần 
→ mỗi bước gọi think() → parseToolCall() → kiểm tra Done → kiểm tra loop → act()
→ observe() → ghi hook. Không bước nào bị bỏ qua, 
không bước nào đổi thứ tự — đây là lý do run() không virtual. 
Toàn bộ logic "làm gì khi gặp lỗi, làm gì khi xong" đều nằm ở đây.
*/

std::string AgentLoop::think() {
    return m_client.chat(m_history);
}
/*
m_client.chat(m_history) — gửi toàn bộ m_history lên Ollama API, nhận về response text của LLM
Trả thẳng kết quả về cho run() mà không xử lý thêm — việc parse JSON là của parseToolCall()
Hàm này ngắn nhưng virtual — subclass có thể override để thêm retry logic, timeout riêng, hoặc dùng LLM khác
*/


/*
parseToolCall() là bộ chuyển đổi từ chuỗi text của LLM sang Action 
(ToolCallAction / DoneAction / ErrorAction) để run() biết phải làm gì tiếp theo.
*/
Action AgentLoop::parseToolCall(const std::string& llmResponse) {
    m_history.push_back({"assistant", llmResponse});

    static const std::regex jsonBlock(R"(\{[\s\S]*\})"); //  tìm khối JSON từ { đến } đầu tiên trong response
    // static ở đây biến jsonBlock thành một object tồn tại suốt chương trình, thay vì tạo mới mỗi lần hàm được gọi.
    std::smatch match; // object lưu kết quả tìm kiếm regex
    if (!std::regex_search(llmResponse, match, jsonBlock)) {
        return ErrorAction{"Không tìm thấy JSON tool call trong response."};
    }
/*
std::regex_search(
    llmResponse, // chuỗi cần tìm
    match,       // nơi lưu kết quả match
    jsonBlock    // regex
);
trả vê true false

regex: tìm chuỗi có dạng này
*/
    try { // tôi sắp làm việc có thể phát sinh ra lỗi
        auto j = nlohmann::json::parse(match.str()); // thành json để có thể truy cập
        if (j.contains("final_answer")) { // Có key này không?
            return DoneAction{j.at("final_answer").get<std::string>()}; // j.at(...) lấy value và chuyển thành string
        }
        if (j.contains("tool") && j.contains("args")) {
            /*
            {
        "tool":"weather",
        "args":"HCM"
            }        
            */
            auto toolName = j.at("tool").get<std::string>();
            auto args = j.at("args").is_string() // phải string k
                          ? j.at("args").get<std::string>()
                          : j.at("args").dump(); // dump biến json thành chuỗi
                          /*
                          {
                                "tool":"weather",
                                "args":{
                                "city":"HCM",
                                "unit":"C"
                            }
                            }
                            -> "{\"city\":\"HCM\",\"unit\":\"C\"}"
                          */
            return ToolCallAction{toolName, args};
        }
        return ErrorAction{"JSON thiếu field 'tool'/'args' hoặc 'final_answer'."};
    } catch (const nlohmann::json::parse_error& e) {
        return ErrorAction{std::string("Lỗi parse JSON: ") + e.what()};
    }
}

// act() dùng std::visit + if constexpr — xử lý từng loại Action
std::optional<std::string> AgentLoop::act(const Action& action) {
    return std::visit([this](auto&& a) -> std::optional<std::string> {
        // không dc auto&  vì std::visit bên trong tự move giá trị ra khỏi variant rồi truyền vào lambda dưới dạng rvalue (giá trị tạm). 
        // Mà auto& chỉ nhận được lvalue (biến có tên), không nhận được rvalue
        // cần this để lambda biết m_registry của ai, chứ không có thì lỗi vì ko bt m_registry là gì
        using T = std::decay_t<decltype(a)>;
        if constexpr (std::is_same_v<T, ToolCallAction>) {
            // getTool() trả unique_ptr -> tạo instance mới, tự hủy khi ra khỏi scope này
            std::unique_ptr<Tool> tool = m_registry.getTool(a.toolName);
            if (!tool) {
                return "Lỗi: tool '" + a.toolName + "' không tồn tại hoặc bị deny.";
            }
            return tool->execute(a.args);
        } else if constexpr (std::is_same_v<T, ErrorAction>) {
            // ví dụ lỗi compile  // Compiler vẫn cố compile dòng này dù T=ToolCallAction
            // ToolCallAction không có field "message"
            return "Lỗi: " + a.message;
        } else {
            return std::nullopt; // DoneAction xử lý ở run(), không tới đây
        }
    }, action);
}

void AgentLoop::observe(const std::string& result) {
    m_history.push_back({"tool", result});
}
/*
observe() chỉ có một nhiệm vụ: ghi kết quả tool vào m_history để LLM có thể quan sát 
(observe) nó ở vòng lặp kế tiếp. Đây chính là chữ O (Observe) trong vòng lặp Observe → Think → Act.

m_history =

[
    {"user", "Thời tiết HCM"},
    {"assistant", "{\"tool\":\"weather\",\"args\":\"HCM\"}"},
    {"tool", "Nhiệt độ ở HCM là 32°C"}
]
*/