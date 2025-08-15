/**
 * @file direct_binding_generator.h
 * @brief 直接绑定代码生成器 - 替代模板系统的硬编码生成器
 * 
 * 这个生成器直接在 C++ 代码中构建 Sol2 绑定代码，避免了模板解析的复杂性和性能开销。
 * 专注于提供一致的代码风格和最佳的运行时性能。
 */

#pragma once

#include "ast_visitor.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <sstream>

namespace lua_binding_generator {

/**
 * @brief 直接绑定代码生成器
 * 
 * 使用硬编码方式生成 Sol2 绑定代码，提供一致的风格和高性能。
 * 支持所有主要的 C++ 导出类型，包括类、函数、枚举、STL 容器等。
 */
class DirectBindingGenerator {
public:
    /**
     * @brief 生成选项
     */
    struct GenerationOptions {
        std::string output_directory = "generated_bindings";    ///< 输出目录
        std::string default_namespace = "global";               ///< 默认命名空间
        bool generate_includes = true;                          ///< 是否生成包含头文件
        bool generate_registration_function = true;             ///< 是否生成注册函数
        bool use_namespace_tables = true;                       ///< 是否使用命名空间表
        int indent_size = 4;                                    ///< 缩进大小
    };

    /**
     * @brief 生成结果
     */
    struct GenerationResult {
        bool success = false;                                   ///< 是否成功
        std::string generated_code;                             ///< 生成的代码
        std::vector<std::string> includes;                     ///< 需要包含的头文件
        std::vector<std::string> errors;                       ///< 错误信息
        std::vector<std::string> warnings;                     ///< 警告信息
        size_t total_bindings = 0;                             ///< 绑定总数
    };

public:
    /**
     * @brief 构造函数
     */
    DirectBindingGenerator();

    /**
     * @brief 析构函数
     */
    ~DirectBindingGenerator() = default;

    /**
     * @brief 设置生成选项
     */
    void SetOptions(const GenerationOptions& options);

    /**
     * @brief 获取生成选项
     */
    const GenerationOptions& GetOptions() const;

    /**
     * @brief 生成完整的模块绑定代码
     * @param module_name 模块名称
     * @param export_items 导出项列表
     * @return 生成结果
     */
    GenerationResult GenerateModuleBinding(const std::string& module_name,
                                          const std::vector<ExportInfo>& export_items);

    /**
     * @brief 生成类绑定代码
     * @param class_info 类信息
     * @param members 类成员列表
     * @return 生成的绑定代码
     */
    std::string GenerateClassBinding(const ExportInfo& class_info,
                                   const std::vector<ExportInfo>& members);

    /**
     * @brief 生成函数绑定代码
     * @param function_info 函数信息
     * @return 生成的绑定代码
     */
    std::string GenerateFunctionBinding(const ExportInfo& function_info);

    /**
     * @brief 生成枚举绑定代码
     * @param enum_info 枚举信息
     * @param enum_values 枚举值列表
     * @return 生成的绑定代码
     */
    std::string GenerateEnumBinding(const ExportInfo& enum_info,
                                  const std::vector<std::string>& enum_values);

    /**
     * @brief 生成 STL 容器绑定代码
     * @param stl_info STL 容器信息
     * @return 生成的绑定代码
     */
    std::string GenerateSTLBinding(const ExportInfo& stl_info);

    /**
     * @brief 生成回调函数绑定代码
     * @param callback_info 回调信息
     * @return 生成的绑定代码
     */
    std::string GenerateCallbackBinding(const ExportInfo& callback_info);

private:
    GenerationOptions options_;                                 ///< 生成选项
    
    /**
     * @brief 代码构建器辅助类
     */
    class CodeBuilder {
    public:
        explicit CodeBuilder(int indent_size = 4);
        
        CodeBuilder& AddLine(const std::string& line);
        CodeBuilder& AddEmptyLine();
        CodeBuilder& AddIndentedLine(const std::string& line);
        CodeBuilder& IncreaseIndent();
        CodeBuilder& DecreaseIndent();
        CodeBuilder& AddComment(const std::string& comment);
        CodeBuilder& AddBlockComment(const std::vector<std::string>& lines);
        
        std::string Build() const;
        void Clear();

    private:
        std::stringstream content_;
        int indent_level_;
        int indent_size_;
        
        std::string GetIndent() const;
    };

