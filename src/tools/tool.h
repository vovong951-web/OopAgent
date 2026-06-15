#pragma once
#include <string>
#include <optional>

class Tool {
public:
    virtual ~Tool() = default;
    virtual std::string name() const = 0;
    virtual std::string description() const = 0;
    virtual std::optional<std::string> execute(const std::string& args) = 0;
};