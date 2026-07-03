#pragma once
#include <string>
#include <optional>
#include <expected>  // C++23

// ToolResult: hoặc trả về string kết quả, hoặc string lỗi (C++23)
using ToolResult = std::expected<std::string, std::string>;

class Tool {
public:
    virtual ~Tool() = default;

    virtual std::string name()        const = 0;
    virtual std::string description() const = 0;

    // Giữ optional<string> để không phải sửa tất cả tool con
    // ToolResult dùng ở các tool mới sau này
    virtual std::optional<std::string> execute(const std::string& args) = 0;
};