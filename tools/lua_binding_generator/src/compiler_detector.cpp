/**
 * @file compiler_detector.cpp
 * @brief 动态编译器检测器实现
 */

#include "lua_binding_generator/compiler_detector.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

namespace lua_binding_generator {

namespace {
    // 平台相关的空设备路径
#ifdef _WIN32
    constexpr const char* NULL_DEVICE = "NUL";
#else
    constexpr const char* NULL_DEVICE = "/dev/null";
#endif
}

// CompilerInfo 方法实现

std::string CompilerDetector::CompilerInfo::GetDebugString() const {
    std::ostringstream oss;
    oss << "CompilerInfo {\n";
    oss << "  type: " << type << "\n";
    oss << "  path: " << compiler_path << "\n";
    oss << "  version: " << version << "\n";
    oss << "  target: " << target_triple << "\n";
    oss << "  found: " << (found ? "true" : "false") << "\n";
    oss << "  include_paths: [\n";
    for (const auto& path : include_paths) {
        oss << "    " << path << "\n";
    }
    oss << "  ]\n";
    oss << "  lib_paths: [\n";
    for (const auto& path : lib_paths) {
        oss << "    " << path << "\n";
    }
    oss << "  ]\n";
    oss << "}";
    return oss.str();
}

// CompilerDetector 方法实现

CompilerDetector::CompilerDetector() : verbose_(false) {
}

CompilerDetector::~CompilerDetector() = default;

CompilerDetector::CompilerInfo CompilerDetector::DetectCompiler() {
    LogDebug("开始检测编译器...");

    // 按优先级顺序尝试不同的编译器
    
    // 1. 首先尝试 Clang（推荐）
    LogDebug("尝试检测 Clang...");
    auto clang_info = TryClang();
    if (clang_info.found) {
        LogDebug("找到 Clang 编译器: " + clang_info.compiler_path);
        return clang_info;
    }

    // 2. 然后尝试 GCC
    LogDebug("尝试检测 GCC...");
    auto gcc_info = TryGCC();
    if (gcc_info.found) {
        LogDebug("找到 GCC 编译器: " + gcc_info.compiler_path);
        return gcc_info;
    }

#ifdef _WIN32
    // 3. Windows 下尝试 MSVC
    LogDebug("尝试检测 MSVC...");
    auto msvc_info = TryMSVC();
    if (msvc_info.found) {
        LogDebug("找到 MSVC 编译器: " + msvc_info.compiler_path);
        return msvc_info;
    }
#endif

    LogError("未找到任何可用的 C++ 编译器");
    return CompilerInfo{}; // 返回空的编译器信息
}

CompilerDetector::CompilerInfo CompilerDetector::UseCompiler(const std::string& compiler_path) {
    LogDebug("使用指定编译器: " + compiler_path);
    
    if (!IsExecutableAvailable(compiler_path)) {
        LogError("指定的编译器不可用: " + compiler_path);
        return CompilerInfo{};
    }

    CompilerInfo info;
    info.compiler_path = compiler_path;
    info.found = true;

    // 检测编译器类型
    std::string compiler_name = std::filesystem::path(compiler_path).filename().string();
    std::transform(compiler_name.begin(), compiler_name.end(), compiler_name.begin(), ::tolower);

    if (compiler_name.find("clang") != std::string::npos) {
        info.type = "clang";
        info.include_paths = GetClangIncludePaths(compiler_path);
    } else if (compiler_name.find("gcc") != std::string::npos || compiler_name.find("g++") != std::string::npos) {
        info.type = "gcc";
        info.include_paths = GetGCCIncludePaths(compiler_path);
    } else if (compiler_name.find("cl.exe") != std::string::npos) {
        info.type = "msvc";
        info.include_paths = GetMSVCIncludePaths();
    } else {
        // 默认尝试作为 Clang 兼容编译器
        info.type = "unknown";
        info.include_paths = GetClangIncludePaths(compiler_path);
    }

    info.version = GetCompilerVersion(compiler_path);
    info.target_triple = GetTargetTriple(compiler_path);

    return info;
}

bool CompilerDetector::IsCompilerAvailable(const std::string& compiler_path) {
    return IsExecutableAvailable(compiler_path);
}

std::vector<std::string> CompilerDetector::GetSupportedCompilers() const {
    return {"clang++", "g++", "cl.exe"};
}

void CompilerDetector::SetVerbose(bool verbose) {
    verbose_ = verbose;
}

// 私有方法实现

CompilerDetector::CompilerInfo CompilerDetector::TryClang() {
    CompilerInfo info;
    info.type = "clang";

    // 获取可能的 Clang 路径
    auto possible_paths = GetPossibleCompilerPaths("clang++");
    
    // 检查环境变量
    std::string env_cxx = GetEnvironmentVariable("CXX");
    if (!env_cxx.empty() && env_cxx.find("clang") != std::string::npos) {
        possible_paths.insert(possible_paths.begin(), env_cxx);
    }

    std::string env_clang = GetEnvironmentVariable("CLANG_CXX");
    if (!env_clang.empty()) {
        possible_paths.insert(possible_paths.begin(), env_clang);
    }

    // 尝试每个可能的路径
    for (const auto& path : possible_paths) {
        if (IsExecutableAvailable(path)) {
            // 获取编译器的完整路径
            std::string full_path = GetFullExecutablePath(path);
            info.compiler_path = full_path;
            info.include_paths = GetClangIncludePaths(full_path);
            
            if (!info.include_paths.empty()) {
                info.version = GetCompilerVersion(full_path);
                info.target_triple = GetTargetTriple(full_path);
                info.found = true;
                break;
            }
        }
    }

    return info;
}

CompilerDetector::CompilerInfo CompilerDetector::TryGCC() {
    CompilerInfo info;
    info.type = "gcc";

    // 获取可能的 GCC 路径
    auto possible_paths = GetPossibleCompilerPaths("g++");
    
    // 检查环境变量
    std::string env_cxx = GetEnvironmentVariable("CXX");
    if (!env_cxx.empty() && (env_cxx.find("gcc") != std::string::npos || env_cxx.find("g++") != std::string::npos)) {
        possible_paths.insert(possible_paths.begin(), env_cxx);
    }

    // 尝试每个可能的路径
    for (const auto& path : possible_paths) {
        if (IsExecutableAvailable(path)) {
            // 获取编译器的完整路径
            std::string full_path = GetFullExecutablePath(path);
            info.compiler_path = full_path;
            info.include_paths = GetGCCIncludePaths(full_path);
            
            if (!info.include_paths.empty()) {
                info.version = GetCompilerVersion(full_path);
                info.target_triple = GetTargetTriple(full_path);
                info.found = true;
                break;
            }
        }
    }

    return info;
}

CompilerDetector::CompilerInfo CompilerDetector::TryMSVC() {
#ifdef _WIN32
    CompilerInfo info;
    info.type = "msvc";

    // 尝试查找 Visual Studio 安装
    auto vs_paths = FindVisualStudioInstallations();
    
    for (const auto& vs_path : vs_paths) {
        std::string cl_path = vs_path + "\\bin\\Hostx64\\x64\\cl.exe";
        if (IsExecutableAvailable(cl_path)) {
            info.compiler_path = cl_path;
            info.include_paths = GetMSVCIncludePaths();
            
            if (!info.include_paths.empty()) {
                info.version = GetCompilerVersion(cl_path);
                info.found = true;
                break;
            }
        }
    }

    return info;
#else
    return CompilerInfo{}; // 非 Windows 平台不支持 MSVC
#endif
}

std::vector<std::string> CompilerDetector::GetClangIncludePaths(const std::string& clang_path) {
    // 执行 clang++ -E -v -x c++ /dev/null 获取系统包含路径
    std::string command = clang_path + " -E -v -x c++ " + NULL_DEVICE + " 2>&1";
    
    LogDebug("执行命令获取包含路径: " + command);
    
    std::string output = ExecuteCommand(command);
    if (output.empty()) {
        LogError("无法获取 Clang 包含路径");
        return {};
    }

    return ParseIncludePathsFromOutput(output);
}

std::vector<std::string> CompilerDetector::GetGCCIncludePaths(const std::string& gcc_path) {
    // 执行 g++ -E -v -x c++ /dev/null 获取系统包含路径
    std::string command = gcc_path + " -E -v -x c++ " + NULL_DEVICE + " 2>&1";
    
    LogDebug("执行命令获取包含路径: " + command);
    
    std::string output = ExecuteCommand(command);
    if (output.empty()) {
        LogError("无法获取 GCC 包含路径");
        return {};
    }

    return ParseIncludePathsFromOutput(output);
}

std::vector<std::string> CompilerDetector::GetMSVCIncludePaths() {
#ifdef _WIN32
    std::vector<std::string> paths;
    
    // 获取 Visual Studio 包含路径
    auto vs_paths = FindVisualStudioInstallations();
    for (const auto& vs_path : vs_paths) {
        std::string include_path = vs_path + "\\include";
        if (PathExists(include_path)) {
            paths.push_back(include_path);
        }
    }

    // 获取 Windows SDK 包含路径
    auto sdk_paths = GetWindowsSDKPaths();
    paths.insert(paths.end(), sdk_paths.begin(), sdk_paths.end());

    return ValidateIncludePaths(paths);
#else
    return {};
#endif
}

std::string CompilerDetector::GetCompilerVersion(const std::string& compiler_path) {
    std::string command = compiler_path + " --version 2>&1";
    std::string output = ExecuteCommand(command);
    
    if (!output.empty()) {
        // 提取第一行作为版本信息
        size_t newline_pos = output.find('\n');
        if (newline_pos != std::string::npos) {
            return output.substr(0, newline_pos);
        }
    }
    
    return "unknown";
}

std::string CompilerDetector::GetTargetTriple(const std::string& compiler_path) {
    std::string command = compiler_path + " -dumpmachine 2>&1";
    std::string output = ExecuteCommand(command);
    
    return TrimWhitespace(output);
}

bool CompilerDetector::IsExecutableAvailable(const std::string& path) {
    if (path.empty()) {
        return false;
    }

    // 如果是相对路径，尝试在 PATH 中查找
    if (path.find('/') == std::string::npos && path.find('\\') == std::string::npos) {
        std::string command = "which " + path + " 2>" + NULL_DEVICE;
#ifdef _WIN32
        command = "where " + path + " 2>" + NULL_DEVICE;
#endif
        std::string result = ExecuteCommand(command);
        return !result.empty();
    }

    // 绝对路径，直接检查文件是否存在且可执行
    return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
}

std::string CompilerDetector::GetFullExecutablePath(const std::string& path) {
    if (path.empty()) {
        return "";
    }

    // 如果已经是绝对路径，直接返回
    if (path.front() == '/' || (path.size() > 1 && path[1] == ':')) {
        return std::filesystem::exists(path) ? path : "";
    }

    // 如果是相对路径，使用 which/where 查找完整路径
    std::string command = "which " + path + " 2>" + NULL_DEVICE;
#ifdef _WIN32
    command = "where " + path + " 2>" + NULL_DEVICE;
#endif
    
    std::string result = ExecuteCommand(command);
    result = TrimWhitespace(result);
    
    // 返回第一行（如果有多个结果）
    size_t newline_pos = result.find('\n');
    if (newline_pos != std::string::npos) {
        result = result.substr(0, newline_pos);
    }
    
    return result.empty() ? path : result;
}

std::string CompilerDetector::ExecuteCommand(const std::string& command) {
    std::string result;
    
    // 使用 popen 执行命令并获取输出
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        LogError("无法执行命令: " + command);
        return "";
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }

