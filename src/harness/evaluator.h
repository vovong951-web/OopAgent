// src/harness/evaluator.h
#pragma once
#include <string>
#include <optional>
#include "trajectory.h"
#include<print>
// ──────────────────────────────────────────────────────────────────
// Struct EvalResult: kết quả sau khi evaluator chấm một trajectory
// ──────────────────────────────────────────────────────────────────
struct EvalResult {
    bool        passed  = false; // pass hay fail
    double      score   = 0.0;  // điểm partial (0.0 → 1.0), dùng optional vẫn được
    std::string reason;          // giải thích: "found keyword '255'" hay "script exited 0"
};

// ──────────────────────────────────────────────────────────────────
// Task: định nghĩa một task để evaluator biết cần kiểm tra gì
// ──────────────────────────────────────────────────────────────────
struct Task {
    std::string id;           // "task_001"
    std::string description;  // mô tả ngắn
    std::string instruction;  // prompt gửi cho agent
    std::string eval_type;    // "keyword" hoặc "functional"
    std::string eval_config;  // keyword list hoặc shell script
    int         max_steps = 10;
};

// ──────────────────────────────────────────────────────────────────
// Evaluator — abstract base class (Strategy Pattern)
// Mỗi subclass implement một cách evaluate khác nhau
// nhưng HarnessRunner chỉ cần biết interface evaluate()
// ──────────────────────────────────────────────────────────────────
class Evaluator {
public:
    virtual ~Evaluator() = default;

    // Pure virtual: subclass PHẢI implement
    // Nhận vào trajectory đã chạy xong + task definition
    // Trả về EvalResult
    virtual EvalResult evaluate(const Trajectory& traj,
                                const Task&       task) const = 0;

    // Non-virtual helper: in kết quả ra console
    void printResult(const EvalResult& r) const {
        std::cout << "[Eval] " << (r.passed ? "PASS" : "FAIL")
          << " | score=" << r.score
          << " | " << r.reason << "\n";
    }
};