/**
 * @file smart_inference_engine.h
 * @brief 智能推导引擎 - 自动从 AST 推导导出信息
 * 
 * 这个引擎负责从 C++ AST 中智能推导各种信息，减少用户的配置负担：
 * 1. 自动推导类名、函数名、变量名
 * 2. 自动推导命名空间（C++ namespace -> Lua namespace）
 * 3. 自动识别属性（get/set 方法配对）
 * 4. 自动分析 STL 容器类型和模板参数
 * 5. 自动推导回调函数的参数类型
 */

#pragma once

#include "ast_visitor.h"
#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace lua_binding_generator {

/**
 * @brief 推导出的导出信息
 */
struct InferredExportInfo {
    // 基础信息
    std::string cpp_name;                   ///< C++ 中的原始名称
    std::string lua_name;                   ///< Lua 中的名称（可能是别名）
    std::string qualified_name;             ///< 完全限定名称
    std::string export_type;                ///< 导出类型：class, function, enum, etc.
    
    // 命名空间信息
    std::string cpp_namespace;              ///< C++ 命名空间
    std::string lua_namespace;              ///< Lua 命名空间
    std::string module_name;                ///< 模块名称
    
    // 类型信息
    std::string type_name;                  ///< 类型名称
    std::string return_type;                ///< 返回类型（函数/方法）
    std::vector<std::string> parameter_types; ///< 参数类型列表
    
    // 类相关信息
    std::string parent_class;               ///< 父类名称
    std::vector<std::string> base_classes;  ///< 基类列表
    bool is_static = false;                 ///< 是否为静态成员
    bool is_const = false;                  ///< 是否为 const 方法
    bool is_virtual = false;                ///< 是否为虚函数
    bool is_pure_virtual = false;           ///< 是否为纯虚函数
    bool is_abstract = false;               ///< 是否为抽象类
    
    // 特殊类型标识
    bool is_singleton = false;              ///< 是否为单例类
    bool is_static_class = false;           ///< 是否为静态类
    std::string singleton_method;           ///< 单例访问方法名
    
    // 属性相关信息
    std::string property_access;            ///< readonly, readwrite
    std::string getter_method;              ///< getter 方法名
    std::string setter_method;              ///< setter 方法名
    bool is_property = false;               ///< 是否为属性
    
    // 常量和变量信息
    bool is_constant = false;               ///< 是否为常量
    std::string constant_value;             ///< 常量值（如果可以推导）
    std::string variable_access;            ///< readonly, readwrite
    
    // STL 相关信息
    bool is_stl_container = false;          ///< 是否为 STL 容器
    std::string container_type;             ///< 容器类型：vector, map, set, etc.
    std::vector<std::string> template_args; ///< 模板参数
    
    // 回调相关信息
    bool is_callback = false;               ///< 是否为回调函数
    std::string callback_signature;         ///< 回调函数签名
    
    // 运算符重载信息
    bool is_operator = false;               ///< 是否为运算符重载
    std::string operator_symbol;            ///< 运算符符号
    std::string lua_metamethod;             ///< 对应的 Lua 元方法
    
    // 模板相关信息
    bool is_template = false;               ///< 是否为模板
    bool is_template_instance = false;      ///< 是否为模板实例
    std::string template_base_name;         ///< 模板基础名称
    std::vector<std::string> template_parameters; ///< 模板参数
    
    // 枚举相关信息
    bool is_enum = false;                   ///< 是否为枚举
    bool is_scoped_enum = false;            ///< 是否为强类型枚举 (enum class)
    std::vector<std::pair<std::string, int64_t>> enum_values; ///< 枚举值列表
    
    // 源位置信息
    std::string source_file;                ///< 源文件路径
    int line_number = 0;                    ///< 行号
    
    // 用户指定的参数（覆盖自动推导）
    std::unordered_map<std::string, std::string> user_params; ///< 用户参数
};

/**
 * @brief 智能推导引擎
 */
class SmartInferenceEngine {
public:
    /**
     * @brief 推导选项
     */
    struct InferenceOptions {
        bool auto_infer_namespaces = true;         ///< 自动推导命名空间
        bool auto_infer_properties = true;         ///< 自动推导属性
        bool auto_infer_stl_containers = true;     ///< 自动推导 STL 容器
        bool auto_infer_callbacks = true;          ///< 自动推导回调函数
        bool prefer_snake_case = false;            ///< 是否偏好下划线命名
        std::string default_namespace = "global";   ///< 默认命名空间
    };

public:
    /**
     * @brief 构造函数
     */
    explicit SmartInferenceEngine(clang::ASTContext* context);

