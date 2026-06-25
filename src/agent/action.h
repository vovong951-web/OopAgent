#pragma once
#include <string>
#include <variant>
// variant là thư viện C++17 cho phép 1 biến chứa được nhiều kiểu khác nhau
// std::variant<Kieu1, Kieu2, Kieu3, ...> tenBien;
struct ToolCallAction { std::string toolName; std::string args; };
struct DoneAction     { std::string finalAnswer; };
struct ErrorAction    { std::string message; };
using Action = std::variant<ToolCallAction, DoneAction, ErrorAction>;