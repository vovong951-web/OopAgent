#pragma once
#include "tool.h"

class FileTool : public Tool {
public:
// Trong file_tool.h, sửa hàm description():
std::string name() const override { return "file"; }
std::string description() const override {
return "file: doc/ghi file. Args: \"write:ten_file.txt:noi_dung\" hoac \"read:ten_file.txt\"";
}
std::optional<std::string> execute(const std::string& args) override;
};