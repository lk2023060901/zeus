/**
 * @file dynamic_compilation_database.cpp
 * @brief 动态编译数据库实现
 */

#include "lua_binding_generator/dynamic_compilation_database.h"

#include <iostream>
#include <filesystem>
#include <algorithm>
#include <sstream>

namespace lua_binding_generator {

DynamicCompilationDatabase::DynamicCompilationDatabase(
    const std::vector<std::string>& source_files,
    const std::string& zeus_include_path,
    const CompilerDetector::CompilerInfo& compiler_info,
    const std::vector<std::string>& additional_flags)
    : source_files_(source_files)
    , zeus_include_path_(zeus_include_path)
    , compiler_info_(compiler_info)
    , additional_flags_(additional_flags)
    , cpp_standard_("c++17")
    , verbose_(false) {
    
    LogDebug("创建动态编译数据库");
    LogDebug("源文件数量: " + std::to_string(source_files_.size()));
    LogDebug("Zeus 包含路径: " + zeus_include_path_);
    LogDebug("编译器类型: " + compiler_info_.type);
    LogDebug("编译器路径: " + compiler_info_.compiler_path);
}

DynamicCompilationDatabase::~DynamicCompilationDatabase() = default;

std::vector<clang::tooling::CompileCommand> DynamicCompilationDatabase::getCompileCommands(
    llvm::StringRef FilePath) const {
    
    std::string file_path = FilePath.str();
    LogDebug("获取文件编译命令: " + file_path);

    // 检查文件是否在我们的源文件列表中
    auto it = std::find(source_files_.begin(), source_files_.end(), file_path);
    if (it == source_files_.end()) {
        // 尝试标准化路径后再次查找
        std::string normalized_path = NormalizeFilePath(file_path);
        for (const auto& source_file : source_files_) {
            if (NormalizeFilePath(source_file) == normalized_path) {
                file_path = source_file;
                break;
            }
        }
    }

    // 创建编译命令
    auto cmd = CreateCompileCommand(file_path);
    
    if (ValidateCompileCommand(cmd)) {
        LogDebug("成功创建编译命令");
        return {cmd};
    } else {
        LogDebug("编译命令验证失败");
        return {};
    }
}

std::vector<std::string> DynamicCompilationDatabase::getAllFiles() const {
    LogDebug("获取所有文件列表");
    return source_files_;
}

std::vector<clang::tooling::CompileCommand> DynamicCompilationDatabase::getAllCompileCommands() const {
    LogDebug("获取所有编译命令");
    
    if (compile_commands_.empty()) {
        BuildCompileCommands();
    }
    
    return compile_commands_;
}

void DynamicCompilationDatabase::AddIncludePath(const std::string& include_path) {
    if (!include_path.empty()) {
        additional_include_paths_.push_back(include_path);
        LogDebug("添加包含路径: " + include_path);
        // 清除缓存，强制重新构建
        compile_commands_.clear();
    }
}

void DynamicCompilationDatabase::AddCompileFlag(const std::string& flag) {
    if (!flag.empty()) {
        additional_flags_.push_back(flag);
        LogDebug("添加编译标志: " + flag);
        // 清除缓存，强制重新构建
        compile_commands_.clear();
    }
}

void DynamicCompilationDatabase::SetCppStandard(const std::string& std_version) {
    cpp_standard_ = std_version;
    LogDebug("设置 C++ 标准: " + std_version);
    // 清除缓存，强制重新构建
    compile_commands_.clear();
}

void DynamicCompilationDatabase::SetVerbose(bool verbose) {
    verbose_ = verbose;
}

std::string DynamicCompilationDatabase::GetDebugInfo() const {
    std::ostringstream oss;
    
    oss << "DynamicCompilationDatabase {\n";
    oss << "  源文件数量: " << source_files_.size() << "\n";
    oss << "  Zeus 包含路径: " << zeus_include_path_ << "\n";
    oss << "  编译器类型: " << compiler_info_.type << "\n";
    oss << "  编译器路径: " << compiler_info_.compiler_path << "\n";
    oss << "  C++ 标准: " << cpp_standard_ << "\n";
    oss << "  额外包含路径数量: " << additional_include_paths_.size() << "\n";
    oss << "  额外编译标志数量: " << additional_flags_.size() << "\n";
    oss << "  系统包含路径数量: " << compiler_info_.include_paths.size() << "\n";
    
    oss << "  源文件列表:\n";
    for (const auto& file : source_files_) {
        oss << "    " << file << "\n";
    }
    
    oss << "  系统包含路径:\n";
    for (const auto& path : compiler_info_.include_paths) {
        oss << "    " << path << "\n";
    }
    
    if (!additional_include_paths_.empty()) {
        oss << "  额外包含路径:\n";
        for (const auto& path : additional_include_paths_) {
            oss << "    " << path << "\n";
        }
    }
    
    if (!additional_flags_.empty()) {
        oss << "  额外编译标志:\n";
        for (const auto& flag : additional_flags_) {
            oss << "    " << flag << "\n";
        }
    }
    
    oss << "}";
    return oss.str();
}

// 私有方法实现

void DynamicCompilationDatabase::BuildCompileCommands() const {
    LogDebug("构建所有编译命令");
    
    compile_commands_.clear();
    compile_commands_.reserve(source_files_.size());
    
    for (const auto& file_path : source_files_) {
        auto cmd = CreateCompileCommand(file_path);
        if (ValidateCompileCommand(cmd)) {
            compile_commands_.push_back(cmd);
        } else {
            LogDebug("跳过无效编译命令: " + file_path);
        }
    }
    
    LogDebug("构建完成，共 " + std::to_string(compile_commands_.size()) + " 个有效命令");
}

clang::tooling::CompileCommand DynamicCompilationDatabase::CreateCompileCommand(
    const std::string& file_path) const {
    
    clang::tooling::CompileCommand cmd;
    
    // 设置文件路径
    cmd.Filename = file_path;
    
    // 设置工作目录
    cmd.Directory = std::filesystem::current_path().string();
    
    // 构建命令行参数
    std::vector<std::string> command_line;
    
    // 1. 编译器路径
    command_line.push_back(compiler_info_.compiler_path);
    
    // 2. 基础编译参数
    auto base_args = GetBaseCompileArgs();
    command_line.insert(command_line.end(), base_args.begin(), base_args.end());
    
    // 3. 包含路径参数
    auto include_args = GetIncludeArgs();
    command_line.insert(command_line.end(), include_args.begin(), include_args.end());
    
    // 4. 编译器特定参数
    auto compiler_args = GetCompilerSpecificArgs();
    command_line.insert(command_line.end(), compiler_args.begin(), compiler_args.end());
    
    // 5. 额外的编译标志
    command_line.insert(command_line.end(), additional_flags_.begin(), additional_flags_.end());
    
    // 6. 源文件路径
    command_line.push_back(file_path);
    
    cmd.CommandLine = command_line;
    
    LogDebug("为文件 " + file_path + " 创建编译命令，参数数量: " + std::to_string(command_line.size()));
    
    return cmd;
}

std::vector<std::string> DynamicCompilationDatabase::GetBaseCompileArgs() const {
    std::vector<std::string> args;
    
    // C++ 标准
    args.push_back("-std=" + cpp_standard_);
    
    // 只进行语法分析，不生成代码
    args.push_back("-fsyntax-only");
    
    // 不进行拼写检查
    args.push_back("-fno-spell-checking");
    
    // 禁用颜色输出（避免终端控制字符）
    args.push_back("-fno-color-diagnostics");
    
    // 禁用警告（我们只关心语法解析）
    args.push_back("-w");
    
    return args;
}

std::vector<std::string> DynamicCompilationDatabase::GetIncludeArgs() const {
    std::vector<std::string> args;
    
    // 1. Zeus 项目包含路径（最高优先级）
    if (!zeus_include_path_.empty()) {
        args.push_back("-I" + zeus_include_path_);
    }
    
    // 2. 额外的包含路径
    for (const auto& path : additional_include_paths_) {
        args.push_back("-I" + path);
    }
    
    // 3. 系统包含路径（使用 -isystem 避免警告）
    for (const auto& path : compiler_info_.include_paths) {
        args.push_back("-isystem");
        args.push_back(path);
    }
    
    return args;
}

std::vector<std::string> DynamicCompilationDatabase::GetCompilerSpecificArgs() const {
    std::vector<std::string> args;
    
    if (compiler_info_.type == "clang") {
        // Clang 特定参数
        args.push_back("-Wno-unknown-warning-option");
        args.push_back("-Wno-unused-command-line-argument");
        
    } else if (compiler_info_.type == "gcc") {
        // GCC 特定参数
        args.push_back("-Wno-unknown-pragmas");
        
    } else if (compiler_info_.type == "msvc") {
        // MSVC 特定参数
        args.push_back("/nologo");
        args.push_back("/EHsc");
    }
    
    return args;
}

std::string DynamicCompilationDatabase::NormalizeFilePath(const std::string& file_path) const {
    try {
        return std::filesystem::canonical(file_path).string();
    } catch (const std::exception&) {
        // 如果无法标准化，返回原始路径
        return file_path;
    }
}

void DynamicCompilationDatabase::LogDebug(const std::string& message) const {
    if (verbose_) {
        std::cout << "[DynamicCompilationDatabase] " << message << std::endl;
    }
}

bool DynamicCompilationDatabase::ValidateCompileCommand(
    const clang::tooling::CompileCommand& cmd) const {
    
    // 检查基本字段
    if (cmd.Filename.empty() || cmd.Directory.empty() || cmd.CommandLine.empty()) {
        LogDebug("编译命令基本字段验证失败");
        return false;
    }
    
    // 检查编译器路径是否存在
    if (!std::filesystem::exists(cmd.CommandLine[0])) {
        LogDebug("编译器路径不存在: " + cmd.CommandLine[0]);
        return false;
    }
    
    // 检查源文件是否存在
    if (!std::filesystem::exists(cmd.Filename)) {
        LogDebug("源文件不存在: " + cmd.Filename);
        return false;
    }
    
    return true;
}

// 便利函数实现

std::unique_ptr<DynamicCompilationDatabase> CreateDynamicCompilationDatabase(
    const std::vector<std::string>& source_files,
    const std::string& zeus_include_path,
    const CompilerDetector::CompilerInfo& compiler_info,
    const std::vector<std::string>& additional_flags) {
    
    return std::make_unique<DynamicCompilationDatabase>(
        source_files, zeus_include_path, compiler_info, additional_flags);
}

} // namespace lua_binding_generator