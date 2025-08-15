/**
 * @file main.cpp
 * @brief 零配置 lua_binding_generator 主程序
 * 
 * 新版本特性：
 * 1. 零配置：无需 compile_commands.json 或任何配置文件
 * 2. 动态编译器检测：自动检测系统编译环境
 * 3. 简洁命令行：支持用户友好的命令行接口
 * 4. 跨平台支持：支持 Windows/macOS/Linux
 */

#include <clang/Tooling/Tooling.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/ASTConsumer.h>
#include <llvm/Support/CommandLine.h>

#include "lua_binding_generator/compiler_detector.h"
#include "lua_binding_generator/dynamic_compilation_database.h"
#include "lua_binding_generator/direct_binding_generator.h"
#include "lua_binding_generator/smart_inference_engine.h"
#include "lua_binding_generator/incremental_generator.h"
#include "lua_binding_generator/ast_visitor.h"

#include <iostream>
#include <memory>
#include <filesystem>
#include <chrono>
#include <fstream>
#include <algorithm>
#include <sstream>

using namespace clang;
using namespace clang::tooling;
using namespace llvm;
using namespace lua_binding_generator;

// 全局变量，用于在 AST 处理完成后保存数据
static std::vector<InferredExportInfo> g_inferred_exports;

// ================================
// 命令行参数结构
// ================================

/**
 * @brief 命令行参数结构
 */
struct CommandLineArgs {
    std::vector<std::string> source_files;      ///< 源文件列表
    std::string input_dir;                      ///< 输入目录
    std::string output_dir = "generated_bindings"; ///< 输出目录
    std::vector<std::string> exclude_files;     ///< 排除的文件
    std::vector<std::string> include_paths;     ///< 额外的包含路径
    std::string module_name;                    ///< 模块名称
    std::string compiler_path;                  ///< 指定编译器路径
    bool verbose = false;                       ///< 详细输出
    bool help = false;                          ///< 显示帮助
    bool show_stats = false;                    ///< 显示统计信息
    bool force_rebuild = false;                 ///< 强制重新构建
};

// ================================
// 文件收集器
// ================================

/**
 * @brief 文件收集器
 */
