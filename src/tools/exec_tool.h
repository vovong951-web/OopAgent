#pragma once
#include "tool.h"

class ExecTool : public Tool {
public:
    std::string name() const override { return "exec"; }
    std::string description() const override {
        return "Chạy lệnh shell và trả về output. Ví dụ: ls -la, echo hello";
    }
    std::optional<std::string> execute(const std::string& args) override;
};