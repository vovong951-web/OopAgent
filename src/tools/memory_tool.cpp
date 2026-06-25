#include "memory_tool.h"
#include <sstream>
#include <chrono>
#include <stdexcept>

MemoryTool::MemoryTool(const std::string& dbPath) {
    sqlite3* rawDb = nullptr;
    int rc = sqlite3_open(dbPath.c_str(), &rawDb);
    // dbPath.c_str(): đường dẫn file database
    // dùng để mở (hoặc tạo) một database SQLite.
    // &rawDb địa chỉ trả kết quả
    //int sqlite3_open(
    // const char* filename,
    // sqlite3** ppDb

    if (rc != SQLITE_OK) {
        std::string err = rawDb ? sqlite3_errmsg(rawDb) : "unknown error";
        //sqlite3_errmsg() dùng để lấy thông báo lỗi dạng chuỗi từ database SQLite.
        if (rawDb) sqlite3_close(rawDb);
        throw std::runtime_error("MemoryTool: khong the mo SQLite DB - " + err);
    }
    db_.reset(rawDb); // unique_ptr nhận quyền sở hữu, tự đóng khi destruct
    initSchema(); ////Khởi tạo database, tạo bảng nếu chưa có.
}

void MemoryTool::initSchema() {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS memories ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  key TEXT NOT NULL,"
        "  value TEXT NOT NULL,"
        "  timestamp INTEGER NOT NULL"
        ");";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_.get(), sql, nullptr,   nullptr, &errMsg);
    //                                    callback   dữ liệu truyền callback
    // nhận 5 tham số: handle database, câu lệnh SQL, một callback xử lý từng dòng kết quả
    // "Ê SQLite, chạy giúp tôi câu SQL này."
    // sql : Là câu lệnh SQL cần chạy.
    // địa chỉ &errMsg để SQLite ghi thông báo lỗi vào nếu có.
    if (rc != SQLITE_OK ) {
        // SQLITE_OK: hằng số kiểu int
        std::string err = errMsg ? errMsg : "unknown error";
        // chuỗi char* gán cho  string
        sqlite3_free(errMsg);
        throw std::runtime_error("MemoryTool: khong the tao bang - " + err);
    }
}

std::optional<std::string> MemoryTool::save(const std::string& key, const std::string& value) {
    const char* sql = "INSERT INTO memories (key, value, timestamp) VALUES (?, ?, ?);";

    sqlite3_stmt* rawStmt = nullptr;
    /*
    sqlite3_stmt là kiểu dữ liệu của SQLite.
    rawStmt là biến con trỏ trỏ tới nó.
    sqlite3_stmt* là "phiếu thực thi một câu SQL". 
    Nó đại diện cho một câu lệnh SQL cụ thể đang chuẩn bị/chờ được chạy.
    */
    if (sqlite3_prepare_v2(db_.get(), sql, -1, &rawStmt, nullptr) != SQLITE_OK) {
        // Biên dịch chuỗi SQL thành một sqlite3_stmt.
        // -1 : Cho SQLite tự tìm độ dài chuỗi SQL bằng ký tự '\0'.
        // nullptr Không cần lấy phần SQL còn thừa.
        return "Loi: khong the chuan bi cau lenh INSERT - " + std::string(sqlite3_errmsg(db_.get()));
        // Lấy chuỗi lỗi từ SQLite.
    }
    /*
    int sqlite3_prepare_v2(
    sqlite3* db,
    const char* zSql,
    int nByte,
    sqlite3_stmt** ppStmt,
    const char** pzTail
    );
    1. sqlite3* db
    
    Database đang mở.
    2. const char* zSql
    Câu lệnh SQL.
    
    3. int nByte

    Độ dài chuỗi SQL.
    4. sqlite3_stmt** ppStmt

    Nơi SQLite ghi kết quả.
    5. const char** pzTail

    Cho biết SQL còn dư lại sau khi parse.
    int rc = sqlite3_prepare_v2(
    db_.get(),   // database
    sql,         // câu SQL
    -1,          // tự tính độ dài
    &stmt,       // nhận stmt
    nullptr      // không cần SQL còn dư
    */
    SqliteStmtPtr stmt(rawStmt); // RAII, tự finalize khi ra khỏi scope

    auto now = std::chrono::system_clock::now().time_since_epoch(); // Lấy thời gian hiện tại
    // system_clock::now() time hiện tại
    // time_since_epoch() time đã trôi qua
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now).count(); // đổi giây
    //duration_cast<std::chrono::seconds>(now) đổi sang giây
    // .count() lấy nguyên 

    sqlite3_bind_text(stmt.get(), 1, key.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 2, value.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt.get(), 3, timestamp);
    /*
    ## int sqlite3_bind_int64(sqlite3_stmt *stmt, int idx, sqlite3_int64 value);
    sqlite3_bind_int64 là một hàm trong SQLite C API dùng để gán (bind) 
    một giá trị số nguyên 64-bit vào một tham số trong câu lệnh SQL đã được chuẩn bị sẵn (prepared statement).
    sqlite3_bind_text dùng để gán (bind) một giá trị kiểu chuỗi văn bản vào một tham số placeholder (?)
    trong câu lệnh SQL đã được chuẩn bị sẵn (prepared statement) bằng sqlite3_prepare_v2 hoặc tương tự.
    SQLITE_TRANSIENT: cho SQLite biết phải tự sao chép (copy)
    dữ liệu chuỗi vào bộ nhớ riêng của nó ngay lập tức,
    vì dữ liệu gốc (key) có thể bị hủy hoặc thay đổi sau khi hàm này chạy xong.
    Đối lập với nó là SQLITE_STATIC, nghĩa là "dữ liệu này sẽ tồn tại mãi,
    không cần copy" — dùng SQLITE_STATIC sai chỗ dễ gây lỗi truy cập vùng nhớ đã giải phóng (use-after-free).
    */