    /**
     * @brief 设置推导选项
     */
    void SetOptions(const InferenceOptions& options);

    /**
     * @brief 获取推导选项
     */
    const InferenceOptions& GetOptions() const;

    /**
     * @brief 设置文件级模块名
     */
    void SetFileModule(const std::string& module_name);

    /**
     * @brief 从类声明推导导出信息
     */
    InferredExportInfo InferFromClass(const clang::CXXRecordDecl* class_decl,
                                     const std::string& annotation_params = "");

    /**
     * @brief 从函数声明推导导出信息
     */
    InferredExportInfo InferFromFunction(const clang::FunctionDecl* func_decl,
                                        const std::string& annotation_params = "");

    /**
     * @brief 从方法声明推导导出信息
     */
    InferredExportInfo InferFromMethod(const clang::CXXMethodDecl* method_decl,
                                      const std::string& annotation_params = "");

    /**
     * @brief 从枚举声明推导导出信息
     */
    InferredExportInfo InferFromEnum(const clang::EnumDecl* enum_decl,
                                    const std::string& annotation_params = "");

    /**
     * @brief 从变量声明推导导出信息
     */
    InferredExportInfo InferFromVariable(const clang::VarDecl* var_decl,
                                        const std::string& annotation_params = "");

    /**
     * @brief 从字段声明推导导出信息（用于回调函数等）
     */
    InferredExportInfo InferFromField(const clang::FieldDecl* field_decl,
                                     const std::string& annotation_params = "");

    /**
     * @brief 分析类的所有公共成员并推导导出信息
     */
    std::vector<InferredExportInfo> InferClassMembers(const clang::CXXRecordDecl* class_decl);

    /**
     * @brief 识别和推导属性（get/set 方法配对）
     */
    std::vector<InferredExportInfo> InferProperties(const std::vector<InferredExportInfo>& methods);

    /**
     * @brief 从单例类声明推导导出信息
     */
    InferredExportInfo InferFromSingleton(const clang::CXXRecordDecl* class_decl,
                                         const std::string& annotation_params = "");

    /**
     * @brief 从静态类声明推导导出信息
     */
    InferredExportInfo InferFromStaticClass(const clang::CXXRecordDecl* class_decl,
                                           const std::string& annotation_params = "");

    /**
     * @brief 从抽象类声明推导导出信息
     */
    InferredExportInfo InferFromAbstractClass(const clang::CXXRecordDecl* class_decl,
                                             const std::string& annotation_params = "");

    /**
     * @brief 从常量声明推导导出信息
     */
    InferredExportInfo InferFromConstant(const clang::VarDecl* var_decl,
                                        const std::string& annotation_params = "");

    /**
     * @brief 从运算符重载推导导出信息
     */
    InferredExportInfo InferFromOperator(const clang::CXXMethodDecl* method_decl,
                                        const std::string& annotation_params = "");

    /**
     * @brief 从模板类声明推导导出信息
     */
    InferredExportInfo InferFromTemplate(const clang::ClassTemplateDecl* template_decl,
                                        const std::string& annotation_params = "");

    /**
     * @brief 从模板实例推导导出信息
     */
    InferredExportInfo InferFromTemplateInstance(const std::string& instance_type,
                                                const std::string& annotation_params = "");

    /**
     * @brief 自动推导枚举值
     */
    std::vector<std::pair<std::string, int64_t>> InferEnumValues(const clang::EnumDecl* enum_decl);

    /**
     * @brief 检测单例模式
     */
    std::string DetectSingletonMethod(const clang::CXXRecordDecl* class_decl);

    /**
     * @brief 检测是否为静态类（只有静态成员）
     */
    bool IsStaticClass(const clang::CXXRecordDecl* class_decl);

    /**
     * @brief 检测是否为抽象类（包含纯虚函数）
     */
    bool IsAbstractClass(const clang::CXXRecordDecl* class_decl);

    /**
     * @brief 推导运算符对应的 Lua 元方法
     */
    std::string InferLuaMetamethod(const std::string& operator_symbol);

    /**
     * @brief 获取推导过程中的错误和警告
     */
    const std::vector<std::string>& GetErrors() const;
    const std::vector<std::string>& GetWarnings() const;

    /**
     * @brief 清除错误和警告
     */
    void ClearErrors();

private:
    clang::ASTContext* ast_context_;                    ///< AST 上下文
    InferenceOptions options_;                          ///< 推导选项
    std::string file_module_;                           ///< 文件级模块名
    std::vector<std::string> errors_;                   ///< 错误信息
    std::vector<std::string> warnings_;                 ///< 警告信息