    int exit_code = pclose(pipe);
    if (exit_code != 0) {
        LogDebug("命令执行失败，退出码: " + std::to_string(exit_code));
    }

    return result;
}

std::string CompilerDetector::GetEnvironmentVariable(const std::string& name) {
    const char* value = std::getenv(name.c_str());
    return value ? std::string(value) : std::string();
}

std::string CompilerDetector::TrimWhitespace(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        return "";
    }
    
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

bool CompilerDetector::PathExists(const std::string& path) {
    return std::filesystem::exists(path);
}

std::vector<std::string> CompilerDetector::GetPossibleCompilerPaths(const std::string& compiler_name) {
    std::vector<std::string> paths;
    
    // 添加命令名本身（在 PATH 中查找）
    paths.push_back(compiler_name);

#ifdef __APPLE__
    // macOS 特定路径
    paths.push_back("/usr/bin/" + compiler_name);
    paths.push_back("/usr/local/bin/" + compiler_name);
    paths.push_back("/opt/homebrew/bin/" + compiler_name);
    
    // Xcode 工具链路径
    std::string xcode_path = GetXcodeToolchainPath();
    if (!xcode_path.empty()) {
        paths.push_back(xcode_path + "/usr/bin/" + compiler_name);
    }
    
#elif __linux__
    // Linux 特定路径
    paths.push_back("/usr/bin/" + compiler_name);
    paths.push_back("/usr/local/bin/" + compiler_name);
    paths.push_back("/opt/gcc/bin/" + compiler_name);
    paths.push_back("/opt/llvm/bin/" + compiler_name);
    
#elif _WIN32
    // Windows 特定路径会在 TryMSVC 中处理
    paths.push_back(compiler_name + ".exe");
#endif

    return paths;
}

