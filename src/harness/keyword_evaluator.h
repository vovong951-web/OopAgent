// src/harness/keyword_evaluator.h
#pragma once
#include "evaluator.h"
#include <sstream>
#include <algorithm>

class KeywordEvaluator : public Evaluator {
public:
    // evaluate: kiểm tra xem output của agent có chứa các keyword kỳ vọng không
    EvalResult evaluate(const Trajectory& traj,
                        const Task&       task) const override
    {
        // Lấy tool_result của bước cuối cùng làm "output" để check
        // Nếu không có bước nào → fail ngay
        if (traj.steps.empty()) {
            return {false, 0.0, "no steps recorded"};
        }

        // Ghép tất cả tool_result + thought lại thành một chuỗi lớn
        // để tìm keyword ở bất kỳ bước nào
        std::string full_output;
        for (const auto& step : traj.steps) {
            full_output += step.thought + " " + step.tool_result + " ";
        }

        // Chuyển full_output về lowercase để so sánh case-insensitive
        std::string lower_output = full_output;
        std::transform(lower_output.begin(), lower_output.end(),
                       lower_output.begin(), ::tolower);

        // Parse eval_config: mỗi keyword cách nhau bởi dấu phẩy
        // vd: eval_config = "255,result.txt"
        std::vector<std::string> keywords;
        std::stringstream ss(task.eval_config);
        std::string token;
        while (std::getline(ss, token, ',')) {
            // Trim whitespace đầu/cuối mỗi keyword
            auto start = token.find_first_not_of(" \t");
            auto end   = token.find_last_not_of(" \t");
            if (start != std::string::npos) {
                keywords.push_back(token.substr(start, end - start + 1));
            }
        }

        // Đếm bao nhiêu keyword tìm thấy
        int found = 0;
        std::string missing_list;
        for (const auto& kw : keywords) {
            std::string lower_kw = kw;
            std::transform(lower_kw.begin(), lower_kw.end(),
                           lower_kw.begin(), ::tolower);

            if (lower_output.find(lower_kw) != std::string::npos) {
                ++found; // tìm thấy keyword này
            } else {
                missing_list += kw + " "; // ghi lại cái nào thiếu
            }
        }

        // Tính partial score: tỷ lệ keyword tìm thấy
        double score = keywords.empty() ? 1.0
                     : static_cast<double>(found) / keywords.size();
        bool   passed = (found == static_cast<int>(keywords.size()));

        std::string reason = passed
            ? "all keywords found"
            : "missing: " + missing_list;

        return {passed, score, reason};
    }
};