#include "calculator_tool.h"
#include <stdexcept>
#include <string>

void CalculatorTool::skipSpaces(const std::string& expr, size_t& pos) {
    while (pos < expr.size() && expr[pos] == ' ') pos++;
}

double CalculatorTool::parseNumber(const std::string& expr, size_t& pos) {
    skipSpaces(expr, pos);
    size_t start = pos;
    if (pos < expr.size() && expr[pos] == '-') pos++;
    while (pos < expr.size() && (std::isdigit(expr[pos]) || expr[pos] == '.')) pos++;
    if (pos == start) throw std::runtime_error("Không tìm thấy số tại vị trí " + std::to_string(pos));
    return std::stod(expr.substr(start, pos - start));
}

double CalculatorTool::parseFactor(const std::string& expr, size_t& pos) {
    skipSpaces(expr, pos);
    if (pos < expr.size() && expr[pos] == '(') {
        pos++; // bỏ qua '('
        double result = parseExpression(expr, pos);
        skipSpaces(expr, pos);
        if (pos >= expr.size() || expr[pos] != ')') throw std::runtime_error("Thiếu dấu )");
        pos++; // bỏ qua ')'
        return result;
    }
    return parseNumber(expr, pos);
}

double CalculatorTool::parseTerm(const std::string& expr, size_t& pos) {
    double result = parseFactor(expr, pos);
    while (true) {
        skipSpaces(expr, pos);
        if (pos < expr.size() && expr[pos] == '*') {
            pos++;
            result *= parseFactor(expr, pos);
        } else if (pos < expr.size() && expr[pos] == '/') {
            pos++;
            double divisor = parseFactor(expr, pos);
            if (divisor == 0) throw std::runtime_error("Chia cho 0");
            result /= divisor;
        } else break;
    }
    return result;
}

double CalculatorTool::parseExpression(const std::string& expr, size_t& pos) {
    double result = parseTerm(expr, pos);
    while (true) {
        skipSpaces(expr, pos);
        if (pos < expr.size() && expr[pos] == '+') {
            pos++;
            result += parseTerm(expr, pos);
        } else if (pos < expr.size() && expr[pos] == '-') {
            pos++;
            result -= parseTerm(expr, pos);
        } else break;
    }
    return result;
}

std::optional<std::string> CalculatorTool::execute(const std::string& args) {
    try {
        size_t pos = 0;
        double result = parseExpression(args, pos);
        // Nếu kết quả là số nguyên thì bỏ phần thập phân
        if (result == static_cast<long long>(result)) {
            return std::to_string(static_cast<long long>(result));
        }
        return std::to_string(result);
    } catch (const std::exception& e) {
        return std::nullopt;
    }
}