// src/harness/functional_evaluator.h
#pragma once
#include "evaluator.h"
#include <cstdio>    // popen, pclose
#include <array>     // std::array cho buffer

class FunctionalEvaluator : public Evaluator {
public:
    EvalResult evaluate(const Trajectory& traj,
                        const Task&       task) const override
    {
        // eval_config chứa shell script, vd:
        // "test -f result.txt && grep 255 result.txt && echo PASS"
        const std::string& script = task.eval_config;

        if (script.empty()) {
            return {false, 0.0, "no eval_script provided"};
        }

        // Chạy script và capture output + exit code
        // popen() mở một subprocess, đọc stdout của nó
        std::array<char, 256> buffer{};  // buffer đọc output script
        std::string           output;

        // Mở subprocess: script + " 2>&1" để merge stderr vào stdout
        FILE* pipe = popen((script + " 2>&1").c_str(), "r");
        if (!pipe) {
            return {false, 0.0, "popen failed: cannot run eval script"};
        }

        // Đọc từng chunk 256 byte cho đến khi hết
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            output += buffer.data();
        }

        // pclose trả về exit status của subprocess
        // WEXITSTATUS() lấy exit code thực sự từ status word
        int status    = pclose(pipe);
        // Xoá dòng 39, thay bằng:
        #ifdef _WIN32
            int exit_code = status;
        #else
            int exit_code = WEXITSTATUS(status);
        #endif

        // Exit code 0 = script thành công = PASS
        bool passed = (exit_code == 0);

        std::string reason = passed
            ? "script exited 0, output: " + output
            : "script exited "  + std::to_string(exit_code)
              + ", output: "    + output;

        return {passed, passed ? 1.0 : 0.0, reason};
    }
};