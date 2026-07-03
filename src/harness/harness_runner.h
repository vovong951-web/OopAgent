#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <fstream>
#include <chrono>
#include <iostream>       // std::cout
#include <filesystem>     // std::filesystem::create_directories
#include <ranges>         // C++20
#include <nlohmann/json.hpp>
#include "trajectory.h"   // đã include step.h bên trong
#include "evaluator.h"
#include "../agent/agent_loop.h"

class HarnessRunner {
public:
    using StepHook = std::function<void(const Step&)>;

    explicit HarnessRunner(std::unique_ptr<Evaluator> evaluator)
        : evaluator_(std::move(evaluator))
    {}

    Trajectory run(AgentLoop& agent, const Task& task) {
        setupEnvironment(task);

        Trajectory traj;
        traj.task_id = task.id;
        traj.model   = agent.getModelName();

        StepHook hook = [&traj](const Step& step) {
            traj.addStep(step);
        };

        agent.setStepHook(hook);

        auto start = std::chrono::high_resolution_clock::now();
        std::string final_answer = agent.run(task.instruction, task.max_steps);
        auto end = std::chrono::high_resolution_clock::now();

        traj.total_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>
                             (end - start).count();

        EvalResult result = evaluator_->evaluate(traj, task);
        traj.success = result.passed;
        evaluator_->printResult(result);
        traj.exportToFile("benchmark");

        return traj;
    }

    void runBatch(AgentLoop& agent, const std::string& tasks_json_path) {
        std::ifstream ifs(tasks_json_path);
        if (!ifs.is_open()) {
            throw std::runtime_error("Cannot open: " + tasks_json_path);
        }

        nlohmann::json json_tasks;
        ifs >> json_tasks;

        std::vector<Task> tasks;
        for (const auto& jt : json_tasks) {
            Task t;
            t.id          = jt["id"].get<std::string>();
            t.description = jt["description"].get<std::string>();
            t.instruction = jt["instruction"].get<std::string>();
            t.eval_type   = jt["eval_type"].get<std::string>();
            t.eval_config = jt.value("eval_config", "");
            t.max_steps   = jt.value("max_steps", 10);
            tasks.push_back(std::move(t));
        }

        int pass_count = 0;
        std::vector<nlohmann::json> batch_results;

        for (const auto& task : tasks) {
            std::cout << "--- Running task: " << task.id << " ---\n";
            Trajectory traj = run(agent, task);

            if (traj.success) ++pass_count;

            batch_results.push_back({
                {"task_id",       task.id},
                {"success",       traj.success},
                {"total_steps",   static_cast<int>(traj.steps.size())},
                {"total_time_ms", traj.total_time_ms}
            });
        }

        double rate = tasks.empty() ? 0.0
                    : static_cast<double>(pass_count) / tasks.size() * 100.0;

        std::cout << "\n== BATCH RESULT ==\n";
        std::cout << "Pass: " << pass_count << "/" << tasks.size()
                  << " (" << rate << "%)\n";

        exportBatchSummary(batch_results, pass_count, tasks.size(), rate);
    }

private:
    std::unique_ptr<Evaluator> evaluator_;

    void setupEnvironment(const Task& task) {
        std::filesystem::create_directories("benchmark");
        // ← thay std::println bằng std::cout
        std::cout << "[Harness] Setup environment for task: " << task.id << "\n";
    }

    void exportBatchSummary(const std::vector<nlohmann::json>& results,
                             int pass, int total, double rate) {
        nlohmann::json summary = {
            {"total_tasks",  total},
            {"passed",       pass},
            {"failed",       total - pass},
            {"success_rate", rate},
            {"tasks",        results}
        };

        std::ofstream ofs("benchmark/batch_summary.json");
        ofs << summary.dump(2);
        // ← thay std::println bằng std::cout
        std::cout << "[Harness] Exported benchmark/batch_summary.json\n";
    }
};