// 1 2 3 là giá trị cần điền của ?
    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
        //sqlite3_step  Đây là hàm dùng để thực thi prepared statement 
        //int sqlite3_step(sqlite3_stmt *stmt);
        // — tức là chạy câu lệnh SQL (INSERT, UPDATE, DELETE, SELECT...) đã được chuẩn bị trước đó bằng sqlite3_prepare_v2.
        return "Loi: khong the luu memory - " + std::string(sqlite3_errmsg(db_.get()));
        // code sẽ trả về thông báo lỗi lấy từ sqlite3_errmsg(db_.get()).
    }

    return "Da luu memory: " + key + " = " + value;
}

std::optional<std::string> MemoryTool::search(const std::string& keyword) {
    const char* sql = "SELECT key, value, timestamp FROM memories "
                       "WHERE key LIKE ? OR value LIKE ? "
                       "ORDER BY timestamp DESC LIMIT 5;";
                       // lấy tối đa 5 answer

    sqlite3_stmt* rawStmt = nullptr;
    if (sqlite3_prepare_v2(db_.get(), sql, -1, &rawStmt, nullptr) != SQLITE_OK) {
        return "Loi: khong the chuan bi cau lenh SELECT - " + std::string(sqlite3_errmsg(db_.get()));
    }
    SqliteStmtPtr stmt(rawStmt);

    std::string pattern = "%" + keyword + "%";
    // Dấu % trong SQL nghĩa là "bất kỳ chuỗi ký tự nào".
    /*
    LIKE '%abc%'
    = chứa "abc" ở bất kỳ vị trí nào trong chuỗi.
    */
    sqlite3_bind_text(stmt.get(), 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 2, pattern.c_str(), -1, SQLITE_TRANSIENT);
    
    std::ostringstream oss;
    int count = 0;
    while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
        /* row là dữ liệu chi tiết 1 hàng
        Structured bindings không áp dụng trực tiếp cho sqlite3 C API,
        nhưng có thể tách rõ ràng từng field
        SQLITE_ROW: Có nghĩa là "Tôi (SQLite) đã tìm thấy và vừa trả về cho bạn một dòng dữ liệu (row) mới".
        Chừng nào mà SQLite vẫn còn tìm thấy một dòng dữ liệu mới,
        thì hãy tiếp tục thực hiện các câu lệnh bên trong vòng lặp này."
        */ 
        const unsigned char* key = sqlite3_column_text(stmt.get(), 0); // lấy cột 0 : key
        // unsigned char* value được dùng để lưu trữ con trỏ trỏ đến một chuỗi ký tự 
        // hoặc một mảng dữ liệu thô (raw data) mà dữ liệu đó không được phép bị thay đổi.
        const unsigned char* value = sqlite3_column_text(stmt.get(), 1); // lấy cột 1: value
        oss << "- " << key << ": " << value << "\n";
        count++;
    }
    if (count == 0) {
        return "Khong tim thay memory nao khop voi: " + keyword;
    }
    return oss.str();
}

std::optional<std::string> MemoryTool::execute(const std::string& args) {
    // Parse "save:key:value" hoặc "search:keyword"
    auto firstColon = args.find(':');
    if (firstColon == std::string::npos) {
        return "Loi: format sai. Dung 'save:key:value' hoac 'search:keyword'.";
    }

    std::string action = args.substr(0, firstColon);
    std::string rest = args.substr(firstColon + 1);

    if (action == "save") {
        auto secondColon = rest.find(':');
        if (secondColon == std::string::npos) {
            return "Loi: 'save' can format 'save:key:value'.";
        }
        std::string key = rest.substr(0, secondColon);
        std::string value = rest.substr(secondColon + 1);
        return save(key, value);
    } else if (action == "search") {
        return search(rest);
    }

    return "Loi: action khong hop le. Dung 'save' hoac 'search'.";
}