std::vector<std::string> CompilerDetector::ParseIncludePathsFromOutput(const std::string& output) {
    std::vector<std::string> paths;
    
    std::istringstream stream(output);
    std::string line;
    bool in_include_section = false;
    
    while (std::getline(stream, line)) {
        // 查找包含路径段的开始
        if (line.find("#include <...> search starts here:") != std::string::npos) {
            in_include_section = true;
            continue;
        }
        
        // 查找包含路径段的结束
        if (line.find("End of search list.") != std::string::npos) {
            break;
        }
        
        // 如果在包含路径段中，提取路径
        if (in_include_section && !line.empty()) {
            std::string path = TrimWhitespace(line);
            
            // 过滤掉注释和无效路径
            if (!path.empty() && path[0] != '#' && PathExists(path)) {
                paths.push_back(path);
            }
        }
    }
    
    return ValidateIncludePaths(paths);
}

std::vector<std::string> CompilerDetector::ValidateIncludePaths(const std::vector<std::string>& paths) {
    std::vector<std::string> valid_paths;
    
    for (const auto& path : paths) {
        if (!path.empty() && PathExists(path) && std::filesystem::is_directory(path)) {
            valid_paths.push_back(path);
        }
    }
    
    return valid_paths;
}

void CompilerDetector::LogDebug(const std::string& message) {
    if (verbose_) {
        std::cout << "[DEBUG] " << message << std::endl;
    }
}

