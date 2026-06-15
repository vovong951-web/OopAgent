#pragma once
#include "tool.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include <vector>

class ToolRegistry {
public:
    // Đăng ký tool với factory function
    void registerTool(const std::string& name, std::function<std::unique_ptr<Tool>()> factory);

    // Lấy tool theo tên, trả về nullptr nếu không có hoặc bị deny
    std::unique_ptr<Tool> getTool(const std::string& name);

    // Liệt kê tên tất cả tool đã đăng ký
    std::vector<std::string> listTools() const;

    // Tạo chuỗi mô tả tất cả tool để inject vào system prompt
    std::string buildToolDescriptions();

    // Chặn tool theo tên
    void denyTool(const std::string& name);

private:
    std::unordered_map<std::string, std::function<std::unique_ptr<Tool>()>> m_factories;
    std::vector<std::string> m_denyList;
};