class ZeusFileCollector {
public:
    /**
     * @brief 收集指定目录中的源文件
     * @param input_dir 输入目录
     * @param exclude_patterns 排除模式列表
     * @return 源文件路径列表
     */
    std::vector<std::string> CollectFiles(const std::string& input_dir,
                                         const std::vector<std::string>& exclude_patterns) {
        std::vector<std::string> files;
        
        if (!std::filesystem::exists(input_dir)) {
            std::cerr << "错误: 输入目录不存在: " << input_dir << std::endl;
            return files;
        }

        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(input_dir)) {
                if (entry.is_regular_file()) {
                    std::string file_path = entry.path().string();
                    std::string extension = entry.path().extension().string();
                    
                    // 只处理 C++ 头文件
                    if (extension == ".h" || extension == ".hpp" || extension == ".hxx") {
                        if (!ShouldExcludeFile(file_path, exclude_patterns)) {
                            files.push_back(file_path);
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "错误: 遍历目录时出错: " << e.what() << std::endl;
        }

        return files;
    }

    /**
     * @brief 检查文件是否应该被排除
     * @param file_path 文件路径
     * @param exclude_patterns 排除模式列表
     * @return 是否应该排除
     */
    bool ShouldExcludeFile(const std::string& file_path,
                          const std::vector<std::string>& exclude_patterns) {
        std::string filename = std::filesystem::path(file_path).filename().string();
        
        for (const auto& pattern : exclude_patterns) {
            // 简单的模式匹配（支持通配符 * 和 ?）
            if (MatchPattern(filename, pattern)) {
                return true;
            }
        }
        
        return false;
    }

private:
    /**
     * @brief 简单的通配符模式匹配
     * @param text 文本
     * @param pattern 模式
     * @return 是否匹配
     */
    bool MatchPattern(const std::string& text, const std::string& pattern) {
        // 简化实现：只支持精确匹配和 * 通配符
        if (pattern == "*") {
            return true;
        }
        
        if (pattern.find('*') == std::string::npos) {
            return text == pattern;
        }
        
        // 简单的前缀/后缀匹配
        if (pattern.front() == '*') {
            std::string suffix = pattern.substr(1);
            return text.size() >= suffix.size() && 
                   text.substr(text.size() - suffix.size()) == suffix;
        }
        
        if (pattern.back() == '*') {
            std::string prefix = pattern.substr(0, pattern.size() - 1);
            return text.size() >= prefix.size() && 
                   text.substr(0, prefix.size()) == prefix;
        }
        
        return text == pattern;
    }
};

// ================================
// AST 处理器（保持原有逻辑）
// ================================

/**
 * @brief 智能 AST 消费者
 */
class SmartASTConsumer : public ASTConsumer {
public:
    explicit SmartASTConsumer(ASTContext* context, const SmartInferenceEngine::InferenceOptions& options, const std::string& module_name = "")
        : ast_context_(context), inference_options_(options), module_name_(module_name) {
        // 延迟创建推导引擎，使用有效的 ASTContext
        inference_engine_ = std::make_unique<SmartInferenceEngine>(context);
        inference_engine_->SetOptions(options);
        if (!module_name_.empty()) {
            inference_engine_->SetFileModule(module_name_);
        }
        inference_engine_->ClearErrors();
    }
    
    void HandleTranslationUnit(ASTContext& context) override {
        // 遍历顶层声明
        int decl_count = 0;
        int main_file_decls = 0;
        for (auto decl : context.getTranslationUnitDecl()->decls()) {
            decl_count++;
            
            // 主文件声明统计
            if (auto named_decl = dyn_cast<NamedDecl>(decl)) {
                auto& source_manager = decl->getASTContext().getSourceManager();
                auto loc = decl->getLocation();
                
                if (loc.isValid() && source_manager.isInMainFile(loc)) {
                    main_file_decls++;
                }
            }
            
            ProcessDeclarationRecursively(decl);
        }
        
        // 立即将推导结果保存到全局变量，避免在 Tool.run() 后访问无效指针
        g_inferred_exports.insert(g_inferred_exports.end(), 
                                 inferred_exports_.begin(), 
                                 inferred_exports_.end());
        
        if (decl_count > 0) {
            std::cout << "✅ AST 处理完成，共处理 " << decl_count << " 个声明，主文件声明 " << main_file_decls << " 个" << std::endl;
        }
    }

private:
    ASTContext* ast_context_;
    SmartInferenceEngine::InferenceOptions inference_options_;
    std::string module_name_;
    std::unique_ptr<SmartInferenceEngine> inference_engine_;
    std::vector<InferredExportInfo> inferred_exports_;
    
    void ProcessDeclarationRecursively(Decl* decl) {
        if (!decl) {
            return;
        }
        
        // 首先处理当前声明
        ProcessDeclaration(decl);
        
        // 如果是命名空间或记录类型，递归处理其内部声明
        if (auto ns_decl = dyn_cast<NamespaceDecl>(decl)) {
            for (auto inner_decl : ns_decl->decls()) {
                ProcessDeclarationRecursively(inner_decl);
            }
        } else if (auto record_decl = dyn_cast<CXXRecordDecl>(decl)) {
            for (auto inner_decl : record_decl->decls()) {
                ProcessDeclarationRecursively(inner_decl);
            }
        }
    }
    
    void ProcessDeclaration(Decl* decl) {
        if (!decl) {
            return;
        }
        
        // 声明处理调试已禁用
        
        if (!ShouldProcessDeclaration(decl)) {
            return;
        }
        
        // 检查是否有导出注解
        if (!HasExportAnnotation(decl)) {
            return;
        }
        
        // 导出项调试已禁用
        
        try {
            if (auto class_decl = dyn_cast<CXXRecordDecl>(decl)) {
                ProcessClassDeclaration(class_decl);
            } else if (auto func_decl = dyn_cast<FunctionDecl>(decl)) {
                ProcessFunctionDeclaration(func_decl);
            } else if (auto enum_decl = dyn_cast<EnumDecl>(decl)) {
                ProcessEnumDeclaration(enum_decl);
            } else if (auto var_decl = dyn_cast<VarDecl>(decl)) {
                ProcessVariableDeclaration(var_decl);
            }
        } catch (const std::exception& e) {
            // 记录错误但继续处理
            std::cerr << "处理声明时出错: " << e.what() << std::endl;
        }
    }
    
    void ProcessClassDeclaration(CXXRecordDecl* class_decl) {
        if (class_decl->isCompleteDefinition()) {
            auto annotation = GetExportAnnotation(class_decl);
            auto class_info = inference_engine_->InferFromClass(class_decl, annotation);
            inferred_exports_.push_back(class_info);
            
            // 如果是 EXPORT_LUA_CLASS，自动处理所有公共成员
            if (annotation.find("lua_export_class") == 0) {
                auto members = inference_engine_->InferClassMembers(class_decl);
                inferred_exports_.insert(inferred_exports_.end(), members.begin(), members.end());
                
                // 智能推导属性
                auto properties = inference_engine_->InferProperties(members);
                inferred_exports_.insert(inferred_exports_.end(), properties.begin(), properties.end());
            }
        }
    }
    
    void ProcessFunctionDeclaration(FunctionDecl* func_decl) {
        auto annotation = GetExportAnnotation(func_decl);
        auto func_info = inference_engine_->InferFromFunction(func_decl, annotation);
        inferred_exports_.push_back(func_info);
    }
    
    void ProcessEnumDeclaration(EnumDecl* enum_decl) {
        auto annotation = GetExportAnnotation(enum_decl);
        auto enum_info = inference_engine_->InferFromEnum(enum_decl, annotation);
        inferred_exports_.push_back(enum_info);
    }
    
    void ProcessVariableDeclaration(VarDecl* var_decl) {
        auto annotation = GetExportAnnotation(var_decl);
        auto var_info = inference_engine_->InferFromVariable(var_decl, annotation);
        inferred_exports_.push_back(var_info);
    }
    
    bool ShouldProcessDeclaration(Decl* decl) {
        // 只处理在主文件中的声明
        auto& source_manager = decl->getASTContext().getSourceManager();
        auto loc = decl->getLocation();
        
        if (loc.isInvalid()) {
            return false;
        }
        
        return source_manager.isInMainFile(loc);
    }
    
    bool HasExportAnnotation(Decl* decl) {
        if (!decl->hasAttrs()) {
            return false;
        }
        
        for (const auto& attr : decl->attrs()) {
            if (auto annotate_attr = dyn_cast<AnnotateAttr>(attr)) {
                std::string annotation = annotate_attr->getAnnotation().str();
                
                // 注解调试已禁用
                
                if (annotation.find("lua_export_") == 0) {
                    return true;
                }
            }
        }
        
        return false;
    }
    
    std::string GetExportAnnotation(Decl* decl) {
        if (!decl->hasAttrs()) {
            return "";
        }
        
        for (const auto& attr : decl->attrs()) {
            if (auto annotate_attr = dyn_cast<AnnotateAttr>(attr)) {
                std::string annotation = annotate_attr->getAnnotation().str();
                if (annotation.find("lua_export_") == 0) {
                    return annotation;
                }
            }
        }
        
        return "";
    }

public:
    const std::vector<InferredExportInfo>& GetInferredExports() const {
        return inferred_exports_;
    }
    
    void SaveInferredExports(std::vector<InferredExportInfo>& output) {
        output = std::move(inferred_exports_);
    }
};

/**
 * @brief 智能前端动作
 */
class SmartFrontendAction : public ASTFrontendAction {
public:
    explicit SmartFrontendAction(const SmartInferenceEngine::InferenceOptions& options, const std::string& module_name = "")
        : inference_options_(options), module_name_(module_name) {}
    
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& compiler, StringRef file) override {
        auto consumer = std::make_unique<SmartASTConsumer>(&compiler.getASTContext(), inference_options_, module_name_);
        consumer_ = consumer.get();
        return std::move(consumer);
    }
    
    SmartASTConsumer* GetConsumer() const {
        return consumer_;
    }

private:
    SmartInferenceEngine::InferenceOptions inference_options_;
    std::string module_name_;
    SmartASTConsumer* consumer_ = nullptr;
};

/**
 * @brief 智能动作工厂
 */
class SmartActionFactory : public FrontendActionFactory {
public:
    explicit SmartActionFactory(const SmartInferenceEngine::InferenceOptions& options, const std::string& module_name = "")
        : inference_options_(options), module_name_(module_name) {}
    
