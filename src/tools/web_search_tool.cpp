#include "web_search_tool.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>

using json = nlohmann::json;

namespace {  // giống static 
    /*
    Đặt trong anonymous namespace giúp hàm này chỉ nhìn thấy được trong file .cpp hiện tại,
     tránh xung đột tên nếu file khác cũng có hàm writeCallback riêng 
     — tương tự hiệu ứng của static ở phạm vi file trong C.
    */
    // Callback ghi response body từ libcurl vào std::string (giống ollama_client.cpp)
    size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t total = size * nmemb;
        static_cast<std::string*>(userp)->append(static_cast<char*>(contents), total);
        // string có thể append nhiều kiểu
        // append nhận (const char* s, size_t count) 
        // static_cast: ép kiểu userp thanh string
        return total;
        /*
        Đây cũng là quy định bắt buộc của libcurl: 
        callback phải trả về đúng số byte đã "xử lý". Nếu trả về số nhỏ hơn total,
         libcurl sẽ hiểu là có lỗi và dừng request giữa chừng.
        */
    }
}

std::string WebSearchTool::urlEncode(const std::string& value) {
    CURL* curl = curl_easy_init();
    if (!curl) return value;
    char* encoded = curl_easy_escape(curl, value.c_str(), static_cast<int>(value.length()));
    /*
    char* curl_easy_escape(
    CURL* handle, // yêu cần là CURL
    const char* string,
    int length  // phải là kiểu int
    );
    mục đích : chuyển ký tự đặc biệt thành dạng %XX
    value.c_str(): C ko hiểu string
    Hàm này tự cấp phát bộ nhớ mới (bằng malloc bên trong libcurl) để chứa kết quả,
    rồi trả về con trỏ char* tới vùng nhớ đó.
    */
    std::string result = encoded ? encoded : value; // khác nullptr
    if (encoded) curl_free(encoded);
    curl_easy_cleanup(curl);
    return result;
}

std::optional<std::string> WebSearchTool::httpGet(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) return std::nullopt;

    std::string responseBody;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    /*
    Nói với libcurl:
    "Gửi request tới địa chỉ này url.c_str() ."
    */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
   //                      ^^^^^^^^^^^^^^^^^^^^^  ^^^^^^^^^^^^
   //                      "tao sắp nhận           con trỏ hàm
   //                       1 function pointer"     thực tế    /*
    // Hai dòng này nói với libcurl: "Mỗi khi nhận được dữ liệu, 
    // hãy gọi hàm writeCallback, 
    // và truyền con trỏ tới responseString vào tham số cuối."
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);  // "đây là DỮ LIỆU sẽ đưa cho hàm đó"
    /*
    biến mà cuối cùng sẽ chứa toàn bộ nội dung response sau khi request hoàn tất
    — &responseBody được ép thành void* 
    rồi truyền vào userp trong writeCallback như đã phân tích.
    */
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L); 
    // Thiết lập thời gian chờ tối đa cho toàn bộ request là 10 giây. 
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); 
    // Cho phép libcurl tự động đi theo HTTP redirect (chuyển hướng).
    // Không có FOLLOWLOCATION:
    // request tới "http://example.com" → server trả 301 redirect tới "https://example.com"
    // → libcurl DỪNG LẠI ở đây, responseBody sẽ chứa nội dung trang redirect (thường rỗng hoặc thông báo ngắn)
    // Có FOLLOWLOCATION (1L = bật):
    // request tới "http://example.com" → nhận 301 → tự động request tiếp "https://example.com"
    // → responseBody chứa đúng nội dung trang đích cuối cùng

    /*
    tự động đi theo redirect — khi server bảo "hãy sang địa chỉ khác",
    libcurl tự làm tiếp luôn (gửi request mới tới địa chỉ đó)
    thay vì dừng lại và trả về cho bạn nội dung của trang chuyển hướng 
    (thường rỗng hoặc không phải nội dung bạn cần).
    */

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        return std::nullopt; // timeout, connection refused, v.v.
    }
    return responseBody;
}

std::optional<std::string> WebSearchTool::execute(const std::string& args) {
    if (args.empty()) {
        return "Loi: can truyen tu khoa tim kiem.";
    }

    std::string url = "https://api.duckduckgo.com/?q=" + urlEncode(args)
                     + "&format=json&no_html=1&skip_disambig=1";
/*
format=json — yêu cầu API trả về dữ liệu dạng JSON (mặc định DuckDuckGo có thể trả XML).
no_html=1 — yêu cầu loại bỏ thẻ HTML khỏi nội dung trả về, chỉ lấy text thuần.
skip_disambig=1 — bỏ qua trang "định hướng" (disambiguation) khi từ khóa có nhiều nghĩa 
(ví dụ tìm "Apple" có thể là công ty hoặc trái cây
    — tham số này yêu cầu API không trả về danh sách lựa chọn mà cố gắng cho thẳng một câu trả lời).
*/

    auto rawResponse = httpGet(url);
    if (!rawResponse.has_value()) {
        return "Loi: khong the ket noi DuckDuckGo API (timeout hoac connection refused).";
    }

    try {
        json data = json::parse(*rawResponse);
        /*
        chuyển chuỗi text JSON thô thành một cấu trúc dữ liệu cây mà bạn có thể truy cập
         như một object/array thông thường.
        */

        // Structured bindings + ưu tiên Abstract trước
        if (data.contains("AbstractText") && !data["AbstractText"].get<std::string>().empty()) {
            // data.contains("AbstractText")  có tồn tại không, kiểm tra xem JSON trả về có field này không
            /*
            DuckDuckGo API thường trả AbstractText rỗng ("")
            khi không tìm được câu trả lời tóm tắt rõ ràng cho từ khóa, 
            nên chỉ kiểm tra contains thôi chưa đủ.
            */
            return data["AbstractText"].get<std::string>();
        }

        // Fallback: lấy RelatedTopics đầu tiên có Text
        /*
        Nếu không có AbstractText hợp lệ, thử tìm trong RelatedTopics 
        — đây là một mảng JSON chứa các chủ đề liên quan mà DuckDuckGo gợi ý.
        */
        if (data.contains("RelatedTopics") && data["RelatedTopics"].is_array()) {
            /*
            kiểm tra đúng kiểu là array trước khi lặp qua, tránh lỗi nếu field này tồn tại 
            nhưng lại có kiểu khác (phòng trường hợp API trả về cấu trúc bất thường).
            */
            for (const auto& topic : data["RelatedTopics"]) {
                // mỗi topic là một JSON object con
                if (topic.contains("Text") && !topic["Text"].get<std::string>().empty()) {
                    return topic["Text"].get<std::string>();
                }
            }
        }

        return "Khong tim thay ket qua phu hop cho: " + args;

    } catch (const json::parse_error& e) { // json ko hợp lệ
        return std::string("Loi: malformed JSON response - ") + e.what();
    }
    // catch bắt lỗi trong try
}