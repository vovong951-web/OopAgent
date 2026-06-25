#pragma once

#include "tool.h"
#include <string>
#include <optional>

// WebSearchTool — gọi DuckDuckGo Instant Answer API qua libcurl.
// GET https://api.duckduckgo.com/?q=...&format=json
// Lưu ý: đây không phải search engine đầy đủ, chỉ trả "instant answer"
// (định nghĩa, infobox tóm tắt). Nếu không có abstract, fallback sang
// RelatedTopics đầu tiên. Có thể trả về optional rỗng nếu DDG không có gì.
class WebSearchTool : public Tool {
public:
    WebSearchTool() = default;

    std::string name() const override { return "web_search"; }
    std::string description() const override {
        return "Tim kiem thong tin tren web qua DuckDuckGo Instant Answer API. "
               "Args: chuoi truy van tim kiem (vi du: 'C++17 std::optional'). "
               "Tra ve doan tom tat (abstract) neu co.";
    }

    std::optional<std::string> execute(const std::string& args) override;

private:
    //  xây dựng URL an toàn
    std::optional<std::string> httpGet(const std::string& url);
    /*
    Hàm này gửi một HTTP GET request tới địa chỉ url 
    và trả về nội dung phản hồi (response body) dưới dạng chuỗi.
    trả về string hoặc nullopt
    */
    std::string urlEncode(const std::string& value);
    /*
    Hàm này mã hóa một chuỗi để có thể đưa an toàn vào URL 
    (URL encoding / percent-encoding).
     Một số ký tự đặc biệt không được phép xuất hiện trực tiếp trong URL
    (dấu cách, &, =, ?, ký tự Unicode...),
    nên cần chuyển chúng thành dạng %XX (mã hex).

    vd
    std::string query = "C++ là gì?";
    std::string encoded = urlEncode(query);
    // kết quả khoảng: "C%2B%2B%20l%C3%A0%20g%C3%AC%3F"
    std::string url = "https://example.com/search?q=" + encoded;
    NẾU KHÔNG CÓ ENCODE:
    không encode, request sẽ bị lỗi hoặc hiểu sai tham số. 
    Ví dụ nếu không encode dấu & trong giá trị tìm kiếm,
    server có thể hiểu nhầm đó là một tham số mới thay vì một phần của giá trị.
    */
};