    std::unique_ptr<FrontendAction> create() override {
        auto action = std::make_unique<SmartFrontendAction>(inference_options_, module_name_);
        last_action_ = action.get();
        return std::move(action);
    }
    
    SmartFrontendAction* GetLastAction() const {
        return last_action_;
    }
    
    const std::vector<InferredExportInfo>& GetInferredExports() const {
        return stored_exports_;
    }
    
    void StoreExports() {
        if (last_action_ && last_action_->GetConsumer()) {
            last_action_->GetConsumer()->SaveInferredExports(stored_exports_);
        }
    }
    
    // 从外部直接添加导出信息（避免指针失效问题）
    void AddExportInfo(const InferredExportInfo& info) {
        stored_exports_.push_back(info);
    }

private:
    SmartInferenceEngine::InferenceOptions inference_options_;
    std::string module_name_;
    SmartFrontendAction* last_action_ = nullptr;
    std::vector<InferredExportInfo> stored_exports_;
};

// ================================
// 命令行解析
// ================================

/**
 * @brief 解析命令行参数
 */
CommandLineArgs ParseCommandLine(int argc, char** argv) {
    CommandLineArgs args;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            args.help = true;
        } else if (arg == "--verbose" || arg == "-v") {
            args.verbose = true;
        } else if (arg == "--show-stats") {
            args.show_stats = true;
        } else if (arg == "--force-rebuild") {
            args.force_rebuild = true;
        } else if (arg.substr(0, 13) == "--output_dir=") {
            args.output_dir = arg.substr(13);
        } else if (arg.substr(0, 12) == "--input_dir=") {
            args.input_dir = arg.substr(12);
        } else if (arg.substr(0, 16) == "--exclude_files=") {
            std::string exclude_list = arg.substr(16);
            std::istringstream iss(exclude_list);
            std::string file;
            while (std::getline(iss, file, ',')) {
                if (!file.empty()) {
                    args.exclude_files.push_back(file);
                }
            }
        } else if (arg.substr(0, 10) == "--include=") {
            args.include_paths.push_back(arg.substr(10));
        } else if (arg.substr(0, 14) == "--module-name=") {
            args.module_name = arg.substr(14);
        } else if (arg.substr(0, 11) == "--compiler=") {
            args.compiler_path = arg.substr(11);
        } else if (arg.substr(0, 2) != "--") {
            // 源文件
            args.source_files.push_back(arg);
        } else {
            std::cerr << "警告: 未知参数: " << arg << std::endl;
        }
    }
    
    return args;
}

