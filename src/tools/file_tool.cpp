#include "file_tool.h"
#include <filesystem>
#include <fstream>
#include <sstream>

std::optional<std::string> FileTool::execute(const std::string& args) {
    // Tách lệnh đầu tiên (read, write, append, exists)
    size_t colon = args.find(':');
    if (colon == std::string::npos) return std::nullopt;

    std::string cmd = args.substr(0, colon);
    std::string rest = args.substr(colon + 1);

    if (cmd == "read") {
        std::ifstream file(rest);
        if (!file.is_open()) return std::nullopt;
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    if (cmd == "exists") {
        bool ok = std::filesystem::exists(rest);
        return ok ? std::string("true") : std::string("false");
    }

    if (cmd == "write") {
        // rest = <path>:<content>
        size_t sep = rest.find(':');
        if (sep == std::string::npos) return std::nullopt;
        std::string path = rest.substr(0, sep);
        std::string content = rest.substr(sep + 1);
        std::ofstream file(path);
        if (!file.is_open()) return std::nullopt;
        file << content;
        return std::string("OK");
    }

    if (cmd == "append") {
        // rest = <path>:<content>
        size_t sep = rest.find(':');
        if (sep == std::string::npos) return std::nullopt;
        std::string path = rest.substr(0, sep);
        std::string content = rest.substr(sep + 1);
        std::ofstream file(path, std::ios::app);
        if (!file.is_open()) return std::nullopt;
        file << content;
        return std::string("OK");
    }

    return std::nullopt;
}