#include "common/utilities/string_utils.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <ctime>

namespace common {
namespace utilities {

// ============ StringUtils 实现 ============

std::vector<std::string> StringUtils::Split(const std::string& str, 
                                           const std::string& delimiter,
                                           bool skip_empty) const {
    std::vector<std::string> result;
    if (str.empty()) return result;
    
    size_t start = 0;
    size_t end = str.find(delimiter);
    
    while (end != std::string::npos) {
        std::string part = str.substr(start, end - start);
        if (!skip_empty || !part.empty()) {
            result.push_back(std::move(part));
        }
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }
    
    // 添加最后一个部分
    std::string last_part = str.substr(start);
    if (!skip_empty || !last_part.empty()) {
        result.push_back(std::move(last_part));
    }
    
    return result;
}

std::string StringUtils::Join(const std::vector<std::string>& parts,
                             const std::string& delimiter) const {
    if (parts.empty()) return "";
    
    std::ostringstream oss;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) oss << delimiter;
        oss << parts[i];
    }
    
    return oss.str();
}

std::string StringUtils::Trim(const std::string& str, const std::string& whitespace) const {
    if (str.empty()) return str;
    
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) return "";
    
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

std::string StringUtils::DetectDelimiter(const std::string& str) const {
    std::vector<std::pair<std::string, int>> candidates = {
        {DefaultDelimiters::PRIMARY, 0},
        {DefaultDelimiters::PIPE, 0},
        {DefaultDelimiters::UNDERSCORE, 0},
        {DefaultDelimiters::SLASH, 0},
        {DefaultDelimiters::STAR, 0},
        {DefaultDelimiters::PLUS, 0},
        {DefaultDelimiters::TAB, 0}
    };
    
    // 统计各分隔符出现次数
    for (auto& [delimiter, count] : candidates) {
        size_t pos = 0;
        while ((pos = str.find(delimiter, pos)) != std::string::npos) {
            count++;
            pos += delimiter.length();
        }
    }
    
    // 返回出现次数最多的分隔符
    auto max_it = std::max_element(candidates.begin(), candidates.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });
    
    return max_it->second > 0 ? max_it->first : DefaultDelimiters::PRIMARY;
}

std::string StringUtils::TimeToString(const std::chrono::system_clock::time_point& tp,
                                     const std::string& format) const {
    auto time_t = std::chrono::system_clock::to_time_t(tp);
    auto tm = *std::localtime(&time_t);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, format.c_str());
    
    return oss.str();
}

std::chrono::system_clock::time_point StringUtils::StringToTime(const std::string& str,
                                                               const std::string& format) const {
    std::tm tm = {};
    std::istringstream iss(str);
    iss >> std::get_time(&tm, format.c_str());
    
    if (iss.fail()) {
        throw detail::TypeConversionException("Failed to parse time string: " + str);
    }
    
    auto time_t = std::mktime(&tm);
    return std::chrono::system_clock::from_time_t(time_t);
}

bool StringUtils::TryStringToTime(const std::string& str,
                                 std::chrono::system_clock::time_point& result,
                                 const std::string& format) const noexcept {
    try {
        result = StringToTime(str, format);
        return true;
    } catch (...) {
        return false;
    }
}

bool StringUtils::HasChinesePunctuation(const std::string& str) const {
    // UTF-8编码的中文标点符号检测
    auto replacements = GetChinesePunctuationReplacements();
    
    for (const auto& [chinese, english] : replacements) {
        if (str.find(chinese) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

std::string StringUtils::NormalizePunctuation(const std::string& str) const {
    std::string result = str;
    auto replacements = GetChinesePunctuationReplacements();
    
    for (const auto& [chinese, english] : replacements) {
        size_t pos = 0;
        while ((pos = result.find(chinese, pos)) != std::string::npos) {
            result.replace(pos, chinese.length(), english);
            pos += english.length();
        }
    }
    
    return result;
}

std::vector<std::pair<std::string, std::string>> StringUtils::GetChinesePunctuationReplacements() const {
    return {
        {"，", ","},   {"。", "."},   {"；", ";"},   {"：", ":"},
        {"？", "?"},   {"！", "!"},   {""", "\""},  {""", "\""},
        {"'", "'"},    {"'", "'"},    {"（", "("},   {"）", ")"},
        {"【", "["},   {"】", "]"},   {"《", "<"},   {"》", ">"},
        {"、", ","},   {"…", "..."}
    };
}

// ============ ThreadSafeStringUtils 实现 ============

std::vector<std::string> ThreadSafeStringUtils::Split(const std::string& str, 
                                                     const std::string& delimiter,
                                                     bool skip_empty) const {
    return StringUtils::GetInstance().Split(str, delimiter, skip_empty);
}

std::string ThreadSafeStringUtils::Join(const std::vector<std::string>& parts,
                                       const std::string& delimiter) const {
    return StringUtils::GetInstance().Join(parts, delimiter);
}

std::string ThreadSafeStringUtils::TimeToString(const std::chrono::system_clock::time_point& tp,
                                               const std::string& format) const {
    return StringUtils::GetInstance().TimeToString(tp, format);
}

std::chrono::system_clock::time_point ThreadSafeStringUtils::StringToTime(const std::string& str,
                                                                         const std::string& format) const {
    return StringUtils::GetInstance().StringToTime(str, format);
}

} // namespace utilities
} // namespace common