    /**
     * @brief 命名空间管理器
     */
    class NamespaceManager {
    public:
        std::string ResolveNamespace(const ExportInfo& info);
        std::string GetNamespaceVariable(const std::string& namespace_name);
        std::vector<std::string> GetRequiredNamespaces() const;
        void Clear();

    private:
        std::unordered_map<std::string, std::string> namespace_vars_;
        std::vector<std::string> used_namespaces_;
    };

    NamespaceManager namespace_manager_;                        ///< 命名空间管理器

    // 内部生成方法
    
    /**
     * @brief 生成文件头部
     */
    std::string GenerateFileHeader(const std::string& module_name);

    /**
     * @brief 生成包含头文件
     */
    std::string GenerateIncludes(const std::vector<ExportInfo>& export_items);

    /**
     * @brief 生成命名空间声明
     */
    std::string GenerateNamespaceDeclarations();

    /**
     * @brief 生成注册函数
     */
    std::string GenerateRegistrationFunction(const std::string& module_name,
                                            const std::string& bindings_code);

    /**
     * @brief 生成构造函数绑定
     */
    std::string GenerateConstructorBindings(const std::vector<ExportInfo>& constructors);

    /**
     * @brief 生成方法绑定
     */
    std::string GenerateMethodBindings(const std::vector<ExportInfo>& methods);

    /**
     * @brief 生成属性绑定
     */
    std::string GeneratePropertyBindings(const std::vector<ExportInfo>& properties);

    /**
     * @brief 生成静态方法绑定
     */
    std::string GenerateStaticMethodBindings(const std::vector<ExportInfo>& static_methods);

    /**
     * @brief 生成操作符重载绑定
     */
    std::string GenerateOperatorBindings(const std::vector<ExportInfo>& operators);

    // 格式化的多行绑定生成方法
    /**
     * @brief 将构造函数绑定分割为多行
     */
    std::vector<std::string> SplitConstructorBindings(const std::vector<ExportInfo>& constructors);

    /**
     * @brief 将方法绑定分割为多行
     */
    std::vector<std::string> SplitMethodBindings(const std::vector<ExportInfo>& methods);

    /**
     * @brief 将属性绑定分割为多行
     */
    std::vector<std::string> SplitPropertyBindings(const std::vector<ExportInfo>& properties);

    /**
     * @brief 将静态方法绑定分割为多行
     */
    std::vector<std::string> SplitStaticMethodBindings(const std::vector<ExportInfo>& static_methods);

    /**
     * @brief 将操作符绑定分割为多行
     */
    std::vector<std::string> SplitOperatorBindings(const std::vector<ExportInfo>& operators);

    /**
     * @brief 生成继承关系代码
     */
    std::string GenerateInheritanceCode(const ExportInfo& class_info);

    /**
     * @brief 分析 STL 容器类型
     */
    struct STLTypeInfo {
        enum Type { VECTOR, MAP, SET, LIST, DEQUE, STACK, QUEUE, UNKNOWN };
        Type container_type;
        std::string full_type_name;
        std::string lua_type_name;
        std::vector<std::string> template_args;
    };

    STLTypeInfo AnalyzeSTLType(const std::string& type_name);

    /**
     * @brief 生成 STL vector 绑定
     */
    std::string GenerateVectorBinding(const STLTypeInfo& info);

    /**
     * @brief 生成 STL map 绑定
     */
    std::string GenerateMapBinding(const STLTypeInfo& info);

    /**
     * @brief 生成 STL set 绑定
     */
    std::string GenerateSetBinding(const STLTypeInfo& info);

    /**
     * @brief 获取 Lua 友好的类型名
     */
    std::string GetLuaTypeName(const std::string& cpp_type);

    /**
     * @brief 获取合格的类型名（包含命名空间）
     */
    std::string GetQualifiedTypeName(const ExportInfo& info);

    /**
     * @brief 检查是否为智能指针类型
     */
    bool IsSmartPointer(const std::string& type_name);

    /**
     * @brief 生成智能指针绑定
     */
    std::string GenerateSmartPointerBinding(const ExportInfo& info);

    /**
     * @brief 按类型分组导出项
     */
    std::unordered_map<std::string, std::vector<ExportInfo>> 
    GroupExportsByType(const std::vector<ExportInfo>& export_items);

    /**
     * @brief 验证导出信息
     */
    bool ValidateExportInfo(const ExportInfo& info, std::vector<std::string>& errors);

    /**
     * @brief 生成错误处理代码
     */
    std::string GenerateErrorHandling();
};

} // namespace lua_binding_generator