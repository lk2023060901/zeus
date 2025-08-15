/**
 * @file dynamic_compilation_database.h
 * @brief 动态编译数据库 - 在内存中构建编译命令，替代静态 JSON 文件
 * 
 * 这个模块提供了一个动态的编译数据库实现，能够在运行时根据
 * 检测到的编译器信息构建编译命令，完全消除对 compile_commands.json 的依赖。
 */

#pragma once

#include "compiler_detector.h"
#include <clang/Tooling/CompilationDatabase.h>
#include <string>
#include <vector>
#include <memory>

namespace lua_binding_generator {

/**
 * @brief 动态编译数据库
 * 
 * 继承自 clang::tooling::CompilationDatabase，在内存中动态构建编译命令，
 * 使用运行时检测到的编译器和系统路径信息。
 */
class DynamicCompilationDatabase : public clang::tooling::CompilationDatabase {
public:
    /**
     * @brief 构造函数
     * @param source_files 要编译的源文件列表
     * @param zeus_include_path Zeus 项目的包含路径
     * @param compiler_info 编译器信息
     * @param additional_flags 额外的编译标志
     */
    DynamicCompilationDatabase(
        const std::vector<std::string>& source_files,
        const std::string& zeus_include_path,
        const CompilerDetector::CompilerInfo& compiler_info,
        const std::vector<std::string>& additional_flags = {});

    /**
     * @brief 析构函数
     */
    ~DynamicCompilationDatabase() override;

    /**
     * @brief 获取指定文件的编译命令
     * @param FilePath 文件路径
     * @return 编译命令列表
     */
    std::vector<clang::tooling::CompileCommand> getCompileCommands(
        llvm::StringRef FilePath) const override;

    /**
     * @brief 获取所有文件列表
     * @return 文件路径列表
     */
    std::vector<std::string> getAllFiles() const override;

    /**
     * @brief 获取所有编译命令
     * @return 所有编译命令列表
     */
    std::vector<clang::tooling::CompileCommand> getAllCompileCommands() const override;

    /**
     * @brief 添加额外的包含路径
     * @param include_path 包含路径
     */
    void AddIncludePath(const std::string& include_path);

    /**
     * @brief 添加额外的编译标志
     * @param flag 编译标志
     */
    void AddCompileFlag(const std::string& flag);

    /**
     * @brief 设置 C++ 标准
     * @param std_version C++ 标准版本（如 "c++17", "c++20"）
     */
    void SetCppStandard(const std::string& std_version);

    /**
     * @brief 启用或禁用详细输出
     * @param verbose 是否启用详细输出
     */
    void SetVerbose(bool verbose);

    /**
     * @brief 获取编译数据库的调试信息
     * @return 调试信息字符串
     */
    std::string GetDebugInfo() const;

private:
    std::vector<std::string> source_files_;              ///< 源文件列表
    std::string zeus_include_path_;                      ///< Zeus 项目包含路径
    CompilerDetector::CompilerInfo compiler_info_;       ///< 编译器信息
    std::vector<std::string> additional_include_paths_;  ///< 额外的包含路径
    std::vector<std::string> additional_flags_;          ///< 额外的编译标志
    std::string cpp_standard_;                           ///< C++ 标准
    bool verbose_;                                       ///< 是否启用详细输出
    
    mutable std::vector<clang::tooling::CompileCommand> compile_commands_; ///< 缓存的编译命令

    /**
     * @brief 构建编译命令
     */
    void BuildCompileCommands() const;

    /**
     * @brief 为指定文件创建编译命令
     * @param file_path 文件路径
     * @return 编译命令
     */
    clang::tooling::CompileCommand CreateCompileCommand(const std::string& file_path) const;

    /**
     * @brief 获取基础编译参数
     * @return 编译参数列表
     */
    std::vector<std::string> GetBaseCompileArgs() const;

    /**
     * @brief 获取包含路径参数
     * @return 包含路径参数列表
     */
    std::vector<std::string> GetIncludeArgs() const;

    /**
     * @brief 获取编译器特定的参数
     * @return 编译器参数列表
     */
    std::vector<std::string> GetCompilerSpecificArgs() const;

    /**
     * @brief 标准化文件路径
     * @param file_path 原始文件路径
     * @return 标准化后的文件路径
     */
    std::string NormalizeFilePath(const std::string& file_path) const;

    /**
     * @brief 记录调试信息
     * @param message 调试信息
     */
    void LogDebug(const std::string& message) const;

    /**
     * @brief 检查编译命令的有效性
     * @param cmd 编译命令
     * @return 是否有效
     */
    bool ValidateCompileCommand(const clang::tooling::CompileCommand& cmd) const;
};

/**
 * @brief 创建动态编译数据库的便利函数
 * @param source_files 源文件列表
 * @param zeus_include_path Zeus 项目包含路径
 * @param compiler_info 编译器信息
 * @param additional_flags 额外的编译标志
 * @return 动态编译数据库的智能指针
 */
std::unique_ptr<DynamicCompilationDatabase> CreateDynamicCompilationDatabase(
    const std::vector<std::string>& source_files,
    const std::string& zeus_include_path,
    const CompilerDetector::CompilerInfo& compiler_info,
    const std::vector<std::string>& additional_flags = {});

} // namespace lua_binding_generator