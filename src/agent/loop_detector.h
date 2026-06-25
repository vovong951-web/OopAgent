#pragma once
#include "action.h" // cần biết định nghĩa Action
#include <string>
#include <vector>
class LoopDetector {
public:
    enum class Status { Ok, Warning, Critical };
    /*
    enum class định nghĩa 3 mức trạng thái trả về sau khi kiểm tra:
    Ok — bình thường, agent tiếp tục
    Warning — có dấu hiệu lặp, inject cảnh báo vào conversation để LLM tự điều chỉnh
    Critical — lặp nghiêm trọng, dừng agent ngay
    */

    explicit LoopDetector(int warningThreshold = 3, int criticalThreshold = 5);
    // Constructor nhận 2 ngưỡng có giá trị mặc định: sau 3 lần lặp thì Warning, sau 5 lần thì Critical

    Status check(const Action& action);
    void reset();

private:
    std::string actionToKey(const Action& action) const;
    int countGenericRepeat() const;
    bool isPingPong(int window) const;

    std::vector<std::string> m_history;
    int m_warningThreshold;
    int m_criticalThreshold;
};