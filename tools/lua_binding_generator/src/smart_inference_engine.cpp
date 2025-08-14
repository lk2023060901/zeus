/**
 * @file smart_inference_engine.cpp
 * @brief 智能推导引擎实现
 */

#include "lua_binding_generator/smart_inference_engine.h"
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <algorithm>
#include <regex>
#include <sstream>

namespace lua_binding_generator {

// ParameterParser 实现
std::unordered_map<std::string, std::string> 
SmartInferenceEngine::ParameterParser::Parse(const std::string& params) {
    std::unordered_map<std::string, std::string> result;
    
    if (params.empty() || params == "auto") {
        return result;
    }
    
    // 解析格式："key1=value1,key2=value2"
    std::istringstream iss(params);
    std::string pair;
    
    while (std::getline(iss, pair, ',')) {
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = pair.substr(0, eq_pos);
            std::string value = pair.substr(eq_pos + 1);
            
            // 去除空格
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            result[key] = value;
        }
    }
    
    return result;
}

bool SmartInferenceEngine::ParameterParser::HasParameter(
    const std::unordered_map<std::string, std::string>& params,
    const std::string& key) {
    return params.find(key) != params.end();
}

std::string SmartInferenceEngine::ParameterParser::GetParameter(
    const std::unordered_map<std::string, std::string>& params,
    const std::string& key, const std::string& default_value) {
    auto it = params.find(key);
    return it != params.end() ? it->second : default_value;
}

// NamespaceInferrer 实现
SmartInferenceEngine::NamespaceInferrer::NamespaceInferrer(const InferenceOptions& options)
    : options_(options) {}

std::string SmartInferenceEngine::NamespaceInferrer::InferCppNamespace(const clang::Decl* decl) {
    if (!options_.auto_infer_namespaces) {
        return "";
    }
    
    std::vector<std::string> namespace_parts;
    const clang::DeclContext* context = decl->getDeclContext();
    
    while (context && !context->isTranslationUnit()) {
        if (auto ns_decl = clang::dyn_cast<clang::NamespaceDecl>(context)) {
            if (!ns_decl->isAnonymousNamespace()) {
                namespace_parts.push_back(ns_decl->getNameAsString());
            }
        }
        context = context->getParent();
    }
    
    // 反转顺序（从外到内）
    std::reverse(namespace_parts.begin(), namespace_parts.end());
    
    std::string result;
    for (size_t i = 0; i < namespace_parts.size(); ++i) {
        if (i > 0) result += "::";
        result += namespace_parts[i];
    }
    
    return result;
}

std::string SmartInferenceEngine::NamespaceInferrer::InferLuaNamespace(
    const std::string& cpp_namespace,
    const std::string& file_module,
    const std::unordered_map<std::string, std::string>& user_params) {
    
    // 1. 检查用户是否明确指定了命名空间
    if (ParameterParser::HasParameter(user_params, "namespace")) {
        std::string ns = ParameterParser::GetParameter(user_params, "namespace");
        return ns == "global" ? "" : ns;
    }
    
    // 2. 使用 C++ 命名空间（转换 :: 为 .）
    if (!cpp_namespace.empty()) {
        std::string lua_ns = cpp_namespace;
        std::replace(lua_ns.begin(), lua_ns.end(), ':', '.');
        // 移除连续的点
        lua_ns = std::regex_replace(lua_ns, std::regex("\\.+"), ".");
        return lua_ns;
    }
    
    // 3. 使用文件模块名
    if (!file_module.empty()) {
        return file_module;
    }
    
    // 4. 使用默认命名空间
    return options_.default_namespace == "global" ? "" : options_.default_namespace;
}

// TypeAnalyzer 实现
SmartInferenceEngine::TypeAnalyzer::TypeAnalyzer(clang::ASTContext* context)
    : ast_context_(context) {}

SmartInferenceEngine::TypeAnalyzer::TypeInfo 
SmartInferenceEngine::TypeAnalyzer::AnalyzeType(clang::QualType type) {
    TypeInfo info;
    
    // 获取完整类型名
    info.full_name = type.getAsString();
    
    // 获取基础类型名（去除模板参数）
    if (auto template_type = type->getAs<clang::TemplateSpecializationType>()) {
        info.base_name = template_type->getTemplateName().getAsTemplateDecl()->getNameAsString();
        info.template_args = ExtractTemplateArgs(type);
    } else {
        info.base_name = info.full_name;
    }
    
    // 检查是否为 STL 容器
    info.is_stl_container = IsSTLContainer(info.full_name);
    if (info.is_stl_container) {
        if (info.full_name.find("vector") != std::string::npos) {
            info.container_type = "vector";
        } else if (info.full_name.find("map") != std::string::npos) {
            info.container_type = "map";
        } else if (info.full_name.find("set") != std::string::npos) {
            info.container_type = "set";
        } else if (info.full_name.find("list") != std::string::npos) {
            info.container_type = "list";
        }
    }
    
    // 检查是否为智能指针
    info.is_smart_pointer = IsSmartPointer(info.full_name);
    
    // 检查是否为回调函数
    info.is_callback = IsCallback(type);
    
    return info;
}

