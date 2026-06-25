#pragma once
#include <string>
#include <optional>

class Tool {
public:
    virtual ~Tool() = default;
    // nếu một hàm trong base class không có virtual, 
    // thì khi bạn gọi hàm đó qua con trỏ kiểu base 
    // (dù con trỏ đang trỏ tới object của class con), 
    // C++ luôn gọi bản trong base
    // ~Tool() là destructor (hàm hủy) của class Tool
    virtual std::string name() const = 0;  //tên tool
    virtual std::string description() const = 0; // mô tả
    virtual std::optional<std::string> execute(const std::string& args) = 0; 
    //  nhận tham số đầu vào từ LLM (dạng string,
    // ví dụ "15*17" hay "result.txt"), xử lý, rồi trả về kết quả.
    // optional   trả về string hoặc nullopt(không trả về gì hết)
};