// ================================
// 转换和生成函数
// ================================

/**
 * @brief 将 InferredExportInfo 转换为 ExportInfo
 */
ExportInfo ConvertToExportInfo(const InferredExportInfo& inferred) {
    ExportInfo info;
    
    // 基础信息映射
    info.name = inferred.cpp_name;
    info.lua_name = inferred.lua_name.empty() ? inferred.cpp_name : inferred.lua_name;
    info.qualified_name = inferred.qualified_name;
    info.source_file = inferred.source_file;
    info.file_path = inferred.source_file;
    
    // 类型信息映射
    info.type_name = inferred.type_name;
    info.return_type = inferred.return_type;
    info.parameter_types = inferred.parameter_types;
    info.export_type = inferred.export_type;
    
    // 访问和属性信息
    info.is_static = inferred.is_static;
    info.is_const = inferred.is_const;
    info.is_virtual = inferred.is_virtual;
    info.property_access = inferred.property_access;
    
    // 命名空间和模块信息
    info.namespace_name = inferred.cpp_namespace;
    info.module_name = inferred.module_name;
    info.parent_class = inferred.parent_class;
    info.owner_class = inferred.parent_class;
    info.base_classes = inferred.base_classes;
    
    // 根据 export_type 确定 ExportInfo::Type
    if (inferred.export_type.find("class") != std::string::npos) {
        info.type = ExportInfo::Type::Class;
    } else if (inferred.export_type.find("function") != std::string::npos) {
        info.type = ExportInfo::Type::Function;
    } else if (inferred.export_type.find("method") != std::string::npos) {
        if (inferred.is_static) {
            info.type = ExportInfo::Type::StaticMethod;
        } else {
            info.type = ExportInfo::Type::Method;
        }
    } else if (inferred.export_type.find("enum") != std::string::npos) {
        info.type = ExportInfo::Type::Enum;
    } else if (inferred.export_type.find("constant") != std::string::npos || inferred.export_type.find("variable") != std::string::npos) {
        info.type = ExportInfo::Type::Constant;
    } else if (inferred.export_type.find("property") != std::string::npos) {
        info.type = ExportInfo::Type::Property;
    } else if (inferred.export_type.find("operator") != std::string::npos) {
        info.type = ExportInfo::Type::Operator;
    } else {
        // 默认类型
        info.type = ExportInfo::Type::Function;
    }
    
    // 访问类型映射
    if (inferred.property_access == "readonly" || inferred.variable_access == "readonly") {
        info.access_type = ExportInfo::AccessType::ReadOnly;
    } else if (inferred.property_access == "readwrite" || inferred.variable_access == "readwrite") {
        info.access_type = ExportInfo::AccessType::ReadWrite;
    } else {
        info.access_type = ExportInfo::AccessType::None;
    }
    
    return info;
}