bool SmartInferenceEngine::TypeAnalyzer::IsSTLContainer(const std::string& type_name) {
    static const std::vector<std::string> stl_containers = {
        "std::vector", "std::list", "std::deque", "std::array",
        "std::map", "std::multimap", "std::unordered_map", "std::unordered_multimap",
        "std::set", "std::multiset", "std::unordered_set", "std::unordered_multiset",
        "std::stack", "std::queue", "std::priority_queue"
    };
    
    for (const auto& container : stl_containers) {
        if (type_name.find(container) == 0) {
            return true;
        }
    }
    
    return false;
}

bool SmartInferenceEngine::TypeAnalyzer::IsSmartPointer(const std::string& type_name) {
    return type_name.find("std::shared_ptr") == 0 ||
           type_name.find("std::unique_ptr") == 0 ||
           type_name.find("std::weak_ptr") == 0;
}

bool SmartInferenceEngine::TypeAnalyzer::IsCallback(clang::QualType type) {
    // 检查是否为 std::function 类型
    if (auto template_type = type->getAs<clang::TemplateSpecializationType>()) {
        auto template_decl = template_type->getTemplateName().getAsTemplateDecl();
        if (template_decl) {
            std::string template_name = template_decl->getQualifiedNameAsString();
            return template_name == "std::function";
        }
    }
    
    return false;
}

std::vector<std::string> SmartInferenceEngine::TypeAnalyzer::ExtractTemplateArgs(clang::QualType type) {
    std::vector<std::string> args;
    
    if (auto template_type = type->getAs<clang::TemplateSpecializationType>()) {
        auto template_args = template_type->template_arguments();
        for (const auto& arg : template_args) {
            if (arg.getKind() == clang::TemplateArgument::Type) {
                args.push_back(arg.getAsType().getAsString());
            }
        }
    }
    
    return args;
}

// PropertyRecognizer 实现
std::vector<SmartInferenceEngine::PropertyRecognizer::PropertyMatch>
SmartInferenceEngine::PropertyRecognizer::RecognizeProperties(
    const std::vector<InferredExportInfo>& methods) {
    
    std::vector<PropertyMatch> properties;
    std::unordered_map<std::string, const InferredExportInfo*> getters;
    std::unordered_map<std::string, const InferredExportInfo*> setters;
    
    // 分离 getter 和 setter
    for (const auto& method : methods) {
        if (IsGetter(method.cpp_name, method.return_type)) {
            std::string prop_name = ExtractPropertyName(method.cpp_name);
            getters[prop_name] = &method;
        } else if (IsSetter(method.cpp_name, method.parameter_types)) {
            std::string prop_name = ExtractPropertyName(method.cpp_name);
            setters[prop_name] = &method;
        }
    }
    
    // 匹配 getter 和 setter
    for (const auto& [prop_name, getter] : getters) {
        PropertyMatch match;
        match.property_name = prop_name;
        match.getter_name = getter->cpp_name;
        
        auto setter_it = setters.find(prop_name);
        if (setter_it != setters.end()) {
            match.setter_name = setter_it->second->cpp_name;
            match.is_readonly = false;
        } else {
            match.is_readonly = true;
        }
        
        properties.push_back(match);
    }
    
    return properties;
}

bool SmartInferenceEngine::PropertyRecognizer::IsGetter(
    const std::string& method_name, const std::string& return_type) {
    
    // 检查方法名模式
    bool has_get_prefix = method_name.length() > 3 && method_name.substr(0, 3) == "get";
    bool has_is_prefix = method_name.length() > 2 && method_name.substr(0, 2) == "is";
    
    // 检查返回类型不为 void
    bool has_return_value = return_type != "void";
    
    return (has_get_prefix || has_is_prefix) && has_return_value;
}

bool SmartInferenceEngine::PropertyRecognizer::IsSetter(
    const std::string& method_name, const std::vector<std::string>& param_types) {
    
    // 检查方法名模式
    bool has_set_prefix = method_name.length() > 3 && method_name.substr(0, 3) == "set";
    
    // 检查参数数量（应该有且仅有一个参数）
    bool has_single_param = param_types.size() == 1;
    
    return has_set_prefix && has_single_param;
}

