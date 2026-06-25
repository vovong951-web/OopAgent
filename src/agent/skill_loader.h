#pragma once
#include <string>
#include <vector>
#include <optional>
#include <filesystem>  // để dùng std::filesystem (đọc thư mục, file)

// Đại diện cho một skill đã load từ file .md
struct Skill {
    std::string name;        // tên skill, lấy từ tên file (không có .md)
    std::string content;     // nội dung đầy đủ của file .md
    std::vector<std::string> keywords; // từ khóa để match với task
};

// SkillLoader — scan thư mục skills/, chọn skill phù hợp theo keyword matching,
// và build system prompt để inject vào agent trước mỗi lần run.
class SkillLoader {
public:
    explicit SkillLoader(const std::string& skillsDir = "skills");
//   ^ Tránh gọi nhầm constructor/hàm
    // Scan toàn bộ file .md trong thư mục skills/ bằng std::filesystem
    void loadAll();

    // Trả về danh sách tên các skill đã load
    std::vector<std::string> listSkills() const;

    // Chọn skill phù hợp nhất với task dựa trên keyword matching
    std::optional<Skill> selectSkill(const std::string& task) const;

    // Build system prompt hoàn chỉnh từ 1 skill cụ thể
    std::string buildSystemPrompt(const Skill& skill) const;

    // Tiện ích: chọn skill theo task rồi build luôn system prompt, 1 bước
    std::string buildSystemPromptForTask(const std::string& task) const;
    // task → chọn skill → build prompt → trả về
    //gộp selectSkill và buildSystemPrompt lại làm một bước:

private:
    std::filesystem::path skillsDir_; //Đường dẫn đến thư mục skills 
    std::vector<Skill> skills_; //Danh sách tất cả skill đã load vào bộ nhớ

    // Trích keyword từ tên file (fallback nếu nội dung không có metadata)
    std::vector<std::string> extractKeywordsFromFilename(const std::string& filename) const;

    // Trích keyword từ dòng metadata đầu file: <!-- keywords: a, b, c -->
    std::vector<std::string> extractKeywordsFromContent(const std::string& content) const;

    // Đọc toàn bộ nội dung 1 file .md
    std::string readFileContent(const std::filesystem::path& path) const;

    // Chuyển chuỗi về lowercase để so sánh keyword không phân biệt hoa thường
    std::string toLowerCase(const std::string& s) const;
    // Chuyển toàn bộ chữ về viết thường. 
    // Để khi so sánh keyword không bị lỗi vì "Plan" vs "plan" 
    // — cả hai đều thành "plan" trước khi so.

};