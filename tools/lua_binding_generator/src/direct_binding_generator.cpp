/**
 * @file direct_binding_generator.cpp
 * @brief 直接绑定代码生成器实现
 */

#include "lua_binding_generator/direct_binding_generator.h"
#include <algorithm>
#include <regex>
#include <sstream>
#include <iomanip>
#include <set>

namespace lua_binding_generator {

// CodeBuilder 实现
DirectBindingGenerator::CodeBuilder::CodeBuilder(int indent_size)
    : indent_level_(0), indent_size_(indent_size) {}

DirectBindingGenerator::CodeBuilder& DirectBindingGenerator::CodeBuilder::AddLine(const std::string& line) {
    content_ << line << "\n";
    return *this;
}

DirectBindingGenerator::CodeBuilder& DirectBindingGenerator::CodeBuilder::AddEmptyLine() {
    content_ << "\n";
    return *this;
}

DirectBindingGenerator::CodeBuilder& DirectBindingGenerator::CodeBuilder::AddIndentedLine(const std::string& line) {
    content_ << GetIndent() << line << "\n";
    return *this;
}

DirectBindingGenerator::CodeBuilder& DirectBindingGenerator::CodeBuilder::IncreaseIndent() {
    indent_level_++;
    return *this;
}

DirectBindingGenerator::CodeBuilder& DirectBindingGenerator::CodeBuilder::DecreaseIndent() {
    if (indent_level_ > 0) {
        indent_level_--;
    }
    return *this;
}

DirectBindingGenerator::CodeBuilder& DirectBindingGenerator::CodeBuilder::AddComment(const std::string& comment) {
    content_ << GetIndent() << "// " << comment << "\n";
    return *this;
}

DirectBindingGenerator::CodeBuilder& DirectBindingGenerator::CodeBuilder::AddBlockComment(const std::vector<std::string>& lines) {
    content_ << GetIndent() << "/*\n";
    for (const auto& line : lines) {
        content_ << GetIndent() << " * " << line << "\n";
    }
    content_ << GetIndent() << " */\n";
    return *this;
}

std::string DirectBindingGenerator::CodeBuilder::Build() const {
    return content_.str();
}

void DirectBindingGenerator::CodeBuilder::Clear() {
    content_.str("");
    content_.clear();
    indent_level_ = 0;
}

std::string DirectBindingGenerator::CodeBuilder::GetIndent() const {
    return std::string(indent_level_ * indent_size_, ' ');
}

// NamespaceManager 实现
std::string DirectBindingGenerator::NamespaceManager::ResolveNamespace(const ExportInfo& info) {
    // 1. 检查是否有明确指定的命名空间
    if (!info.namespace_name.empty() && info.namespace_name != "global") {
        return info.namespace_name;
    }
    
    // 2. 使用默认全局命名空间
    return "global";
}

std::string DirectBindingGenerator::NamespaceManager::GetNamespaceVariable(const std::string& namespace_name) {
    if (namespace_name == "global") {
        return "lua";
    }
    
    // 检查是否已经创建了变量
    auto it = namespace_vars_.find(namespace_name);
    if (it != namespace_vars_.end()) {
        return it->second;
    }
    
    // 创建新的命名空间变量名
    std::string var_name = namespace_name + "_ns";
    std::replace(var_name.begin(), var_name.end(), '.', '_');
    std::replace(var_name.begin(), var_name.end(), ':', '_');
    
    namespace_vars_[namespace_name] = var_name;
    used_namespaces_.push_back(namespace_name);
    
    return var_name;
}

std::vector<std::string> DirectBindingGenerator::NamespaceManager::GetRequiredNamespaces() const {
    return used_namespaces_;
}

void DirectBindingGenerator::NamespaceManager::Clear() {
    namespace_vars_.clear();
    used_namespaces_.clear();
}

// DirectBindingGenerator 实现
DirectBindingGenerator::DirectBindingGenerator() = default;

void DirectBindingGenerator::SetOptions(const GenerationOptions& options) {
    options_ = options;
}

const DirectBindingGenerator::GenerationOptions& DirectBindingGenerator::GetOptions() const {
    return options_;
}

DirectBindingGenerator::GenerationResult DirectBindingGenerator::GenerateModuleBinding(
    const std::string& module_name,
    const std::vector<ExportInfo>& export_items) {
    
    GenerationResult result;
    namespace_manager_.Clear();
    
    try {
        CodeBuilder builder(options_.indent_size);
        
        // 1. 生成文件头部
        builder.AddLine(GenerateFileHeader(module_name));
        builder.AddEmptyLine();
        
        // 2. 生成包含头文件
        if (options_.generate_includes) {
            builder.AddLine(GenerateIncludes(export_items));
            builder.AddEmptyLine();
        }
        
        // 3. 按类型分组导出项
        auto grouped_exports = GroupExportsByType(export_items);
        
        // 4. 生成绑定代码
        std::string bindings_code;
        CodeBuilder bindings_builder(options_.indent_size);
        
        // 预先收集所有需要的命名空间
        for (const auto& item : export_items) {
            std::string ns = namespace_manager_.ResolveNamespace(item);
            namespace_manager_.GetNamespaceVariable(ns);
        }
        
        // 生成命名空间声明
        if (options_.use_namespace_tables) {
            std::string ns_declarations = GenerateNamespaceDeclarations();
            if (!ns_declarations.empty()) {
                bindings_builder.AddLine(ns_declarations);
                bindings_builder.AddEmptyLine();
            }
        }
        
        // 生成类绑定
        if (grouped_exports.count("class") > 0) {
            bindings_builder.AddComment("Class bindings");
            for (const auto& class_info : grouped_exports["class"]) {
                // 找到这个类的所有成员
                std::vector<ExportInfo> members;
                for (const auto& item : export_items) {
                    if (item.parent_class == class_info.name) {
                        members.push_back(item);
                    }
                }
                bindings_builder.AddLine(GenerateClassBinding(class_info, members));
                bindings_builder.AddEmptyLine();
                result.total_bindings++;
            }
        }
        
        // 生成函数绑定
        if (grouped_exports.count("function") > 0) {
            bindings_builder.AddComment("Function bindings");
            
            // 按命名空间分组函数绑定
            std::unordered_map<std::string, std::vector<ExportInfo>> func_by_namespace;
            for (const auto& func_info : grouped_exports["function"]) {
                std::string ns = namespace_manager_.ResolveNamespace(func_info);
                func_by_namespace[ns].push_back(func_info);
            }
            
            // 为每个命名空间生成函数绑定
            for (const auto& [ns, funcs] : func_by_namespace) {
                if (funcs.size() > 1) {
                    std::string ns_comment = (ns == "global") ? "Global functions" : 
                                           "Functions in namespace " + ns;
                    bindings_builder.AddComment(ns_comment);
                }
                
                for (const auto& func_info : funcs) {
                    bindings_builder.AddLine(GenerateFunctionBinding(func_info));
                    result.total_bindings++;
                }
                
                if (funcs.size() > 1) {
                    bindings_builder.AddEmptyLine();
                }
            }
        }
        
        // 生成枚举绑定
        if (grouped_exports.count("enum") > 0) {
            bindings_builder.AddComment("Enum bindings");
            
            // 按命名空间分组枚举绑定
            std::unordered_map<std::string, std::vector<ExportInfo>> enum_by_namespace;
            for (const auto& enum_info : grouped_exports["enum"]) {
                std::string ns = namespace_manager_.ResolveNamespace(enum_info);
                enum_by_namespace[ns].push_back(enum_info);
            }
            
            // 为每个命名空间生成枚举绑定
            for (const auto& [ns, enums] : enum_by_namespace) {
                if (enums.size() > 1) {
                    std::string ns_comment = (ns == "global") ? "Global enums" : 
                                           "Enums in namespace " + ns;
                    bindings_builder.AddComment(ns_comment);
                }
                
                for (const auto& enum_info : enums) {
                    // TODO: 从 AST 提取枚举值
                    std::vector<std::string> enum_values; 
                    bindings_builder.AddLine(GenerateEnumBinding(enum_info, enum_values));
                    result.total_bindings++;
                }
                
                if (enums.size() > 1) {
                    bindings_builder.AddEmptyLine();
                }
            }
        }
        
        // 生成 STL 绑定
        if (grouped_exports.count("stl") > 0) {
            bindings_builder.AddComment("STL container bindings");
            for (const auto& stl_info : grouped_exports["stl"]) {
                bindings_builder.AddLine(GenerateSTLBinding(stl_info));
                result.total_bindings++;
            }
        }
        
        bindings_code = bindings_builder.Build();
        
        // 5. 生成注册函数
        if (options_.generate_registration_function) {
            builder.AddLine(GenerateRegistrationFunction(module_name, bindings_code));
        } else {
            builder.AddLine(bindings_code);
        }
        
        result.generated_code = builder.Build();
        result.success = true;
        
    } catch (const std::exception& e) {
        result.success = false;
        result.errors.push_back(std::string("Generation failed: ") + e.what());
    }
    
    return result;
}

std::string DirectBindingGenerator::GenerateClassBinding(const ExportInfo& class_info,
                                                       const std::vector<ExportInfo>& members) {
    CodeBuilder builder(options_.indent_size);
    
    std::string namespace_var = namespace_manager_.GetNamespaceVariable(
        namespace_manager_.ResolveNamespace(class_info));
    
    std::string lua_class_name = class_info.lua_name.empty() ? class_info.name : class_info.lua_name;
    std::string qualified_name = GetQualifiedTypeName(class_info);
    
    // 开始类绑定 - 将类名放在同一行
    builder.AddIndentedLine(namespace_var + ".new_usertype<" + qualified_name + ">(\"" + lua_class_name + "\",");
    builder.IncreaseIndent();
    
    // 分类成员，并去除重复
    std::vector<ExportInfo> constructors, methods, static_methods, properties, operators;
    std::set<std::string> seen_signatures;
    
    for (const auto& member : members) {
        // 创建唯一签名来检测重复
        std::string signature = member.export_type + "::" + member.name + "::" + 
                               member.qualified_name + "::" + member.parent_class;
        
        if (seen_signatures.find(signature) != seen_signatures.end()) {
            continue; // 跳过重复项
        }
        seen_signatures.insert(signature);
        
        if (member.export_type == "constructor") {
            constructors.push_back(member);
        } else if (member.export_type == "method") {
            methods.push_back(member);
        } else if (member.export_type == "static_method") {
            static_methods.push_back(member);
        } else if (member.export_type == "property") {
            properties.push_back(member);
        } else if (member.export_type == "operator") {
            operators.push_back(member);
        }
    }
    
    // 收集所有绑定项
    std::vector<std::string> all_bindings;
    
    // 生成继承关系 (必须在构造函数之前)
    if (!class_info.base_classes.empty()) {
        all_bindings.push_back(GenerateInheritanceCode(class_info));
    }
    
    // 生成构造函数绑定
    for (const auto& ctor : constructors) {
        std::string ctor_name = ctor.lua_name.empty() ? ctor.name : ctor.lua_name;
        std::string qualified_ctor_name = GetQualifiedTypeName(ctor);
        all_bindings.push_back("\"" + ctor_name + "\", &" + qualified_ctor_name);
    }
    
    // 生成方法绑定
    for (const auto& method : methods) {
        std::string method_name = method.lua_name.empty() ? method.name : method.lua_name;
        std::string qualified_method_name = GetQualifiedTypeName(method);
        all_bindings.push_back("\"" + method_name + "\", &" + qualified_method_name);
    }
    
    // 生成静态方法绑定
    for (const auto& static_method : static_methods) {
        std::string static_name = static_method.lua_name.empty() ? static_method.name : static_method.lua_name;
        std::string qualified_static_name = GetQualifiedTypeName(static_method);
        all_bindings.push_back("\"" + static_name + "\", &" + qualified_static_name);
    }
    
    // 生成属性绑定
    for (const auto& prop : properties) {
        std::string prop_name = prop.lua_name.empty() ? prop.name : prop.lua_name;
        std::string qualified_prop_name = GetQualifiedTypeName(prop);
        
        if (prop.property_access == "readonly") {
            all_bindings.push_back("\"" + prop_name + "\", sol::readonly_property(&" + qualified_prop_name + ")");
        } else {
            all_bindings.push_back("\"" + prop_name + "\", sol::property(&" + qualified_prop_name + ")");
        }
    }
    
    // 生成操作符绑定
    for (const auto& op : operators) {
        std::string op_name = op.lua_name.empty() ? op.name : op.lua_name;
        std::string qualified_op_name = GetQualifiedTypeName(op);
        all_bindings.push_back("\"" + op_name + "\", &" + qualified_op_name);
    }
    
    // 输出所有绑定项，除了最后一项外都加逗号
    for (size_t i = 0; i < all_bindings.size(); ++i) {
        if (i == all_bindings.size() - 1) {
            // 最后一项不加逗号
            builder.AddIndentedLine(all_bindings[i]);
        } else {
            // 其他项加逗号
            builder.AddIndentedLine(all_bindings[i] + ",");
        }
    }
    
    builder.DecreaseIndent();
    builder.AddIndentedLine(");");
    
    return builder.Build();
}

std::string DirectBindingGenerator::GenerateFunctionBinding(const ExportInfo& function_info) {
    std::string namespace_var = namespace_manager_.GetNamespaceVariable(
        namespace_manager_.ResolveNamespace(function_info));
    
    std::string lua_name = function_info.lua_name.empty() ? function_info.name : function_info.lua_name;
    std::string qualified_name = GetQualifiedTypeName(function_info);
    
    if (namespace_var == "lua") {
        return "lua[\"" + lua_name + "\"] = &" + qualified_name + ";";
    } else {
        return namespace_var + "[\"" + lua_name + "\"] = &" + qualified_name + ";";
    }
}

std::string DirectBindingGenerator::GenerateEnumBinding(const ExportInfo& enum_info,
                                                       const std::vector<std::string>& enum_values) {
    CodeBuilder builder(options_.indent_size);
    
    std::string namespace_var = namespace_manager_.GetNamespaceVariable(
        namespace_manager_.ResolveNamespace(enum_info));
    
    std::string lua_name = enum_info.lua_name.empty() ? enum_info.name : enum_info.lua_name;
    std::string qualified_name = GetQualifiedTypeName(enum_info);
    
    if (enum_values.empty()) {
        // 如果没有枚举值，生成简化版本
        builder.AddIndentedLine(namespace_var + ".new_enum(\"" + lua_name + "\");");
    } else {
        // 多行格式，每个枚举值一行
        builder.AddIndentedLine(namespace_var + ".new_enum(\"" + lua_name + "\"");
        builder.IncreaseIndent();
        
        for (size_t i = 0; i < enum_values.size(); ++i) {
            const auto& value = enum_values[i];
            std::string prefix = (i == 0 ? ", " : ", ");
            builder.AddIndentedLine(prefix + "\"" + value + "\", " + 
                                   qualified_name + "::" + value);
        }
        
        builder.DecreaseIndent();
        builder.AddIndentedLine(");");
    }
    
    return builder.Build();
}

std::string DirectBindingGenerator::GenerateSTLBinding(const ExportInfo& stl_info) {
    auto type_info = AnalyzeSTLType(stl_info.type_name);
    
    switch (type_info.container_type) {
        case STLTypeInfo::VECTOR:
            return GenerateVectorBinding(type_info);
        case STLTypeInfo::MAP:
            return GenerateMapBinding(type_info);
        case STLTypeInfo::SET:
            return GenerateSetBinding(type_info);
        default:
            return "// Unsupported STL container: " + stl_info.type_name;
    }
}

std::string DirectBindingGenerator::GenerateCallbackBinding(const ExportInfo& callback_info) {
    // 回调函数通常作为类成员变量处理
    std::string lua_name = callback_info.lua_name.empty() ? callback_info.name : callback_info.lua_name;
    return "\"" + lua_name + "\", &" + GetQualifiedTypeName(callback_info);
}

// 私有辅助方法实现
std::string DirectBindingGenerator::GenerateFileHeader(const std::string& module_name) {
    CodeBuilder builder;
    
    builder.AddBlockComment({
        "@file " + module_name + "_bindings.cpp",
        "@brief Auto-generated Lua bindings for " + module_name + " module",
        "",
        "This file is automatically generated by lua_binding_generator.",
        "Do not modify this file manually."
    });
    
    return builder.Build();
}

std::string DirectBindingGenerator::GenerateIncludes(const std::vector<ExportInfo>& export_items) {
    CodeBuilder builder;
    
    // 标准包含
    builder.AddLine("#include <sol/sol.hpp>");
    
    // 收集需要的头文件
    std::set<std::string> required_includes;
    for (const auto& item : export_items) {
        if (!item.source_file.empty()) {
            required_includes.insert(item.source_file);
        }
    }
    
    // 添加用户头文件
    for (const auto& include : required_includes) {
        builder.AddLine("#include \"" + include + "\"");
    }
    
    return builder.Build();
}

std::string DirectBindingGenerator::GenerateNamespaceDeclarations() {
    CodeBuilder builder;
    
    auto namespaces = namespace_manager_.GetRequiredNamespaces();
    for (const auto& ns : namespaces) {
        if (ns != "global") {
            std::string var_name = namespace_manager_.GetNamespaceVariable(ns);
            
            // 处理命名空间，支持 :: 和 . 分隔符
            std::string lua_path = "lua";
            std::string processed_ns = ns;
            
            // 替换 :: 为 .
            size_t pos = 0;
            while ((pos = processed_ns.find("::", pos)) != std::string::npos) {
                processed_ns.replace(pos, 2, ".");
                pos += 1;
            }
            
            std::istringstream iss(processed_ns);
            std::string part;
            while (std::getline(iss, part, '.')) {
                if (!part.empty()) {
                    lua_path += "[\"" + part + "\"]";
                }
            }
            
            builder.AddIndentedLine("auto " + var_name + " = " + lua_path + 
                                   ".get_or_create<sol::table>();");
        }
    }
    
    return builder.Build();
}

std::string DirectBindingGenerator::GenerateRegistrationFunction(const std::string& module_name,
                                                                const std::string& bindings_code) {
    CodeBuilder builder;
    
    builder.AddLine("void register_" + module_name + "_bindings(sol::state& lua) {");
    builder.IncreaseIndent();
    builder.AddLine(bindings_code);
    builder.DecreaseIndent();
    builder.AddLine("}");
    
    return builder.Build();
}

std::string DirectBindingGenerator::GenerateConstructorBindings(const std::vector<ExportInfo>& constructors) {
    if (constructors.empty()) {
        return "sol::constructors<>()";
    }
    
    std::string result = "sol::constructors<";
    for (size_t i = 0; i < constructors.size(); ++i) {
        if (i > 0) result += ", ";
        
        // 构建构造函数签名
        const auto& ctor = constructors[i];
        std::string signature = ctor.qualified_name + "(";
        
        // TODO: 从 AST 获取参数类型
        // 这里需要解析 ctor.parameters
        signature += ")";
        
        result += signature;
    }
    result += ">()";
    
    return result;
}

std::vector<std::string> DirectBindingGenerator::SplitConstructorBindings(const std::vector<ExportInfo>& constructors) {
    std::vector<std::string> lines;
    
    if (constructors.empty()) {
        lines.push_back("sol::constructors<>()");
        return lines;
    }
    
    // 为每个构造函数创建一个绑定
    std::set<std::string> seen_ctors;
    for (const auto& ctor : constructors) {
        std::string ctor_name = ctor.lua_name.empty() ? ctor.name : ctor.lua_name;
        std::string qualified_name = GetQualifiedTypeName(ctor);
        
        // 避免重复的构造函数绑定
        std::string signature = ctor_name + "::" + qualified_name;
        if (seen_ctors.find(signature) != seen_ctors.end()) {
            continue;
        }
        seen_ctors.insert(signature);
        
        lines.push_back("\"" + ctor_name + "\", &" + qualified_name);
    }
    
    return lines;
}

std::string DirectBindingGenerator::GenerateMethodBindings(const std::vector<ExportInfo>& methods) {
    std::string result;
    
    for (size_t i = 0; i < methods.size(); ++i) {
        const auto& method = methods[i];
        std::string lua_name = method.lua_name.empty() ? method.name : method.lua_name;
        
        if (i > 0) result += ", ";
        result += "\"" + lua_name + "\", &" + GetQualifiedTypeName(method);
    }
    
    return result;
}

std::vector<std::string> DirectBindingGenerator::SplitMethodBindings(const std::vector<ExportInfo>& methods) {
    std::vector<std::string> lines;
    std::set<std::string> seen_methods;
    
    for (const auto& method : methods) {
        std::string lua_name = method.lua_name.empty() ? method.name : method.lua_name;
        std::string qualified_name = GetQualifiedTypeName(method);
        
        // 避免重复的方法绑定
        std::string signature = lua_name + "::" + qualified_name;
        if (seen_methods.find(signature) != seen_methods.end()) {
            continue;
        }
        seen_methods.insert(signature);
        
        lines.push_back("\"" + lua_name + "\", &" + qualified_name);
    }
    
    return lines;
}

std::string DirectBindingGenerator::GeneratePropertyBindings(const std::vector<ExportInfo>& properties) {
    std::string result;
    
    for (size_t i = 0; i < properties.size(); ++i) {
        const auto& prop = properties[i];
        std::string lua_name = prop.lua_name.empty() ? prop.name : prop.lua_name;
        
        if (i > 0) result += ", ";
        
        if (prop.property_access == "readonly") {
            result += "\"" + lua_name + "\", sol::readonly_property(&" + GetQualifiedTypeName(prop) + ")";
        } else {
            // TODO: 处理读写属性，需要 getter 和 setter
            result += "\"" + lua_name + "\", sol::property(&" + GetQualifiedTypeName(prop) + ")";
        }
    }
    
    return result;
}

std::vector<std::string> DirectBindingGenerator::SplitPropertyBindings(const std::vector<ExportInfo>& properties) {
    std::vector<std::string> lines;
    std::set<std::string> seen_properties;
    
    for (const auto& prop : properties) {
        std::string lua_name = prop.lua_name.empty() ? prop.name : prop.lua_name;
        std::string qualified_name = GetQualifiedTypeName(prop);
        
        // 避免重复的属性绑定
        std::string signature = lua_name + "::" + qualified_name;
        if (seen_properties.find(signature) != seen_properties.end()) {
            continue;
        }
        seen_properties.insert(signature);
        
        std::string binding;
        if (prop.property_access == "readonly") {
            binding = "\"" + lua_name + "\", sol::readonly_property(&" + qualified_name + ")";
        } else {
            // TODO: 处理读写属性，需要 getter 和 setter
            binding = "\"" + lua_name + "\", sol::property(&" + qualified_name + ")";
        }
        
        lines.push_back(binding);
    }
    
    return lines;
}

std::string DirectBindingGenerator::GenerateStaticMethodBindings(const std::vector<ExportInfo>& static_methods) {
    std::string result;
    
    for (size_t i = 0; i < static_methods.size(); ++i) {
        const auto& method = static_methods[i];
        std::string lua_name = method.lua_name.empty() ? method.name : method.lua_name;
        
        if (i > 0) result += ", ";
        result += "\"" + lua_name + "\", &" + GetQualifiedTypeName(method);
    }
    
    return result;
}

std::vector<std::string> DirectBindingGenerator::SplitStaticMethodBindings(const std::vector<ExportInfo>& static_methods) {
    std::vector<std::string> lines;
    std::set<std::string> seen_static_methods;
    
    for (const auto& method : static_methods) {
        std::string lua_name = method.lua_name.empty() ? method.name : method.lua_name;
        std::string qualified_name = GetQualifiedTypeName(method);
        
        // 避免重复的静态方法绑定
        std::string signature = lua_name + "::" + qualified_name;
        if (seen_static_methods.find(signature) != seen_static_methods.end()) {
            continue;
        }
        seen_static_methods.insert(signature);
        
        lines.push_back("\"" + lua_name + "\", &" + qualified_name);
    }
    
    return lines;
}

std::string DirectBindingGenerator::GenerateOperatorBindings(const std::vector<ExportInfo>& operators) {
    // TODO: 实现操作符绑定
    return "";
}

std::vector<std::string> DirectBindingGenerator::SplitOperatorBindings(const std::vector<ExportInfo>& operators) {
    std::vector<std::string> lines;
    std::set<std::string> seen_operators;
    
    for (const auto& op : operators) {
        std::string lua_name = op.lua_name.empty() ? op.name : op.lua_name;
        std::string qualified_name = GetQualifiedTypeName(op);
        
        // 避免重复的操作符绑定
        std::string signature = lua_name + "::" + qualified_name;
        if (seen_operators.find(signature) != seen_operators.end()) {
            continue;
        }
        seen_operators.insert(signature);
        
        lines.push_back("\"" + lua_name + "\", &" + qualified_name);
    }
    
    return lines;
}

std::string DirectBindingGenerator::GenerateInheritanceCode(const ExportInfo& class_info) {
    if (class_info.base_classes.empty()) {
        return "";
    }
    
    std::string result = "sol::base_classes, sol::bases<";
    for (size_t i = 0; i < class_info.base_classes.size(); ++i) {
        if (i > 0) result += ", ";
        result += class_info.base_classes[i];
    }
    result += ">()";
    
    return result;
}

DirectBindingGenerator::STLTypeInfo DirectBindingGenerator::AnalyzeSTLType(const std::string& type_name) {
    STLTypeInfo info;
    info.full_type_name = type_name;
    info.container_type = STLTypeInfo::UNKNOWN;
    
    // 简单的类型分析
    if (type_name.find("std::vector") == 0) {
        info.container_type = STLTypeInfo::VECTOR;
        info.lua_type_name = "vector"; // TODO: 添加模板参数
    } else if (type_name.find("std::map") == 0) {
        info.container_type = STLTypeInfo::MAP;
        info.lua_type_name = "map";
    } else if (type_name.find("std::set") == 0) {
        info.container_type = STLTypeInfo::SET;
        info.lua_type_name = "set";
    }
    
    // TODO: 解析模板参数
    
    return info;
}

std::string DirectBindingGenerator::GenerateVectorBinding(const STLTypeInfo& info) {
    CodeBuilder builder;
    
    builder.AddIndentedLine("lua.new_usertype<" + info.full_type_name + ">(\"" + info.lua_type_name + "\"");
    builder.IncreaseIndent();
    builder.AddIndentedLine(", sol::constructors<" + info.full_type_name + "()>()");
    builder.AddIndentedLine(", \"size\", &" + info.full_type_name + "::size");
    builder.AddIndentedLine(", \"empty\", &" + info.full_type_name + "::empty");
    builder.AddIndentedLine(", \"clear\", &" + info.full_type_name + "::clear");
    builder.AddIndentedLine(", \"push_back\", &" + info.full_type_name + "::push_back");
    builder.AddIndentedLine(", \"pop_back\", &" + info.full_type_name + "::pop_back");
    // TODO: 添加更多 vector 方法
    builder.DecreaseIndent();
    builder.AddIndentedLine(");");
    
    return builder.Build();
}

std::string DirectBindingGenerator::GenerateMapBinding(const STLTypeInfo& info) {
    // TODO: 实现 map 绑定
    return "// TODO: Map binding for " + info.full_type_name;
}

std::string DirectBindingGenerator::GenerateSetBinding(const STLTypeInfo& info) {
    // TODO: 实现 set 绑定
    return "// TODO: Set binding for " + info.full_type_name;
}

std::string DirectBindingGenerator::GetLuaTypeName(const std::string& cpp_type) {
    // 简单的类型名转换
    std::string result = cpp_type;
    std::replace(result.begin(), result.end(), ':', '_');
    std::replace(result.begin(), result.end(), '<', '_');
    std::replace(result.begin(), result.end(), '>', '_');
    std::replace(result.begin(), result.end(), ' ', '_');
    return result;
}

std::string DirectBindingGenerator::GetQualifiedTypeName(const ExportInfo& info) {
    if (!info.qualified_name.empty()) {
        return info.qualified_name;
    }
    
    // 构建合格名称
    std::string result;
    if (!info.namespace_name.empty() && info.namespace_name != "global") {
        result = info.namespace_name + "::";
    }
    
    if (!info.parent_class.empty()) {
        result += info.parent_class + "::";
    }
    
    result += info.name;
    return result;
}

bool DirectBindingGenerator::IsSmartPointer(const std::string& type_name) {
    return type_name.find("std::shared_ptr") == 0 ||
           type_name.find("std::unique_ptr") == 0 ||
           type_name.find("std::weak_ptr") == 0;
}

std::string DirectBindingGenerator::GenerateSmartPointerBinding(const ExportInfo& info) {
    // TODO: 实现智能指针绑定
    return "// TODO: Smart pointer binding for " + info.type_name;
}

std::unordered_map<std::string, std::vector<ExportInfo>> 
DirectBindingGenerator::GroupExportsByType(const std::vector<ExportInfo>& export_items) {
    std::unordered_map<std::string, std::vector<ExportInfo>> grouped;
    
    for (const auto& item : export_items) {
        grouped[item.export_type].push_back(item);
    }
    
    return grouped;
}

bool DirectBindingGenerator::ValidateExportInfo(const ExportInfo& info, std::vector<std::string>& errors) {
    bool valid = true;
    
    if (info.name.empty()) {
        errors.push_back("Export item has empty name");
        valid = false;
    }
    
    if (info.export_type.empty()) {
        errors.push_back("Export item '" + info.name + "' has empty export type");
        valid = false;
    }
    
    return valid;
}

std::string DirectBindingGenerator::GenerateErrorHandling() {
    // TODO: 实现错误处理代码生成
    return "";
}

} // namespace lua_binding_generator