void CompilerDetector::LogError(const std::string& message) {
    std::cerr << "[ERROR] " << message << std::endl;
}

#ifdef _WIN32
std::vector<std::string> CompilerDetector::FindVisualStudioInstallations() {
    std::vector<std::string> installations;
    
    // 常见的 Visual Studio 安装路径
    std::vector<std::string> common_paths = {
        "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC",
        "C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\VC\\Tools\\MSVC",
        "C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\VC\\Tools\\MSVC",
        "C:\\Program Files\\Microsoft Visual Studio\\2019\\Community\\VC\\Tools\\MSVC",
        "C:\\Program Files\\Microsoft Visual Studio\\2019\\Professional\\VC\\Tools\\MSVC",
        "C:\\Program Files\\Microsoft Visual Studio\\2019\\Enterprise\\VC\\Tools\\MSVC",
        "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Tools\\MSVC",
        "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Professional\\VC\\Tools\\MSVC",
        "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Enterprise\\VC\\Tools\\MSVC"
    };
    
    for (const auto& base_path : common_paths) {
        if (PathExists(base_path)) {
            // 查找最新版本的工具集
            try {
                for (const auto& entry : std::filesystem::directory_iterator(base_path)) {
                    if (entry.is_directory()) {
                        installations.push_back(entry.path().string());
                    }
                }
            } catch (const std::exception&) {
                // 忽略访问错误
            }
        }
    }
    
    return installations;
}

std::vector<std::string> CompilerDetector::GetWindowsSDKPaths() {
    std::vector<std::string> paths;
    
    // 常见的 Windows SDK 路径
    std::vector<std::string> sdk_base_paths = {
        "C:\\Program Files (x86)\\Windows Kits\\10\\Include",
        "C:\\Program Files\\Windows Kits\\10\\Include"
    };
    
    for (const auto& base_path : sdk_base_paths) {
        if (PathExists(base_path)) {
            try {
                // 查找最新版本的 SDK
                for (const auto& entry : std::filesystem::directory_iterator(base_path)) {
                    if (entry.is_directory()) {
                        std::string version_path = entry.path().string();
                        
                        // 添加 ucrt 和 um 路径
                        std::string ucrt_path = version_path + "\\ucrt";
                        std::string um_path = version_path + "\\um";
                        
                        if (PathExists(ucrt_path)) {
                            paths.push_back(ucrt_path);
                        }
                        if (PathExists(um_path)) {
                            paths.push_back(um_path);
                        }
                    }
                }
            } catch (const std::exception&) {
                // 忽略访问错误
            }
        }
    }
    
    return paths;
}
#endif

#ifdef __APPLE__
std::string CompilerDetector::GetXcodeToolchainPath() {
    // 尝试获取 Xcode 开发者目录
    std::string output = ExecuteCommand("xcode-select -p 2>" + std::string(NULL_DEVICE));
    std::string dev_dir = TrimWhitespace(output);
    
    if (!dev_dir.empty() && PathExists(dev_dir)) {
        std::string toolchain_path = dev_dir + "/Toolchains/XcodeDefault.xctoolchain";
        if (PathExists(toolchain_path)) {
            return toolchain_path;
        }
    }
    
    // 回退到默认路径
    std::string default_path = "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain";
    if (PathExists(default_path)) {
        return default_path;
    }
    
    return "";
}

std::string CompilerDetector::GetMacOSSDKPath() {
    std::string output = ExecuteCommand("xcrun --show-sdk-path 2>" + std::string(NULL_DEVICE));
    return TrimWhitespace(output);
}
#endif

} // namespace lua_binding_generator