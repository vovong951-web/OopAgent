#pragma once
#include "tool.h"
#include <string>
#include <optional>
#include <memory>
#include <sqlite3.h>

// MemoryTool — luu/truy van memory qua SQLite.
// Args format: "save:key:value" hoac "search:keyword"
// Bang: memories(id INTEGER PK, key TEXT, value TEXT, timestamp INTEGER)
class MemoryTool : public Tool {
public:
    explicit MemoryTool(const std::string& dbPath = "agent_memory.db");
    // explicit  là để bạn phải tự viết rõ việc tạo object, chứ viết gọn
    // máy tính hiểu nhưng ta và đọc giả có thể nhầm lẫn, lỗi ngầm
    std::string name() const override { return "memory"; }
    std::string description() const override {
        return "Luu va truy van bo nho lau dai qua SQLite. "
               "Args: 'save:<key>:<value>' de luu, 'search:<keyword>' de tim. "
               "Vi du: 'save:user_name:Vong' hoac 'search:user_name'.";
    }

    std::optional<std::string> execute(const std::string& args) override;

private:
    // Custom deleter cho sqlite3* — dùng với unique_ptr để tự động sqlite3_close
    struct SqliteCloser {
        void operator()(sqlite3* db) const {
            if (db) sqlite3_close(db);
        }
        /*
        void operator() để có thể SqliteCloser(db)
        sqlite3* sẽ trỏ tới một vùng nhớ do thư viện SQLite tạo ra.
        Bên trong vùng nhớ đó chứa rất nhiều thông tin như:
        File database nào đang mở.
        Các bảng, transaction hiện tại.
        Trạng thái kết nối.
        Cache của SQLite.
        ...
        */
    };
    // nạp chồng toán tử () - (deleter) dành cho sqlite3*.
    using SqliteDbPtr = std::unique_ptr<sqlite3, SqliteCloser>;

    // Custom deleter cho sqlite3_stmt* — tự động sqlite3_finalize
    struct StmtFinalizer {
        void operator()(sqlite3_stmt* stmt) const {
            if (stmt) sqlite3_finalize(stmt);
        }
        /*
        Nếu sqlite3* là tay cầm của database, 
        thì sqlite3_stmt* là tay cầm của một câu lệnh SQL cụ thể.
        sqlite3_stmt* là "phiếu thực thi một câu SQL". 
        Nó đại diện cho một câu lệnh SQL cụ thể đang chuẩn bị/chờ được chạy.
        */
    };
    using SqliteStmtPtr = std::unique_ptr<sqlite3_stmt, StmtFinalizer>;

    SqliteDbPtr db_;

    void initSchema(); //Khởi tạo database, tạo bảng nếu chưa có.
    std::optional<std::string> save(const std::string& key, const std::string& value);
    //Có thể dùng để lưu: vào database
    std::optional<std::string> search(const std::string& keyword);
    //Tìm dữ liệu
};