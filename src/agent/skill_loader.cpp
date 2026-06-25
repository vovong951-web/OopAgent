#include "skill_loader.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

SkillLoader::SkillLoader(const std::string& skillsDir)
    : skillsDir_(skillsDir) {}
    // skillsDir_ thư mục đường dẫn chứa skill
    // mục tiêu biến đổi thành giá trị thường
std::string SkillLoader::toLowerCase(const std::string& s) const {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}
/*
std::transform(
    nguồn_bắt_đầu,
    nguồn_kết_thúc,
    nơi_ghi_kết_quả,
    hàm_biến_đổi
);
unsigned char c có thể lỗi nếu giá trị âm
*/
std::string SkillLoader::readFileContent(const fs::path& path) const {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    std::ostringstream oss;
    oss << file.rdbuf();
    // oss << file.rdbuf(),
    // nó đọc hết toàn bộ file một lần, nhanh hơn đọc từng dòng bằng getline.
    return oss.str();
}

std::vector<std::string> SkillLoader::extractKeywordsFromFilename(const std::string& filename) const {
    std::string stem = filename;
    auto dotPos = stem.find(".md");
    if (dotPos != std::string::npos) {
        stem = stem.substr(0, dotPos);
    }

    std::vector<std::string> keywords;
    std::stringstream ss(stem);
    std::string token;
    while (std::getline(ss, token, '_')) {
        if (!token.empty()) {
            keywords.push_back(toLowerCase(token));
        }
    }
    return keywords;
}

std::vector<std::string> SkillLoader::extractKeywordsFromContent(const std::string& content) const {
    std::vector<std::string> keywords;
    std::string marker = "<!-- keywords:";
    auto start = content.find(marker);
    if (start == std::string::npos) {
        return keywords; // không có metadata, trả về rỗng -> sẽ fallback sang filename
    }
    start += marker.length();
    auto end = content.find("-->", start);
    if (end == std::string::npos) {
        return keywords;
    }

    std::string raw = content.substr(start, end - start);
    std::stringstream ss(raw);
    std::string token;
    while (std::getline(ss, token, ',')) {
        size_t first = token.find_first_not_of(" \t\n\r");
        size_t last = token.find_last_not_of(" \t\n\r");
        if (first == std::string::npos) continue;
        token = token.substr(first, last - first + 1);
        if (!token.empty()) {
            keywords.push_back(toLowerCase(token));
        }
    }
    return keywords;
}

void SkillLoader::loadAll() {
    skills_.clear();
    // "Xóa toàn bộ dữ liệu cũ, rồi nạp lại từ đầu."

    if (!fs::exists(skillsDir_) || !fs::is_directory(skillsDir_)) {
        return; // thư mục không tồn tại hoặc không phải thư mục → bỏ qua, không crash
    }

    for (const auto& entry : fs::directory_iterator(skillsDir_)) {
        // fs::directory_iterator là một iterator để duyệt các file/thư mục bên trong một thư mục.
        if (!entry.is_regular_file()) continue;
        // không phải file thường
        if (entry.path().extension() != ".md") continue;
        // entry.path().extension() là cách lấy phần đuôi mở rộng (extension)

        Skill skill;
        skill.name = entry.path().stem().string();
        // path là chuyên để xử lý đường dẫn
        // path.stem() trả về tên file không có phần mở rộng. .string() chuyển từ fs::path sang std::string.
        skill.content = readFileContent(entry.path());

        // Ưu tiên keyword từ metadata trong nội dung, fallback về tên file
        auto contentKeywords = extractKeywordsFromContent(skill.content);
        if (!contentKeywords.empty()) {
            skill.keywords = contentKeywords;
        } else {
            skill.keywords = extractKeywordsFromFilename(entry.path().filename().string());
        }

        skills_.push_back(std::move(skill));
        // Đưa skill vào vector bằng cách chuyển quyền sở hữu dữ liệu thay vì sao chép."
    }
}

std::vector<std::string> SkillLoader::listSkills() const {
    std::vector<std::string> names;
    for (const auto& skill : skills_) {
        names.push_back(skill.name);
    }
    return names;
}

std::optional<Skill> SkillLoader::selectSkill(const std::string& task) const {
    if (skills_.empty()) {
        return std::nullopt;
    }

    std::string taskLower = toLowerCase(task);

    const Skill* bestMatch = nullptr;
    int bestScore = 0;

    for (const auto& skill : skills_) {
        int score = 0;
        for (const auto& keyword : skill.keywords) {
            if (taskLower.find(keyword) != std::string::npos) {
                score++;
            }
        }
        if (score > bestScore) {
            bestScore = score;
            bestMatch = &skill;
        }
    }

    if (bestMatch == nullptr) {
        return std::nullopt;
    }
    return *bestMatch;
}
// Tạo system prompt từ một skill đã có
std::string SkillLoader::buildSystemPrompt(const Skill& skill) const {
    std::ostringstream oss;
    oss << "Ban dang su dung ky nang: " << skill.name << "\n\n";
    oss << skill.content;
    return oss.str();
}
// Từ task → tìm skill phù hợp → tạo system prompt
std::string SkillLoader::buildSystemPromptForTask(const std::string& task) const {
    auto skill = selectSkill(task);
    if (!skill.has_value()) {
        return "";
    }
    return buildSystemPrompt(*skill);
}