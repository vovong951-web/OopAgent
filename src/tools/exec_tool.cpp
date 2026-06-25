#include "exec_tool.h"
#include <array>
#include <memory>

std::optional<std::string> ExecTool::execute(const std::string& args) {
    std::array<char, 256> buffer;
    // buffer giúp đọc hết cả file không cần từng dòng
    // fget dùng chuỗi char nên khai báo buffer char
    std::string result;

#ifdef _WIN32
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(args.c_str(), "r"), _pclose);
    /* File là kiểu trả về, decltype là kiểu của hàm xóa nhưng chưa biết, 
    // args.c_str(), "r") là kiểu _popen(args.c_str(), "r")
    // args.c_str() → lệnh cần chạy.
    // "r" → cách mình muốn làm việc với lệnh đó (ở đây là đọc output của nó).
    _pclose là kiểu xóa tránh rò rỉ bộ nhớ
    */
#else
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(args.c_str(), "r"), pclose);
#endif

    if (!pipe) return std::nullopt;

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    /*
    char* fgets(char* str, int count, FILE* stream);
    buffer.data()  trỏ tới đầu chuỗi
    buffer.size()  = 256
    pipe.get()  để lấy dc có dạng FILE*
    popen là tạo đường ống nối với kết quả của một quá trình và có thể lấy đó làm dữ liệu để thực thi lệnh kế

    
    */
    return result;
}