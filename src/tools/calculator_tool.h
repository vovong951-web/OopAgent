#pragma once
#include "tool.h"

class CalculatorTool : public Tool {
public:
    std::string name() const override { return "calculator"; }
    std::string description() const override {
        return "Tính biểu thức số học. Ví dụ: 15*17, (3+4)*2";
    }
    std::optional<std::string> execute(const std::string& args) override;

private:
    // Các hàm parse biểu thức
    double parseExpression(const std::string& expr, size_t& pos);
    double parseTerm(const std::string& expr, size_t& pos);
    double parseFactor(const std::string& expr, size_t& pos);
    double parseNumber(const std::string& expr, size_t& pos);
    void skipSpaces(const std::string& expr, size_t& pos);
};