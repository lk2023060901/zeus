/**
 * @file main_v2.cpp
 * @brief 重构后的 lua_binding_generator 主程序 - 版本 2.0
 * 
 * 新版本特性：
 * 1. 摒弃模板系统，使用硬编码生成器
 * 2. 极简化的宏系统，智能推导减少用户负担
 * 3. 增量编译支持，只重新生成变更的文件
 * 4. 改进的命令行界面和配置文件支持
 * 5. 更好的错误处理和性能优化
 */

#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/ASTConsumer.h>
#include <llvm/Support/CommandLine.h>

#include "lua_binding_generator/direct_binding_generator.h"
#include "lua_binding_generator/smart_inference_engine.h"
#include "lua_binding_generator/incremental_generator.h"
#include "lua_binding_generator/ast_visitor.h"

#include <iostream>
#include <memory>
#include <filesystem>
#include <chrono>
#include <fstream>

using namespace clang;
using namespace clang::tooling;
using namespace llvm;
using namespace lua_binding_generator;

// ================================
// 命令行选项定义
// ================================

static cl::OptionCategory LuaBindingGeneratorCategory("Lua Binding Generator v2.0 Options");

static cl::opt<std::string> OutputDirectory(
    "output-dir",
    cl::desc("Output directory for generated bindings"),
    cl::value_desc("directory"),
    cl::init("generated_bindings"),
    cl::cat(LuaBindingGeneratorCategory)
);

static cl::opt<std::string> ModuleName(
    "module-name",
    cl::desc("Name of the Lua module to generate"),
    cl::value_desc("name"),
    cl::init(""),
    cl::cat(LuaBindingGeneratorCategory)
);

static cl::opt<std::string> ConfigFile(
    "config",
    cl::desc("Configuration file path"),
    cl::value_desc("file"),
    cl::init(""),
    cl::cat(LuaBindingGeneratorCategory)
);

static cl::opt<bool> EnableIncremental(
    "incremental",
    cl::desc("Enable incremental compilation"),
    cl::init(true),
    cl::cat(LuaBindingGeneratorCategory)
);

static cl::opt<bool> ForceRebuild(
    "force-rebuild",
    cl::desc("Force rebuild all files (ignore cache)"),
    cl::init(false),
    cl::cat(LuaBindingGeneratorCategory)
);

static cl::opt<bool> EnableParallel(
    "parallel",
    cl::desc("Enable parallel processing"),
    cl::init(true),
    cl::cat(LuaBindingGeneratorCategory)
);

static cl::opt<int> MaxThreads(
    "max-threads",
    cl::desc("Maximum number of threads (0 = auto)"),
    cl::init(0),
    cl::cat(LuaBindingGeneratorCategory)
);

static cl::opt<bool> Verbose(
    "verbose",
    cl::desc("Enable verbose output"),
    cl::init(false),
    cl::cat(LuaBindingGeneratorCategory)
);

static cl::opt<bool> PreferSnakeCase(
    "snake-case",
    cl::desc("Prefer snake_case naming in Lua"),
    cl::init(false),
    cl::cat(LuaBindingGeneratorCategory)
);

static cl::opt<std::string> DefaultNamespace(
    "default-namespace",
    cl::desc("Default namespace for exports"),
    cl::value_desc("namespace"),
    cl::init("global"),
    cl::cat(LuaBindingGeneratorCategory)
);

static cl::opt<bool> ShowStats(
    "stats",
    cl::desc("Show generation statistics"),
    cl::init(false),
    cl::cat(LuaBindingGeneratorCategory)
);

// ================================
// 配置管理
// ================================

/**
 * @brief 配置结构
 */
struct GeneratorConfig {
    std::string output_dir = "generated_bindings";
    std::string module_name = "";
    std::string default_namespace = "global";
    bool enable_incremental = true;
    bool force_rebuild = false;
    bool enable_parallel = true;
    int max_threads = 0;
    bool verbose = false;
    bool prefer_snake_case = false;
    bool show_stats = false;
    
    // 生成选项
    bool generate_includes = true;
    bool generate_registration_function = true;
    bool use_namespace_tables = true;
    int indent_size = 4;
    
    // 推导选项
    bool auto_infer_namespaces = true;
    bool auto_infer_properties = true;
    bool auto_infer_stl_containers = true;
    bool auto_infer_callbacks = true;
    
    // 增量编译选项
    std::string cache_file = ".lua_binding_cache";
    std::chrono::seconds cache_expiry{3600};
};

/**
 * @brief 从命令行加载配置
 */