/**
 * @brief 创建输出目录
 */
bool CreateOutputDirectory(const std::string& output_dir, bool verbose = false) {
    try {
        if (std::filesystem::exists(output_dir)) {
            if (verbose) {
                std::cout << "📁 输出目录已存在: " << output_dir << std::endl;
            }
            return true;
        }
        
        if (std::filesystem::create_directories(output_dir)) {
            if (verbose) {
                std::cout << "📁 创建输出目录: " << output_dir << std::endl;
            }
            return true;
        }
    } catch (const std::exception& e) {
        std::cerr << "❌ 创建输出目录失败: " << e.what() << std::endl;
    }
    
    return false;
}

// ================================
// 工具函数
// ================================

/**
 * @brief 获取 Zeus 项目包含路径
 */
std::string GetZeusIncludePath(const std::string& executable_path) {
    // 从可执行文件路径推导项目根目录
    std::filesystem::path exe_path(executable_path);
    std::filesystem::path project_root = exe_path.parent_path().parent_path().parent_path().parent_path();
    std::filesystem::path include_path = project_root / "include";
    
    if (std::filesystem::exists(include_path)) {
        return include_path.string();
    }
    
    // 如果推导失败，尝试当前目录往上找
    std::filesystem::path current_path = std::filesystem::current_path();
    
    // 从当前目录开始，向上查找包含 include/common/lua/export_macros.h 的目录
    for (int i = 0; i < 5; ++i) {
        std::filesystem::path test_include = current_path / "include";
        std::filesystem::path export_macros = test_include / "common" / "lua" / "export_macros.h";
        
        if (std::filesystem::exists(export_macros)) {
            return test_include.string();
        }
        
        current_path = current_path.parent_path();
        if (current_path.empty() || current_path == current_path.parent_path()) {
            break;
        }
    }
    
    return "";
}

