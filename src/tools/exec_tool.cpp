#include "exec_tool.h"
#include <array>
#include <memory>

std::optional<std::string> ExecTool::execute(const std::string& args) {
    std::array<char, 256> buffer;
    std::string result;

#ifdef _WIN32
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(args.c_str(), "r"), _pclose);
#else
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(args.c_str(), "r"), pclose);
#endif

    if (!pipe) return std::nullopt;

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    return result;
}