    /**
     * @brief 参数解析器
     */
    class ParameterParser {
    public:
        static std::unordered_map<std::string, std::string> Parse(const std::string& params);
        static bool HasParameter(const std::unordered_map<std::string, std::string>& params,
                                const std::string& key);
        static std::string GetParameter(const std::unordered_map<std::string, std::string>& params,
                                       const std::string& key, const std::string& default_value = "");
    };

    /**
     * @brief 命名空间推导器
     */
    class NamespaceInferrer {
    public:
        explicit NamespaceInferrer(const InferenceOptions& options);
        
        std::string InferCppNamespace(const clang::Decl* decl);
        std::string InferLuaNamespace(const std::string& cpp_namespace,
                                     const std::string& file_module,
                                     const std::unordered_map<std::string, std::string>& user_params);
        
    private:
        const InferenceOptions& options_;
    };

    /**
     * @brief 类型分析器
     */
    class TypeAnalyzer {
    public:
        explicit TypeAnalyzer(clang::ASTContext* context);
        
        struct TypeInfo {
            std::string full_name;                      ///< 完整类型名
            std::string base_name;                      ///< 基础类型名
            bool is_stl_container = false;             ///< 是否为 STL 容器
            bool is_smart_pointer = false;             ///< 是否为智能指针
            bool is_callback = false;                  ///< 是否为回调函数
            std::string container_type;                ///< 容器类型
            std::vector<std::string> template_args;    ///< 模板参数
        };
        
        TypeInfo AnalyzeType(clang::QualType type);
        bool IsSTLContainer(const std::string& type_name);
        bool IsSmartPointer(const std::string& type_name);
        bool IsCallback(clang::QualType type);
        std::vector<std::string> ExtractTemplateArgs(clang::QualType type);
        
    private:
        clang::ASTContext* ast_context_;
    };

    /**
     * @brief 属性识别器
     */
    class PropertyRecognizer {
    public:
        struct PropertyMatch {
            std::string property_name;                  ///< 属性名
            std::string getter_name;                    ///< getter 方法名
            std::string setter_name;                    ///< setter 方法名（可选）
            bool is_readonly = false;                   ///< 是否只读
        };
        
        std::vector<PropertyMatch> RecognizeProperties(const std::vector<InferredExportInfo>& methods);
        
    private:
        bool IsGetter(const std::string& method_name, const std::string& return_type);
        bool IsSetter(const std::string& method_name, const std::vector<std::string>& param_types);
        std::string ExtractPropertyName(const std::string& getter_name);
        bool IsGetterSetterPair(const std::string& getter, const std::string& setter);
    };

    /**
     * @brief 名称转换器
     */
    class NameConverter {
    public:
        static std::string ToLuaName(const std::string& cpp_name, bool prefer_snake_case = false);
        static std::string ToSnakeCase(const std::string& camelCase);
        static std::string ToCamelCase(const std::string& snake_case);
        static std::string SanitizeLuaName(const std::string& name);
    };

    std::unique_ptr<NamespaceInferrer> namespace_inferrer_;     ///< 命名空间推导器
    std::unique_ptr<TypeAnalyzer> type_analyzer_;               ///< 类型分析器
    std::unique_ptr<PropertyRecognizer> property_recognizer_;   ///< 属性识别器

    // 内部辅助方法

    /**
     * @brief 从声明中提取基础信息
     */
    InferredExportInfo ExtractBasicInfo(const clang::NamedDecl* decl,
                                       const std::string& export_type,
                                       const std::string& annotation_params);

    /**
     * @brief 解析用户参数并应用覆盖
     */
    void ApplyUserParameters(InferredExportInfo& info,
                            const std::unordered_map<std::string, std::string>& user_params);

    /**
     * @brief 推导继承关系
     */
    std::vector<std::string> InferBaseClasses(const clang::CXXRecordDecl* class_decl);

    /**
     * @brief 推导函数参数类型
     */
    std::vector<std::string> InferParameterTypes(const clang::FunctionDecl* func_decl);

    /**
     * @brief 获取源文件信息
     */
    std::pair<std::string, int> GetSourceLocation(const clang::Decl* decl);

    /**
     * @brief 检查是否应该忽略某个声明
     */
    bool ShouldIgnore(const clang::Decl* decl);

    /**
     * @brief 记录错误
     */
    void RecordError(const std::string& error);

    /**
     * @brief 记录警告
     */
    void RecordWarning(const std::string& warning);

    /**
     * @brief 验证推导结果
     */
    bool ValidateInferredInfo(const InferredExportInfo& info);
};

} // namespace lua_binding_generator