#include "tool_registry.h"
#include <algorithm>

void ToolRegistry::registerTool(const std::string& name, std::function<std::unique_ptr<Tool>()> factory) {
    m_factories[name] = factory;
}
//thêm lamda vào map đã tạo bên private
std::unique_ptr<Tool> ToolRegistry::getTool(const std::string& name) {
    // Kiểm tra xem tool có bị deny không
    if (std::find(m_denyList.begin(), m_denyList.end(), name) != m_denyList.end()) {
        return nullptr;
    }

    // Kiểm tra xem tool có tồn tại không
    auto it = m_factories.find(name);
    if (it == m_factories.end()) {
        return nullptr;
    }

    // Gọi factory để tạo tool instance
    return it->second();
    // it trả về con trỏ 
}

std::vector<std::string> ToolRegistry::listTools() const {
    std::vector<std::string> names;
    for (const auto& [name, factory] : m_factories) {  // cách dùng c+ 17
        names.push_back(name);
    }
    return names;
}

std::string ToolRegistry::buildToolDescriptions() {
    std::string result;
    for (const auto& [name, factory] : m_factories) {
        auto tool = factory();
        result += "- " + tool->name() + ": " + tool->description() + "\n";
    }
    return result;
}

void ToolRegistry::denyTool(const std::string& name) {
    m_denyList.push_back(name);
}