std::string SmartInferenceEngine::PropertyRecognizer::ExtractPropertyName(const std::string& getter_name) {
    if (getter_name.length() > 3 && getter_name.substr(0, 3) == "get") {
        std::string prop_name = getter_name.substr(3);
        if (!prop_name.empty()) {
            prop_name[0] = std::tolower(prop_name[0]);
        }
        return prop_name;
    }
    
    if (getter_name.length() > 2 && getter_name.substr(0, 2) == "is") {
        std::string prop_name = getter_name.substr(2);
        if (!prop_name.empty()) {
            prop_name[0] = std::tolower(prop_name[0]);
        }
        return prop_name;
    }
    
    if (getter_name.length() > 3 && getter_name.substr(0, 3) == "set") {
        std::string prop_name = getter_name.substr(3);
        if (!prop_name.empty()) {
            prop_name[0] = std::tolower(prop_name[0]);
        }
        return prop_name;
    }
    
    return getter_name;
}

// NameConverter 实现
std::string SmartInferenceEngine::NameConverter::ToLuaName(
    const std::string& cpp_name, bool prefer_snake_case) {
    
    if (prefer_snake_case) {
        return SanitizeLuaName(ToSnakeCase(cpp_name));
    } else {
        return SanitizeLuaName(cpp_name);
    }
}

std::string SmartInferenceEngine::NameConverter::ToSnakeCase(const std::string& camelCase) {
    std::string result;
    
    for (size_t i = 0; i < camelCase.length(); ++i) {
        char c = camelCase[i];
        
        if (std::isupper(c) && i > 0) {
            result += '_';
        }
        result += std::tolower(c);
    }
    
    return result;
}

std::string SmartInferenceEngine::NameConverter::ToCamelCase(const std::string& snake_case) {
    std::string result;
    bool capitalize_next = false;
    
    for (char c : snake_case) {
        if (c == '_') {
            capitalize_next = true;
        } else {
            if (capitalize_next) {
                result += std::toupper(c);
                capitalize_next = false;
            } else {
                result += c;
            }
        }
    }
    
    return result;
}

std::string SmartInferenceEngine::NameConverter::SanitizeLuaName(const std::string& name) {
    // 移除 Lua 关键字冲突和特殊字符
    static const std::unordered_set<std::string> lua_keywords = {
        "and", "break", "do", "else", "elseif", "end", "false", "for",
        "function", "if", "in", "local", "nil", "not", "or", "repeat",
        "return", "then", "true", "until", "while"
    };
    
    std::string result = name;
    
    if (lua_keywords.count(result) > 0) {
        result += "_";
    }
    
    return result;
}

// SmartInferenceEngine 主要实现
SmartInferenceEngine::SmartInferenceEngine(clang::ASTContext* context)
    : ast_context_(context) {
    
    namespace_inferrer_ = std::make_unique<NamespaceInferrer>(options_);
    type_analyzer_ = std::make_unique<TypeAnalyzer>(context);
    property_recognizer_ = std::make_unique<PropertyRecognizer>();
}

void SmartInferenceEngine::SetOptions(const InferenceOptions& options) {
    options_ = options;
    namespace_inferrer_ = std::make_unique<NamespaceInferrer>(options_);
}

const SmartInferenceEngine::InferenceOptions& SmartInferenceEngine::GetOptions() const {
    return options_;
}

void SmartInferenceEngine::SetFileModule(const std::string& module_name) {
    file_module_ = module_name;
}

InferredExportInfo SmartInferenceEngine::InferFromClass(
    const clang::CXXRecordDecl* class_decl, const std::string& annotation_params) {
    
    auto info = ExtractBasicInfo(class_decl, "class", annotation_params);
    
    // 推导基类
    info.base_classes = InferBaseClasses(class_decl);
    
    // 分析类型
    auto type_info = type_analyzer_->AnalyzeType(class_decl->getTypeForDecl()->getCanonicalTypeInternal());
    info.is_stl_container = type_info.is_stl_container;
    info.container_type = type_info.container_type;
    info.template_args = type_info.template_args;
    
    return info;
}

InferredExportInfo SmartInferenceEngine::InferFromFunction(
    const clang::FunctionDecl* func_decl, const std::string& annotation_params) {
    
    auto info = ExtractBasicInfo(func_decl, "function", annotation_params);
    
    // 推导返回类型和参数
    info.return_type = func_decl->getReturnType().getAsString();
    info.parameter_types = InferParameterTypes(func_decl);
    
    return info;
}