/**
 * @brief 收集源文件
 */
std::vector<std::string> CollectSourceFiles(const CommandLineArgs& args) {
    std::vector<std::string> source_files;
    
    if (!args.input_dir.empty()) {
        // 目录模式
        ZeusFileCollector collector;
        source_files = collector.CollectFiles(args.input_dir, args.exclude_files);
    } else {
        // 文件列表模式
        source_files = args.source_files;
    }
    
    // 验证文件存在性
    std::vector<std::string> valid_files;
    for (const auto& file : source_files) {
        if (std::filesystem::exists(file)) {
            valid_files.push_back(file);
        } else {
            std::cerr << "警告: 文件不存在: " << file << std::endl;
        }
    }
    
    return valid_files;
}

/**
 * @brief 显示用法帮助
 */
void PrintUsage(const char* program_name) {
    std::cout << "Lua Binding Generator - 零配置 C++ 到 Lua 绑定工具\n\n";
    
    std::cout << "用法:\n";
    std::cout << "  " << program_name << " file1.h file2.h ... [选项]\n";
    std::cout << "  " << program_name << " --input_dir=<目录> [选项]\n\n";
    
    std::cout << "选项:\n";
    std::cout << "  --help, -h              显示此帮助信息\n";
    std::cout << "  --verbose, -v           启用详细输出\n";
    std::cout << "  --output_dir=<目录>     输出目录 (默认: generated_bindings)\n";
    std::cout << "  --input_dir=<目录>      输入目录（递归搜索 .h 文件）\n";
    std::cout << "  --exclude_files=<列表>  排除文件列表（逗号分隔）\n";
    std::cout << "  --include=<路径>        额外的包含路径\n";
    std::cout << "  --module-name=<名称>    模块名称\n";
    std::cout << "  --compiler=<路径>       指定编译器路径\n";
    std::cout << "  --show-stats            显示生成统计信息\n";
    std::cout << "  --force-rebuild         强制重新构建\n\n";
    
    std::cout << "示例:\n";
    std::cout << "  " << program_name << " examples/real_test.h --output_dir=bindings\n";
    std::cout << "  " << program_name << " --input_dir=src/game --exclude_files=internal.h,debug.h\n";
    std::cout << "  " << program_name << " game.h player.h --module-name=GameCore --verbose\n\n";
    
    std::cout << "注意:\n";
    std::cout << "  此工具需要 C++ 编译环境，支持 Clang、GCC 或 MSVC。\n";
    std::cout << "  如果未检测到编译器，请确保已安装并在 PATH 环境变量中。\n";
}

/**
 * @brief 显示编译器未找到错误
 */
void PrintCompilerNotFoundError() {
    std::cerr << "❌ 错误: 未找到可用的 C++ 编译器\n\n";
    std::cerr << "lua_binding_generator 需要 C++ 编译环境来解析源代码。\n";
    std::cerr << "请安装以下任一编译器：\n\n";
    
#ifdef __APPLE__
    std::cerr << "📱 macOS:\n";
    std::cerr << "  • Xcode Command Line Tools (推荐)\n";
    std::cerr << "    xcode-select --install\n";
    std::cerr << "  • Homebrew LLVM\n";
    std::cerr << "    brew install llvm\n\n";
#elif __linux__
    std::cerr << "🐧 Linux:\n";
    std::cerr << "  • Ubuntu/Debian: sudo apt install clang\n";
    std::cerr << "  • CentOS/RHEL: sudo yum install clang\n";
    std::cerr << "  • Arch Linux: sudo pacman -S clang\n\n";
#elif _WIN32
    std::cerr << "🪟 Windows:\n";
    std::cerr << "  • Visual Studio (推荐)\n";
    std::cerr << "    https://visualstudio.microsoft.com/\n";
    std::cerr << "  • LLVM for Windows\n";
    std::cerr << "    https://releases.llvm.org/\n";
    std::cerr << "  • MinGW-w64\n";
    std::cerr << "    https://www.mingw-w64.org/\n\n";
#endif

    std::cerr << "安装完成后，请确保编译器在 PATH 环境变量中。\n";
}

