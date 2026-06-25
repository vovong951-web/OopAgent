#pragma once
#include "tool.h"

class CalculatorTool : public Tool { // thừa hưởng tool 
public:
// override dùng để tránh lỗi hàm con viết sai tên khai báo khác trong hàm cha
    std::string name() const override { return "calculator"; }
    std::string description() const override {
        return "Tính biểu thức số học. Ví dụ: 15*17, (3+4)*2";
    }
    std::optional<std::string> execute(const std::string& args) override;
    // agrs : 12 x 4, .... kiểu câu hỏi thực thi lệnh 

private:
    // Các hàm parse biểu thức
    //"Parse biểu thức" nghĩa là phân tích một chuỗi biểu thức thành 
    // cấu trúc mà chương trình hiểu được.
    // size_t là kiểu trả về không âm 
    
    double parseExpression(const std::string& expr, size_t& pos);   // cộng trừ
    double parseTerm(const std::string& expr, size_t& pos);   // nhân chia
    double parseFactor(const std::string& expr, size_t& pos);   // liên quan tính toán trong ngoặc
    double parseNumber(const std::string& expr, size_t& pos);   // chuyển đổi số
    void skipSpaces(const std::string& expr, size_t& pos); // bỏ khoảng trắng 
};