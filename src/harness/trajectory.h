// src/harness/trajectory.h
#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <fstream>   // std::ofstream
#include <string>
#include "step.h"  // dùng Step từ file riêng, bỏ định nghĩa Step trong trajectory.h
// ────────────────────────────────────────────
// Struct Step: lưu một bước trong agent loop
// ────────────────────────────────────────────
// ────────────────────────────────────────────────────────────
// Struct Trajectory: toàn bộ một lần agent chạy một task
// ────────────────────────────────────────────────────────────
struct Trajectory {
    std::string        task_id;         // id của task (vd: "task_001")
    std::string        model;           // tên model đang dùng (vd: "gemma4")
    bool               success = false; // agent có hoàn thành task không
    int                total_tokens = 0;  // tổng token cả lần chạy
    long long          total_time_ms = 0; // tổng thời gian (ms)
    std::vector<Step>  steps;           // danh sách từng bước

    // Thêm một bước mới vào trajectory
    void addStep(Step s) {
        total_tokens  += s.tokens_used; // cộng dồn token
        total_time_ms += s.latency_ms;  // cộng dồn thời gian
        steps.push_back(std::move(s));  // move để tránh copy
    }

    // Serialize toàn bộ trajectory ra JSON
    nlohmann::json toJson() const {
        nlohmann::json j;
        j["task_id"]       = task_id;
        j["model"]         = model;
        j["success"]       = success;
        j["total_tokens"]  = total_tokens;
        j["total_time_ms"] = total_time_ms;

        // Duyệt từng Step, gọi toJson() rồi push vào array
        j["steps"] = nlohmann::json::array();
        for (const auto& step : steps) {
            j["steps"].push_back(step.toJson());
        }
        return j;
    }

    // Ghi file trajectory_{task_id}.json ra đĩa
    void exportToFile(const std::string& dir = "benchmark") const {
        std::string filename = dir + "/trajectory_" + task_id + ".json";
        std::ofstream ofs(filename);
        // dump(2) = pretty print với indent 2 spaces
        ofs << toJson().dump(2);
    }
};