InferredExportInfo SmartInferenceEngine::InferFromMethod(
    const clang::CXXMethodDecl* method_decl, const std::string& annotation_params) {
    
    std::string export_type = method_decl->isStatic() ? "static_method" : "method";
    auto info = ExtractBasicInfo(method_decl, export_type, annotation_params);
    
    // 方法特定信息
    info.is_static = method_decl->isStatic();
    info.is_const = method_decl->isConst();
    info.return_type = method_decl->getReturnType().getAsString();
    info.parameter_types = InferParameterTypes(method_decl);
    
    // 推导父类
    if (auto parent = clang::dyn_cast<clang::CXXRecordDecl>(method_decl->getParent())) {
        info.parent_class = parent->getNameAsString();
    }
    
    return info;
}

InferredExportInfo SmartInferenceEngine::InferFromEnum(
    const clang::EnumDecl* enum_decl, const std::string& annotation_params) {
    
    return ExtractBasicInfo(enum_decl, "enum", annotation_params);
}

InferredExportInfo SmartInferenceEngine::InferFromVariable(
    const clang::VarDecl* var_decl, const std::string& annotation_params) {
    
    auto info = ExtractBasicInfo(var_decl, "variable", annotation_params);
    
    // 变量特定信息
    info.type_name = var_decl->getType().getAsString();
    info.is_static = var_decl->isStaticDataMember() || var_decl->hasGlobalStorage();
    
    return info;
}

InferredExportInfo SmartInferenceEngine::InferFromField(
    const clang::FieldDecl* field_decl, const std::string& annotation_params) {
    
    auto info = ExtractBasicInfo(field_decl, "field", annotation_params);
    
    // 字段特定信息
    info.type_name = field_decl->getType().getAsString();
    
    // 检查是否为回调函数
    auto type_info = type_analyzer_->AnalyzeType(field_decl->getType());
    if (type_info.is_callback) {
        info.export_type = "callback";
        info.is_callback = true;
        info.callback_signature = info.type_name;
    }
    
    // 推导父类
    if (auto parent = clang::dyn_cast<clang::CXXRecordDecl>(field_decl->getParent())) {
        info.parent_class = parent->getNameAsString();
    }
    
    return info;
}

std::vector<InferredExportInfo> SmartInferenceEngine::InferClassMembers(
    const clang::CXXRecordDecl* class_decl) {
    
    std::vector<InferredExportInfo> members;
    
    // 遍历所有成员
    for (auto decl : class_decl->decls()) {
        // 只处理公共成员
        if (decl->getAccess() != clang::AS_public) {
            continue;
        }
        
        // 检查是否应该忽略
        if (ShouldIgnore(decl)) {
            continue;
        }
        
        if (auto method = clang::dyn_cast<clang::CXXMethodDecl>(decl)) {
            if (!method->isImplicit()) {  // 忽略隐式生成的方法
                members.push_back(InferFromMethod(method));
            }
        } else if (auto field = clang::dyn_cast<clang::FieldDecl>(decl)) {
            members.push_back(InferFromField(field));
        } else if (auto ctor = clang::dyn_cast<clang::CXXConstructorDecl>(decl)) {
            if (!ctor->isImplicit()) {
                auto info = ExtractBasicInfo(ctor, "constructor", "");
                info.parameter_types = InferParameterTypes(ctor);
                members.push_back(info);
            }
        }
    }
    
    return members;
}

std::vector<InferredExportInfo> SmartInferenceEngine::InferProperties(
    const std::vector<InferredExportInfo>& methods) {
    
    if (!options_.auto_infer_properties) {
        return {};
    }
    
    auto property_matches = property_recognizer_->RecognizeProperties(methods);
    
    std::vector<InferredExportInfo> properties;
    for (const auto& match : property_matches) {
        InferredExportInfo prop_info;
        
        prop_info.cpp_name = match.property_name;
        prop_info.lua_name = NameConverter::ToLuaName(match.property_name, options_.prefer_snake_case);
        prop_info.export_type = "property";
        prop_info.is_property = true;
        prop_info.property_access = match.is_readonly ? "readonly" : "readwrite";
        prop_info.getter_method = match.getter_name;
        prop_info.setter_method = match.setter_name;
        
        properties.push_back(prop_info);
    }
    
    return properties;
}

const std::vector<std::string>& SmartInferenceEngine::GetErrors() const {
    return errors_;
}

const std::vector<std::string>& SmartInferenceEngine::GetWarnings() const {
    return warnings_;
}

void SmartInferenceEngine::ClearErrors() {
    errors_.clear();
    warnings_.clear();
}

