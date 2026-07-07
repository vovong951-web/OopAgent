#include "ollama_client.h"
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <stdexcept>
#include<iostream>
using json = nlohmann::json;
/*
Khi curl tải dữ liệu về:

https://example.com

Server trả về:

Hello World

Curl cần biết:

"Tôi nhận được dữ liệu rồi, giờ ghi nó vào đâu?"

Nó sẽ gọi hàm writeCallback() của bạn.

Mục đích của writeCallback() là:

Nhận dữ liệu mà libcurl tải về và lưu nó vào nơi bạn muốn (ở đây là std::string).
Bạn
 │
 │ URL
 ▼
libcurl
 │
 │ gửi request
 ▼
Server
 │
 │ trả dữ liệu
 ▼
libcurl
 │
 │ gọi writeCallback
 ▼
std::string result

 ## libcurl là một "nhân viên giao hàng chuyên nghiệp".

Bạn chỉ cần nói:

"Tới địa chỉ này lấy dữ liệu về cho tôi."

Nó sẽ tự lo:

Kết nối Internet.
Gửi request.
Nhận response.
Xử lý HTTPS.
Xử lý redirect.
Xử lý timeout.
*/
static size_t writeCallback(void *contents, size_t size, size_t nmemb, std::string *output)
// static có hiệu lực chỉ khi trong hàm
{
    output->append((char *)contents, size * nmemb);
    return size * nmemb;
}
/*
void* contents kiểu con trỏ tới dữ liệu thô
mà libcul vừa nhận nhưng chưa bt kiểu dữ liệu.
size * nmemb số byte của dữ liệu
std::string* output: Con trỏ tới chuỗi mà bạn muốn lưu kết quả.
chuoi.append(
    địa_chỉ_dữ_liệu,
    số_ký_tự_cần_lấy
);
(char*)contents ép kiểu  từ void*
*/

OllamaClient::OllamaClient(const std::string &baseUrl, const std::string &model)
    : m_baseUrl(baseUrl), m_model(model) {}
// kiểu gán

void OllamaClient::setModel(const std::string &model) { m_model = model; }
void OllamaClient::setTemperature(float temp) { m_temperature = temp; }
// Gửi một HTTP POST request và trả về nội dung phản hồi từ server dưới dạng std::string.
std::string OllamaClient::httpPost(const std::string &url, const std::string &body)
{
    CURL *curl = curl_easy_init();
    // tạo curl, sai trả null
    if (!curl)
        throw std::runtime_error("Failed to init curl");

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    /*
    Nói với libcurl:
    "Gửi request tới địa chỉ này url.c_str() ."
    */
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());     // Dữ liệu cần gửi đi.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback); // hãy gọi writeCallback nhưng vẫn chưa biết gửi tới đâu
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    // Khi có dữ liệu trả về:
    // gọi writeCallback(...)
    // và cho nó biết phải ghi vào response

    struct curl_slist *headers = nullptr;                                   // struct curl_slist* kiểu cũ của c++ khi gọi struct
    headers = curl_slist_append(headers, "Content-Type: application/json"); // thêm các e header với nội dung
    headers = curl_slist_append(headers, "ngrok-skip-browser-warning: true"); // ← THÊM
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    /*
    curl_easy_setopt(
    curl,
    option,
    value
    );
    Này curl,
    khi gửi request,
    hãy dùng danh sách header này.
    */

    CURLcode res = curl_easy_perform(curl); // Gửi request
    /*
    curl_easy_perform

    Nó sẽ:

    Kết nối server
    ↓
    Gửi HTTP request
    ↓
    Gửi headers
    ↓
    Gửi body
    ↓
    Nhận response
    ↓
    Gọi callback
    ↓
    Hoàn tất

    CURLcode : Là enum báo lỗi.
    */
    curl_slist_free_all(headers); // giai phong
    curl_easy_cleanup(curl);      // Hủy curl session

    if (res != CURLE_OK)

        /*
        CURLE_OK
        CURLE_COULDNT_CONNECT
        CURLE_OPERATION_TIMEDOUT
        ...
        */
        throw std::runtime_error(curl_easy_strerror(res));  // curl_easy_strerror(res) chuyển mã lỗi res thành chuỗi đọc
    

    return response;
}
// Nhận vào lịch sử hội thoại, trả về câu trả lời của AI.
std::string OllamaClient::chat(const std::vector<Message> &messages)
{
    json payload; // Tạo payload JSON, ban đầu {}
    payload["model"] = m_model;
    payload["temperature"] = m_temperature;
    payload["stream"] = false; // stream: false → nhận toàn bộ response 1 lần, không nhận từng chữ
    // Chuyển vector<Message> sang JSON array:
    /*
    {
  "model":"llama3",
  "temperature":0.7,
  "stream":false
}
    */
    json msgs = json::array(); //Tạo mảng messages kiểu tin json rỗng []
    for (const auto &msg : messages)
    {
        msgs.push_back({{"role", msg.role}, {"content", msg.content}}); 
        /*
        json j = {
        {"key1", value1},
        {"key2", value2}
};
        */
    }
    payload["messages"] = msgs; //tạo hoặc truy cập field tên "messages" trong JSON object
    /*
    sau khi lệnh 
    {
    "messages": [...]
    }
    */

    std::string response = httpPost(m_baseUrl + "/api/chat", payload.dump());
    /*
    payload.dump() → chuyển object json thành string JSON
    httpPost(...) → gửi request lên server
    response → là chuỗi JSON trả về từ server
    */

    json result = json::parse(response);                    // biến đổi để có thể tác động cụ thể bên trong
    /*
    string → JSON object (để truy cập theo key)
    response lúc này chỉ là string bình thường
    json::parse(...) biến nó thành object JSON để bạn truy cập kiểu:
    result["message"]
    result["message"]["content"]
    */
    return result["message"]["content"].get<std::string>(); // lấy hàng đổi thành string
    /*
    result["message"] → lấy object message
    ["content"] → lấy field content
    .get<std::string>() → ép kiểu JSON value → string C++
    Field = “trường dữ liệu” trong JSON
    vd:
    {
    "message": {
    "content": "Hello world"
    }
    }
    "message" → field cấp 1
    "content" → field con bên trong message
    field	1 “ô dữ liệu” trong JSON
    object	nhóm các field
    key	tên field
    */
}