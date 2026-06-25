#pragma once
#include "tool.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include <vector>

class ToolRegistry { // đăng ký tool động  và lấy tool theo tên
public:
    // Đăng ký tool với factory function
    /* std::unique_ptr<Tool> là con trỏ thông minh,
     sở hữu độc quyền một object kiểu Tool (hoặc class con của Tool),
     tự động delete khi ra khỏi scope
     std::function là một "hộp chứa hàm" — nó có thể giữ bất kỳ thứ gì gọi được như hàm
     (function pointer, lambda, functor), miễn là khớp đúng signature 
     (kiểu tham số + kiểu trả về) khai báo trong <...>.
     Cú pháp bên trong <...> luôn viết theo dạng: KieuTraVe(KieuThamSo1, KieuThamSo2, ...).
     Ví dụ std::function<int(double, double)> nghĩa là "một hàm nhận 2 tham số double, trả về int". 
    tạo tool: 
     registry.registerTool("calculator", []() { 
    return std::make_unique<CalculatorTool>(); 
});
với mục đích là tiết kiệm tài nguyên, khi dùng mới tạo
     */
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
    // gọi tên thì cho ra hàm lambda
    std::vector<std::string> m_denyList;
    // danh sách chặn
};