GeneratorConfig LoadConfigFromCommandLine() {
    GeneratorConfig config;
    
    config.output_dir = OutputDirectory.getValue();
    config.module_name = ModuleName.getValue();
    config.default_namespace = DefaultNamespace.getValue();
    config.enable_incremental = EnableIncremental.getValue();
    config.force_rebuild = ForceRebuild.getValue();
    config.enable_parallel = EnableParallel.getValue();
    config.max_threads = MaxThreads.getValue();
    config.verbose = Verbose.getValue();
    config.prefer_snake_case = PreferSnakeCase.getValue();
    config.show_stats = ShowStats.getValue();
    
    return config;
}

/**
 * @brief 从配置文件加载配置（简化实现）
 */
bool LoadConfigFromFile(const std::string& config_file, GeneratorConfig& config) {
    if (config_file.empty() || !std::filesystem::exists(config_file)) {
        return false;
    }
    
    // 简化的配置文件加载
    // 实际实现应该支持 JSON 或 YAML 格式
    std::ifstream file(config_file);
    if (!file.is_open()) {
        return false;
    }
    
    // TODO: 实现配置文件解析
    return true;
}

// ================================
// AST 处理器
// ================================

/**
 * @brief 智能 AST 消费者
 */
class SmartASTConsumer : public ASTConsumer {
public:
    explicit SmartASTConsumer(ASTContext* context, SmartInferenceEngine* inference_engine)
        : inference_engine_(inference_engine) {
        inference_engine_->ClearErrors();
    }
    
    void HandleTranslationUnit(ASTContext& context) override {
        // 遍历顶层声明
        for (auto decl : context.getTranslationUnitDecl()->decls()) {
            ProcessDeclaration(decl);
        }
    }

private:
    SmartInferenceEngine* inference_engine_;
    std::vector<InferredExportInfo> inferred_exports_;
    
    void ProcessDeclaration(Decl* decl) {
        if (!decl || !ShouldProcessDeclaration(decl)) {
            return;
        }
        
        // 检查是否有导出注解
        if (!HasExportAnnotation(decl)) {
            return;
        }
        
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
};

/**
 * @brief 智能前端动作
 */
class SmartFrontendAction : public ASTFrontendAction {
public:
    explicit SmartFrontendAction(SmartInferenceEngine* inference_engine)
        : inference_engine_(inference_engine) {}
    
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& compiler, StringRef file) override {
        // 设置文件模块名（从文件名推导或使用全局设置）
        if (!ModuleName.getValue().empty()) {
            inference_engine_->SetFileModule(ModuleName.getValue());
        }
        
        consumer_ = std::make_unique<SmartASTConsumer>(&compiler.getASTContext(), inference_engine_);
        return std::unique_ptr<ASTConsumer>(consumer_.get());
    }
    
    SmartASTConsumer* GetConsumer() const {
        return consumer_.get();
    }

private:
    SmartInferenceEngine* inference_engine_;
    std::unique_ptr<SmartASTConsumer> consumer_;
};

/**
 * @brief 智能动作工厂
 */
class SmartActionFactory : public FrontendActionFactory {
public:
    explicit SmartActionFactory(SmartInferenceEngine* inference_engine)
        : inference_engine_(inference_engine) {}
    
    std::unique_ptr<FrontendAction> create() override {
        auto action = std::make_unique<SmartFrontendAction>(inference_engine_);
        last_action_ = action.get();
        return std::move(action);
    }
    
    SmartFrontendAction* GetLastAction() const {
        return last_action_;
    }

private:
    SmartInferenceEngine* inference_engine_;
    SmartFrontendAction* last_action_ = nullptr;
};

// ================================
// 统计信息和进度显示
// ================================

/**
 * @brief 显示生成统计信息
 */
void ShowGenerationStatistics(const IncrementalResult& result, 
                             const IncrementalGenerator::CacheStats& cache_stats,
                             const GeneratorConfig& config) {
    
    std::cout << "\n=== Lua 绑定生成统计 ===" << std::endl;
    std::cout << "✅ 状态: " << (result.success ? "成功" : "失败") << std::endl;
    std::cout << "📁 处理文件: " << result.processed_files.size() << " 个" << std::endl;
    std::cout << "⏭️  跳过文件: " << result.skipped_files.size() << " 个" << std::endl;
    std::cout << "⏱️  耗时: " << result.elapsed_time.count() << " 毫秒" << std::endl;
    
    if (config.enable_incremental) {
        std::cout << "\n--- 增量编译统计 ---" << std::endl;
        std::cout << "💾 缓存命中: " << result.cache_hits << " 次" << std::endl;
        std::cout << "🔍 缓存未命中: " << result.cache_misses << " 次" << std::endl;
        std::cout << "📊 缓存命中率: " << std::fixed << std::setprecision(1) 
                  << (result.cache_hits + result.cache_misses > 0 ? 
                      100.0 * result.cache_hits / (result.cache_hits + result.cache_misses) : 0.0) 
                  << "%" << std::endl;
        std::cout << "💿 缓存大小: " << (cache_stats.cache_size_bytes / 1024.0) << " KB" << std::endl;
    }
    
    if (!result.processed_files.empty()) {
        std::cout << "\n--- 处理的文件 ---" << std::endl;
        for (const auto& file : result.processed_files) {
            std::cout << "  ✓ " << file << std::endl;
        }
    }
    
    if (!result.warnings.empty()) {
        std::cout << "\n--- 警告 (" << result.warnings.size() << ") ---" << std::endl;
        for (const auto& warning : result.warnings) {
            std::cout << "  ⚠️  " << warning << std::endl;
        }
    }
    
    if (!result.errors.empty()) {
        std::cout << "\n--- 错误 (" << result.errors.size() << ") ---" << std::endl;
        for (const auto& error : result.errors) {
            std::cout << "  ❌ " << error << std::endl;
        }
    }
    
    std::cout << "========================" << std::endl;
}

