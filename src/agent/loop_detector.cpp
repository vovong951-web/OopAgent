#include "loop_detector.h"
#include <string>
#include <vector>
LoopDetector::LoopDetector(int warningThreshold, int criticalThreshold)
    : m_warningThreshold(warningThreshold), m_criticalThreshold(criticalThreshold) {}
// Nó nhận một Action và biến nó thành một chuỗi (key) để so sánh xem hành động có bị lặp lại hay không.
std::string LoopDetector::actionToKey(const Action& action) const {
    return std::visit([](auto&& a) -> std::string {
        // -> std::string  tất cả phải ra string
        // std::visit(HAM_XU_LY, variant);
        /*
        T&  &  -> T&
        T&  && -> T&
        T&& &  -> T&
        T&& && -> T&& và không có & thì giữ nguyên
        n
        rvalue là tín hiệu báo cho compiler biết "object này sắp chết,
        phải dùng auto&& là vì  chỉ & ko thì ko lấy dc dạng rvalue & lấy dc cả lvalue và rvalue. rvalue như là hàm move().
        nếu là rvalue sẽ ra T&&, lvalue là T&.
         */
        using T = std::decay_t<decltype(a)>;
        //lấy kiểu và bỏ &
        // Tức là compiler nhìn điều kiện trước, chọn nhánh đúng rồi mới biên dịch tiếp. Nhánh sai coi như chưa từng tồn tại
        // tránh lỗi compile
        if constexpr (std::is_same_v<T, ToolCallAction>) {
            // so sánh trả true false
            return a.toolName + ":" + a.args;
        } else if constexpr (std::is_same_v<T, DoneAction>) {
            return "DONE";
        } else {
            return "ERROR";
        }
    }, action);
}

int LoopDetector::countGenericRepeat() const {
    if (m_history.empty()) return 0;
    const auto& last = m_history.back();
    // lấy phần tử cuối
    int count = 0;
    for (auto it = m_history.rbegin(); it != m_history.rend() && *it == last; ++it) ++count;
    // r.begin là xét từ cuối về đầu 
    return count;
}

bool LoopDetector::isPingPong(int window) const {
    if (static_cast<int>(m_history.size()) < window) return false;
    // window như là sô lần tổi thiểu history để mới cần xét repeat
    int n = static_cast<int>(m_history.size());
    const auto& a = m_history[n - 1];
    const auto& b = m_history[n - 2];
    if (a == b) return false; // generic repeat lo riêng rồi
    // Nếu 2 action cuối giống nhau thì đây là generic repeat (lặp cùng 1 action), không phải ping-pong (A→B→A→B)
    for (int i = 0; i < window; ++i) {
        const auto& expected = (i % 2 == 0) ? a : b;
        if (m_history[n - 1 - i] != expected) return false;
        // xét để xem sự xen kẻ để xét ping pong 
    }
    return true;
}
/*
check() là cổng vào duy nhất của LoopDetector từ bên ngoài.
Mỗi lần AgentLoop thực hiện 1 action, nó gọi check() để hỏi:
"Action này có gây ra loop không?"
*/
LoopDetector::Status LoopDetector::check(const Action& action) {
    //        ^ trả về             ^ hàm
    m_history.push_back(actionToKey(action));

    int repeat = countGenericRepeat();
    bool pingPong = isPingPong(4);

    if (repeat >= m_criticalThreshold || (pingPong && m_history.size() >= 6)) {
        return Status::Critical;
    }
    if (repeat >= m_warningThreshold || pingPong) {
        return Status::Warning;
    }
    return Status::Ok;
}

void LoopDetector::reset() {
    m_history.clear();
}       