// 私有方法实现
InferredExportInfo SmartInferenceEngine::ExtractBasicInfo(
    const clang::NamedDecl* decl, const std::string& export_type,
    const std::string& annotation_params) {
    
    InferredExportInfo info;
    
    // 基础信息
    info.cpp_name = decl->getNameAsString();
    info.export_type = export_type;
    
    // 解析用户参数
    info.user_params = ParameterParser::Parse(annotation_params);
    
    // 推导命名空间
    info.cpp_namespace = namespace_inferrer_->InferCppNamespace(decl);
    info.lua_namespace = namespace_inferrer_->InferLuaNamespace(
        info.cpp_namespace, file_module_, info.user_params);
    
    // 推导 Lua 名称
    if (ParameterParser::HasParameter(info.user_params, "alias")) {
        info.lua_name = ParameterParser::GetParameter(info.user_params, "alias");
    } else {
        info.lua_name = NameConverter::ToLuaName(info.cpp_name, options_.prefer_snake_case);
    }
    
    // 构建合格名称
    info.qualified_name = info.cpp_namespace.empty() ? 
        info.cpp_name : (info.cpp_namespace + "::" + info.cpp_name);
    
    // 源位置信息
    auto [source_file, line_number] = GetSourceLocation(decl);
    info.source_file = source_file;
    info.line_number = line_number;
    
    // 模块信息
    if (!file_module_.empty()) {
        info.module_name = file_module_;
    }
    
    return info;
}

void SmartInferenceEngine::ApplyUserParameters(
    InferredExportInfo& info,
    const std::unordered_map<std::string, std::string>& user_params) {
    
    // 应用用户指定的参数覆盖
    for (const auto& [key, value] : user_params) {
        if (key == "alias") {
            info.lua_name = value;
        } else if (key == "namespace") {
            info.lua_namespace = (value == "global") ? "" : value;
        } else if (key == "access") {
            info.property_access = value;
        }
        // 可以添加更多参数处理
    }
}

std::vector<std::string> SmartInferenceEngine::InferBaseClasses(
    const clang::CXXRecordDecl* class_decl) {
    
    std::vector<std::string> base_classes;
    
    for (const auto& base : class_decl->bases()) {
        clang::QualType base_type = base.getType();
        base_classes.push_back(base_type.getAsString());
    }
    
    return base_classes;
}

std::vector<std::string> SmartInferenceEngine::InferParameterTypes(
    const clang::FunctionDecl* func_decl) {
    
    std::vector<std::string> param_types;
    
    for (unsigned i = 0; i < func_decl->getNumParams(); ++i) {
        auto param = func_decl->getParamDecl(i);
        param_types.push_back(param->getType().getAsString());
    }
    
    return param_types;
}

std::pair<std::string, int> SmartInferenceEngine::GetSourceLocation(const clang::Decl* decl) {
    auto& source_manager = ast_context_->getSourceManager();
    auto loc = decl->getLocation();
    
    if (loc.isValid()) {
        clang::PresumedLoc presumed_loc = source_manager.getPresumedLoc(loc);
        if (presumed_loc.isValid()) {
            std::string filename = presumed_loc.getFilename();
            int line_number = presumed_loc.getLine();
            return {filename, line_number};
        }
    }
    
    return {"", 0};
}

bool SmartInferenceEngine::ShouldIgnore(const clang::Decl* decl) {
    // 检查是否有 EXPORT_LUA_IGNORE 注解
    if (decl->hasAttrs()) {
        for (const auto& attr : decl->attrs()) {
            if (auto annotate_attr = clang::dyn_cast<clang::AnnotateAttr>(attr)) {
                std::string annotation = annotate_attr->getAnnotation().str();
                if (annotation.find("lua_export_ignore") == 0) {
                    return true;
                }
            }
        }
    }
    
    return false;
}

void SmartInferenceEngine::RecordError(const std::string& error) {
    errors_.push_back(error);
}

void SmartInferenceEngine::RecordWarning(const std::string& warning) {
    warnings_.push_back(warning);
}

bool SmartInferenceEngine::ValidateInferredInfo(const InferredExportInfo& info) {
    bool valid = true;
    
    if (info.cpp_name.empty()) {
        RecordError("Inferred export info has empty C++ name");
        valid = false;
    }
    
    if (info.lua_name.empty()) {
        RecordError("Inferred export info has empty Lua name for: " + info.cpp_name);
        valid = false;
    }
    
    if (info.export_type.empty()) {
        RecordError("Inferred export info has empty export type for: " + info.cpp_name);
        valid = false;
    }
    
    return valid;
}

} // namespace lua_binding_generator