/**
 * @brief 显示使用帮助
 */
void PrintUsage(const char* program_name) {
    std::cout << "Lua Binding Generator v2.0 - 智能化 C++ 到 Lua 绑定工具\n\n";
    std::cout << "用法: " << program_name << " [选项] <源文件...>\n\n";
    std::cout << "特性:\n";
    std::cout << "  • 极简化的宏系统，智能推导减少配置\n";
    std::cout << "  • 增量编译，只重新生成变更的文件\n";
    std::cout << "  • 并行处理，提升大项目生成速度\n";
    std::cout << "  • 硬编码生成器，消除模板解析开销\n\n";
    std::cout << "示例:\n";
    std::cout << "  " << program_name << " src/*.h\n";
    std::cout << "  " << program_name << " --module-name=GameCore --output-dir=bindings src/game/*.h\n";
    std::cout << "  " << program_name << " --force-rebuild --verbose src/**/*.h\n\n";
}

// ================================
// 主函数
// ================================

int main(int argc, const char** argv) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 解析命令行参数
    auto ExpectedParser = CommonOptionsParser::create(argc, argv, LuaBindingGeneratorCategory);
    if (!ExpectedParser) {
        llvm::errs() << "错误: " << ExpectedParser.takeError() << "\n";
        PrintUsage(argv[0]);
        return 1;
    }
    
    CommonOptionsParser& OptionsParser = ExpectedParser.get();
    auto source_files = OptionsParser.getSourcePathList();
    
    if (source_files.empty()) {
        std::cerr << "错误: 未指定源文件\n";
        PrintUsage(argv[0]);
        return 1;
    }
    
    // 加载配置
    auto config = LoadConfigFromCommandLine();
    
    if (!ConfigFile.getValue().empty()) {
        LoadConfigFromFile(ConfigFile.getValue(), config);
    }
    
    if (config.verbose) {
        std::cout << "🚀 Lua Binding Generator v2.0 启动中..." << std::endl;
        std::cout << "📂 输出目录: " << config.output_dir << std::endl;
        std::cout << "📦 模块名: " << (config.module_name.empty() ? "自动推导" : config.module_name) << std::endl;
        std::cout << "🔄 增量编译: " << (config.enable_incremental ? "启用" : "禁用") << std::endl;
        std::cout << "🏃 并行处理: " << (config.enable_parallel ? "启用" : "禁用") << std::endl;
        std::cout << "📝 源文件数: " << source_files.size() << std::endl;
    }
    
    try {
        // 创建输出目录
        std::filesystem::create_directories(config.output_dir);
        
        // 初始化组件
        SmartInferenceEngine::InferenceOptions inference_options;
        inference_options.auto_infer_namespaces = config.auto_infer_namespaces;
        inference_options.auto_infer_properties = config.auto_infer_properties;
        inference_options.auto_infer_stl_containers = config.auto_infer_stl_containers;
        inference_options.auto_infer_callbacks = config.auto_infer_callbacks;
        inference_options.prefer_snake_case = config.prefer_snake_case;
        inference_options.default_namespace = config.default_namespace;
        
        SmartInferenceEngine inference_engine(nullptr);  // ASTContext 将在 AST 处理时设置
        inference_engine.SetOptions(inference_options);
        
        DirectBindingGenerator::GenerationOptions generation_options;
        generation_options.output_directory = config.output_dir;
        generation_options.default_namespace = config.default_namespace;
        generation_options.generate_includes = config.generate_includes;
        generation_options.generate_registration_function = config.generate_registration_function;
        generation_options.use_namespace_tables = config.use_namespace_tables;
        generation_options.indent_size = config.indent_size;
        
        DirectBindingGenerator binding_generator;
        binding_generator.SetOptions(generation_options);
        
        IncrementalGenerator::Options incremental_options;
        incremental_options.cache_file = config.cache_file;
        incremental_options.force_rebuild = config.force_rebuild;
        incremental_options.enable_parallel = config.enable_parallel;
        incremental_options.max_threads = config.max_threads;
        incremental_options.verbose = config.verbose;
        incremental_options.cache_expiry = config.cache_expiry;
        
        IncrementalGenerator incremental_generator(incremental_options);
        
        // 创建 Clang 工具
        ClangTool Tool(OptionsParser.getCompilations(), source_files);
        SmartActionFactory action_factory(&inference_engine);
        
        if (config.verbose) {
            std::cout << "🔍 分析源文件..." << std::endl;
        }
        
        // 运行 AST 分析
        int parse_result = Tool.run(&action_factory);
        if (parse_result != 0) {
            std::cerr << "❌ 源文件解析失败" << std::endl;
            return parse_result;
        }
        
        // 获取推导结果
        auto last_action = action_factory.GetLastAction();
        if (!last_action || !last_action->GetConsumer()) {
            std::cerr << "❌ 无法获取 AST 分析结果" << std::endl;
            return 1;
        }
        
        const auto& inferred_exports = last_action->GetConsumer()->GetInferredExports();
        
        if (config.verbose) {
            std::cout << "📊 发现 " << inferred_exports.size() << " 个导出项" << std::endl;
        }
        
        if (inferred_exports.empty()) {
            std::cout << "⚠️  未找到任何标记为导出的项目" << std::endl;
            std::cout << "💡 确保使用了 EXPORT_LUA_* 宏标记要导出的代码" << std::endl;
            return 0;
        }
        
        // 转换推导结果为生成器格式
        std::vector<ExportInfo> export_infos;
        for (const auto& inferred : inferred_exports) {
            ExportInfo info;
            info.name = inferred.cpp_name;
            info.lua_name = inferred.lua_name;
            info.qualified_name = inferred.qualified_name;
            info.export_type = inferred.export_type;
            info.namespace_name = inferred.lua_namespace;
            info.type_name = inferred.type_name;
            info.parent_class = inferred.parent_class;
            info.base_classes = inferred.base_classes;
            info.source_file = inferred.source_file;
            // ... 其他字段转换
            
            export_infos.push_back(info);
        }
        
        // 执行增量生成
        std::string effective_module_name = config.module_name.empty() ? "GeneratedBindings" : config.module_name;
        
        auto generation_function = [&](const std::string& file_path, std::string& error_msg) -> bool {
            try {
                // 为每个文件生成绑定
                auto result = binding_generator.GenerateModuleBinding(effective_module_name, export_infos);
                
                if (!result.success) {
                    error_msg = "生成失败";
                    for (const auto& error : result.errors) {
                        error_msg += ": " + error;
                    }
                    return false;
                }
                
                // 写入输出文件
                std::string output_file = config.output_dir + "/" + effective_module_name + "_bindings.cpp";
                std::ofstream outfile(output_file);
                if (!outfile.is_open()) {
                    error_msg = "无法创建输出文件: " + output_file;
                    return false;
                }
                
                outfile << result.generated_code;
                
                // 更新增量生成器
                incremental_generator.UpdateFileInfo(file_path, output_file, effective_module_name);
                
                return true;
                
            } catch (const std::exception& e) {
                error_msg = std::string("生成异常: ") + e.what();
                return false;
            }
        };
        
        IncrementalResult result;
        if (config.enable_incremental) {
            if (config.verbose) {
                std::cout << "🔄 执行增量生成..." << std::endl;
            }
            result = incremental_generator.Generate(source_files, generation_function);
        } else {
            if (config.verbose) {
                std::cout << "🔄 执行完整生成..." << std::endl;
            }
            // 强制重新生成所有文件
            IncrementalGenerator::Options temp_options = incremental_options;
            temp_options.force_rebuild = true;
            IncrementalGenerator temp_generator(temp_options);
            result = temp_generator.Generate(source_files, generation_function);
        }
        
        // 显示结果
        if (config.show_stats || config.verbose) {
            auto cache_stats = incremental_generator.GetCacheStats();
            ShowGenerationStatistics(result, cache_stats, config);
        } else if (result.success) {
            std::cout << "✅ 生成完成! 处理了 " << result.processed_files.size() 
                      << " 个文件，跳过 " << result.skipped_files.size() << " 个文件" << std::endl;
        }
        
        if (!result.success) {
            std::cerr << "❌ 生成失败" << std::endl;
            return 1;
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        if (config.verbose) {
            std::cout << "🎉 总耗时: " << total_time.count() << " 毫秒" << std::endl;
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ 程序异常: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ 未知异常" << std::endl;
        return 1;
    }
}