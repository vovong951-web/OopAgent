#pragma once
#include "tool.h"

class FileTool : public Tool {
public:
    std::string name() const override { return "file"; }
    std::string description() const override {
        return "Đọc/ghi file. Dùng: read:<path>, write:<path>:<content>, append:<path>:<content>, exists:<path>";
    }
    std::optional<std::string> execute(const std::string& args) override;
};