// ================================
// 主函数
// ================================

int main(int argc, char** argv) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 解析命令行参数
    auto args = ParseCommandLine(argc, argv);
    
    if (args.help) {
        PrintUsage(argv[0]);
        return 0;
    }
    
    // 收集源文件
    std::vector<std::string> source_files = CollectSourceFiles(args);
    
    if (source_files.empty()) {
        std::cerr << "错误: 未指定源文件\n";
        PrintUsage(argv[0]);
        return 1;
    }
    
    // 1. 检测编译环境
    CompilerDetector detector;
    detector.SetVerbose(args.verbose);
    
    CompilerDetector::CompilerInfo compiler_info;
    if (!args.compiler_path.empty()) {
        // 使用指定的编译器
        compiler_info = detector.UseCompiler(args.compiler_path);
    } else {
        // 自动检测编译器
        compiler_info = detector.DetectCompiler();
    }
    
    if (!compiler_info.found) {
        PrintCompilerNotFoundError();
        return 1;
    }
    
    if (args.verbose) {
        std::cout << "🔧 检测到编译器: " << compiler_info.type 
                  << " " << compiler_info.version << std::endl;
        std::cout << "📍 编译器路径: " << compiler_info.compiler_path << std::endl;
        std::cout << "📦 系统包含路径: " << compiler_info.include_paths.size() << " 个" << std::endl;
    }
    
    // 2. 获取 Zeus 包含路径
    std::string zeus_include_path = GetZeusIncludePath(argv[0]);
    if (zeus_include_path.empty()) {
        std::cerr << "警告: 无法自动检测 Zeus 项目包含路径" << std::endl;
    } else if (args.verbose) {
        std::cout << "🎯 Zeus 包含路径: " << zeus_include_path << std::endl;
    }
    
    // 3. 创建动态编译数据库
    auto compilation_db = CreateDynamicCompilationDatabase(
        source_files, zeus_include_path, compiler_info);
    
    compilation_db->SetVerbose(args.verbose);
    
    // 添加额外的包含路径
    for (const auto& include_path : args.include_paths) {
        compilation_db->AddIncludePath(include_path);
    }
    
    if (args.verbose) {
        std::cout << "🗃️  编译数据库信息:\n" << compilation_db->GetDebugInfo() << std::endl;
    }
    
    // 4. 创建 ClangTool
    ClangTool tool(*compilation_db, source_files);
    
    // 5. 初始化推导引擎选项
    SmartInferenceEngine::InferenceOptions inference_options;
    inference_options.auto_infer_namespaces = true;
    inference_options.auto_infer_properties = true;
    inference_options.auto_infer_stl_containers = true;
    inference_options.auto_infer_callbacks = true;
    inference_options.prefer_snake_case = false;
    inference_options.default_namespace = "global";
    
    // 6. 运行 AST 分析
    SmartActionFactory action_factory(inference_options, args.module_name);
    
    if (args.verbose) {
        std::cout << "🔍 开始分析源文件..." << std::endl;
    }
    
    int parse_result = tool.run(&action_factory);
    if (parse_result != 0) {
        std::cerr << "❌ 源文件解析失败" << std::endl;
        return parse_result;
    }
    
    // 7. 使用全局变量中的推导结果（避免访问无效指针）
    if (args.verbose) {
        std::cout << "📊 发现 " << g_inferred_exports.size() << " 个导出项" << std::endl;
    }
    
    if (g_inferred_exports.empty()) {
        std::cout << "⚠️  未找到任何标记为导出的项目" << std::endl;
        std::cout << "💡 确保使用了 EXPORT_LUA_* 宏标记要导出的代码" << std::endl;
        return 0;
    }
    
    // 8. 生成绑定代码
    std::cout << "✅ 解析完成！共发现 " << g_inferred_exports.size() << " 个导出项" << std::endl;
    
    if (args.verbose) {
        std::cout << "🔄 开始代码生成..." << std::endl;
    }
    
    // 创建输出目录
    if (!CreateOutputDirectory(args.output_dir, args.verbose)) {
        std::cerr << "❌ 无法创建输出目录: " << args.output_dir << std::endl;
        return 1;
    }
    
    // 转换推导结果为代码生成器格式
    std::vector<ExportInfo> export_items;
    export_items.reserve(g_inferred_exports.size());
    
    for (const auto& inferred : g_inferred_exports) {
        export_items.push_back(ConvertToExportInfo(inferred));
    }
    
    if (args.verbose) {
        std::cout << "🔄 已转换 " << export_items.size() << " 个导出项" << std::endl;
    }
    
    // 配置代码生成器
    DirectBindingGenerator generator;
    DirectBindingGenerator::GenerationOptions gen_options;
    gen_options.output_directory = args.output_dir;
    gen_options.generate_includes = true;
    gen_options.generate_registration_function = true;
    gen_options.use_namespace_tables = true;
    gen_options.indent_size = 4;
    
    // 使用模块名称，如果未指定则使用源文件名
    std::string module_name = args.module_name;
    if (module_name.empty() && !source_files.empty()) {
        // 从第一个源文件提取模块名
        std::filesystem::path first_file(source_files[0]);
        module_name = first_file.stem().string();
        // 清理文件名，使其适合作为模块名
        std::replace_if(module_name.begin(), module_name.end(), 
                       [](char c) { return !std::isalnum(c); }, '_');
    }
    if (module_name.empty()) {
        module_name = "GeneratedBindings";
    }
    
    generator.SetOptions(gen_options);
    
    // 生成绑定代码
    auto result = generator.GenerateModuleBinding(module_name, export_items);
    
    if (!result.success) {
        std::cerr << "❌ 代码生成失败" << std::endl;
        for (const auto& error : result.errors) {
            std::cerr << "   " << error << std::endl;
        }
        return 1;
    }
    
    // 写入生成的代码到文件
    std::string output_filename = module_name + "_bindings.cpp";
    std::string output_path = args.output_dir + "/" + output_filename;
    
    try {
        std::ofstream output_file(output_path);
        if (!output_file.is_open()) {
            std::cerr << "❌ 无法创建输出文件: " << output_path << std::endl;
            return 1;
        }
        
        output_file << result.generated_code;
        output_file.close();
        
        std::cout << "✅ 代码生成完成！" << std::endl;
        std::cout << "📄 输出文件: " << output_path << std::endl;
        std::cout << "📊 生成了 " << result.total_bindings << " 个绑定" << std::endl;
        
        if (args.show_stats) {
            std::cout << "\n📈 详细统计:" << std::endl;
            std::cout << "   - 导出项总数: " << g_inferred_exports.size() << std::endl;
            std::cout << "   - 绑定总数: " << result.total_bindings << std::endl;
            std::cout << "   - 输出文件: " << output_filename << std::endl;
            if (!result.warnings.empty()) {
                std::cout << "   - 警告数量: " << result.warnings.size() << std::endl;
            }
        }
        
        if (!result.warnings.empty()) {
            std::cout << "\n⚠️  警告信息:" << std::endl;
            for (const auto& warning : result.warnings) {
                std::cout << "   " << warning << std::endl;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ 文件写入失败: " << e.what() << std::endl;
        return 1;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    if (args.verbose) {
        std::cout << "🎉 总耗时: " << total_time.count() << " 毫秒" << std::endl;
    }
    
    return 0;
}