/**
 * @file compiler_detector.h
 * @brief 动态编译器检测器 - 运行时检测用户系统的编译环境
 * 
 * 这个模块负责在程序运行时检测用户系统上可用的 C++ 编译器，
 * 并获取对应的系统包含路径，消除对静态配置文件的依赖。
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace lua_binding_generator {

/**
 * @brief 编译器检测器
 * 
 * 在运行时动态检测系统上可用的 C++ 编译器，支持：
 * - Clang/LLVM
 * - GCC
 * - MSVC (Windows)
 */
class CompilerDetector {
public:
    /**
     * @brief 编译器信息结构
     */
    struct CompilerInfo {
        std::string compiler_path;                 ///< 编译器可执行文件路径
        std::vector<std::string> include_paths;    ///< 系统包含路径列表
        std::vector<std::string> lib_paths;        ///< 库文件路径列表
        std::string version;                       ///< 编译器版本
        std::string type;                          ///< 编译器类型：clang, gcc, msvc
        std::string target_triple;                 ///< 目标三元组
        bool found = false;                        ///< 是否找到可用编译器
        
        /**
         * @brief 检查编译器信息是否有效
         */
        bool IsValid() const {
            return found && !compiler_path.empty() && !include_paths.empty();
        }
        
        /**
         * @brief 获取调试信息字符串
         */
        std::string GetDebugString() const;
    };

public:
    /**
     * @brief 构造函数
     */
    CompilerDetector();

    /**
     * @brief 析构函数
     */
    ~CompilerDetector();

    /**
     * @brief 检测可用的编译器
     * @return 编译器信息，如果未找到则 found = false
     */
    CompilerInfo DetectCompiler();

    /**
     * @brief 强制使用指定的编译器
     * @param compiler_path 编译器路径
     * @return 编译器信息
     */
    CompilerInfo UseCompiler(const std::string& compiler_path);

    /**
     * @brief 检查指定路径的编译器是否可用
     * @param compiler_path 编译器路径
     * @return 是否可用
     */
    bool IsCompilerAvailable(const std::string& compiler_path);

    /**
     * @brief 获取支持的编译器类型列表
     * @return 编译器类型列表
     */
    std::vector<std::string> GetSupportedCompilers() const;

    /**
     * @brief 设置是否启用详细输出
     * @param verbose 是否详细输出
     */
    void SetVerbose(bool verbose);

private:
    bool verbose_;                                 ///< 是否启用详细输出

    /**
     * @brief 尝试检测 Clang 编译器
     * @return 编译器信息
     */
    CompilerInfo TryClang();

    /**
     * @brief 尝试检测 GCC 编译器
     * @return 编译器信息
     */
    CompilerInfo TryGCC();

    /**
     * @brief 尝试检测 MSVC 编译器
     * @return 编译器信息
     */
    CompilerInfo TryMSVC();

    /**
     * @brief 获取 Clang 的系统包含路径
     * @param clang_path Clang 编译器路径
     * @return 包含路径列表
     */
    std::vector<std::string> GetClangIncludePaths(const std::string& clang_path);

    /**
     * @brief 获取 GCC 的系统包含路径
     * @param gcc_path GCC 编译器路径
     * @return 包含路径列表
     */
    std::vector<std::string> GetGCCIncludePaths(const std::string& gcc_path);

    /**
     * @brief 获取 MSVC 的系统包含路径
     * @return 包含路径列表
     */
    std::vector<std::string> GetMSVCIncludePaths();

    /**
     * @brief 获取编译器版本信息
     * @param compiler_path 编译器路径
     * @return 版本字符串
     */
    std::string GetCompilerVersion(const std::string& compiler_path);

    /**
     * @brief 获取编译器目标三元组
     * @param compiler_path 编译器路径
     * @return 目标三元组字符串
     */
    std::string GetTargetTriple(const std::string& compiler_path);

    /**
     * @brief 检查可执行文件是否存在且可执行
     * @param path 可执行文件路径
     * @return 是否可执行
     */
    bool IsExecutableAvailable(const std::string& path);

    /**
     * @brief 获取可执行文件的完整路径
     * @param path 可执行文件路径（可能是相对路径）
     * @return 完整的绝对路径
     */
    std::string GetFullExecutablePath(const std::string& path);

    /**
     * @brief 执行命令并获取输出
     * @param command 要执行的命令
     * @return 命令输出，如果失败返回空字符串
     */
    std::string ExecuteCommand(const std::string& command);

    /**
     * @brief 获取环境变量值
     * @param name 环境变量名
     * @return 环境变量值，如果不存在返回空字符串
     */
    std::string GetEnvironmentVariable(const std::string& name);

    /**
     * @brief 去除字符串前后的空白字符
     * @param str 输入字符串
     * @return 处理后的字符串
     */
    std::string TrimWhitespace(const std::string& str);

    /**
     * @brief 检查路径是否存在
     * @param path 文件或目录路径
     * @return 是否存在
     */
    bool PathExists(const std::string& path);

    /**
     * @brief 获取可能的编译器路径列表
     * @param compiler_name 编译器名称（如 "clang++", "g++"）
     * @return 可能的路径列表
     */
    std::vector<std::string> GetPossibleCompilerPaths(const std::string& compiler_name);

    /**
     * @brief 解析编译器输出中的包含路径
     * @param output 编译器输出
     * @return 包含路径列表
     */
    std::vector<std::string> ParseIncludePathsFromOutput(const std::string& output);

    /**
     * @brief 验证包含路径是否有效
     * @param paths 包含路径列表
     * @return 有效的包含路径列表
     */
    std::vector<std::string> ValidateIncludePaths(const std::vector<std::string>& paths);

    /**
     * @brief 记录调试信息
     * @param message 调试信息
     */
    void LogDebug(const std::string& message);

    /**
     * @brief 记录错误信息
     * @param message 错误信息
     */
    void LogError(const std::string& message);

#ifdef _WIN32
    /**
     * @brief Windows 特定：查找 Visual Studio 安装路径
     * @return VS 安装路径列表
     */
    std::vector<std::string> FindVisualStudioInstallations();

    /**
     * @brief Windows 特定：获取 Windows SDK 路径
     * @return SDK 路径列表
     */
    std::vector<std::string> GetWindowsSDKPaths();
#endif

#ifdef __APPLE__
    /**
     * @brief macOS 特定：获取 Xcode 工具链路径
     * @return 工具链路径
     */
    std::string GetXcodeToolchainPath();

    /**
     * @brief macOS 特定：获取 SDK 路径
     * @return SDK 路径
     */
    std::string GetMacOSSDKPath();
#endif
};

} // namespace lua_binding_generator