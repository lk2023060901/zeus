#include "lua_binding_generator/ast_visitor.h"
#include <clang/Basic/SourceManager.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Type.h>
#include <sstream>
#include <algorithm>
#include <regex>

namespace lua_binding_generator {

LuaASTVisitor::LuaASTVisitor(clang::ASTContext* context)
    : context_(context), processed_files_(0) {
}

bool LuaASTVisitor::VisitCXXRecordDecl(clang::CXXRecordDecl* decl) {
    if (!decl || !decl->hasDefinition()) {
        return true;
    }
    
    std::string export_type;
    if (!HasLuaExportAnnotation(decl, export_type)) {
        return true;
    }
    
    // 检查是否应该忽略
    if (ShouldIgnoreDeclaration(decl)) {
        return true;
    }
    
    ExportInfo info(ExportInfo::Type::Class, decl->getNameAsString());
    info.qualified_name = GetQualifiedName(decl);
    info.source_location = GetSourceLocationString(decl->getLocation());
    info.file_path = GetFilePath(decl->getLocation());
    info.attributes = ParseAnnotationAttributes(export_type);
    info.base_classes = ExtractBaseClasses(decl);
    
    // 提取命名空间信息
    if (auto ns_decl = llvm::dyn_cast_or_null<clang::NamespaceDecl>(decl->getDeclContext())) {
        info.namespace_name = ns_decl->getNameAsString();
    }
    
    if (ValidateExportInfo(info)) {
        exported_items_.push_back(std::move(info));
    }
    
    return true;
}

bool LuaASTVisitor::VisitCXXMethodDecl(clang::CXXMethodDecl* decl) {
    if (!decl) {
        return true;
    }
    
    std::string export_type;
    if (!HasLuaExportAnnotation(decl, export_type)) {
        return true;
    }
    
    if (ShouldIgnoreDeclaration(decl)) {
        return true;
    }
    
    ExportInfo::Type info_type = ExportInfo::Type::Method;
    if (decl->isStatic()) {
        info_type = ExportInfo::Type::StaticMethod;
    }
    
    ExportInfo info(info_type, decl->getNameAsString());
    info.qualified_name = GetQualifiedName(decl);
    info.source_location = GetSourceLocationString(decl->getLocation());
    info.file_path = GetFilePath(decl->getLocation());
    info.attributes = ParseAnnotationAttributes(export_type);
    info.return_type = ExtractTypeInfo(decl->getReturnType());
    info.is_static = decl->isStatic();
    info.is_const = decl->isConst();
    info.is_virtual = decl->isVirtual();
    
    ExtractParameterInfo(decl, info.parameter_types, info.parameter_names);
    
    // 获取所属类信息
    if (auto parent_class = decl->getParent()) {
        info.owner_class = parent_class->getNameAsString();
    }
    
    if (ValidateExportInfo(info)) {
        exported_items_.push_back(std::move(info));
    }
    
    return true;
}

bool LuaASTVisitor::VisitCXXConstructorDecl(clang::CXXConstructorDecl* decl) {
    if (!decl) {
        return true;
    }
    
    std::string export_type;
    if (!HasLuaExportAnnotation(decl, export_type)) {
        return true;
    }
    
    if (ShouldIgnoreDeclaration(decl)) {
        return true;
    }
    
    ExportInfo info(ExportInfo::Type::Constructor, decl->getParent()->getNameAsString());
    info.qualified_name = GetQualifiedName(decl);
    info.source_location = GetSourceLocationString(decl->getLocation());
    info.file_path = GetFilePath(decl->getLocation());
    info.attributes = ParseAnnotationAttributes(export_type);
    
    ExtractParameterInfo(decl, info.parameter_types, info.parameter_names);
    
    if (auto parent_class = decl->getParent()) {
        info.owner_class = parent_class->getNameAsString();
    }
    
    if (ValidateExportInfo(info)) {
        exported_items_.push_back(std::move(info));
    }
    
    return true;
}

bool LuaASTVisitor::VisitFieldDecl(clang::FieldDecl* decl) {
    if (!decl) {
        return true;
    }
    
    std::string export_type;
    if (!HasLuaExportAnnotation(decl, export_type)) {
        return true;
    }
    
    if (ShouldIgnoreDeclaration(decl)) {
        return true;
    }
    
    ExportInfo info(ExportInfo::Type::Property, decl->getNameAsString());
    info.qualified_name = GetQualifiedName(decl);
    info.source_location = GetSourceLocationString(decl->getLocation());
    info.file_path = GetFilePath(decl->getLocation());
    info.attributes = ParseAnnotationAttributes(export_type);
    info.return_type = ExtractTypeInfo(decl->getType());
    info.access_type = DetermineAccessType(info.attributes);
    
    if (auto parent_class = llvm::dyn_cast_or_null<clang::CXXRecordDecl>(decl->getDeclContext())) {
        info.owner_class = parent_class->getNameAsString();
    }
    
    if (ValidateExportInfo(info)) {
        exported_items_.push_back(std::move(info));
    }
    
    return true;
}

bool LuaASTVisitor::VisitFunctionDecl(clang::FunctionDecl* decl) {
    if (!decl || llvm::isa<clang::CXXMethodDecl>(decl)) {
        return true; // CXXMethodDecl 由 VisitCXXMethodDecl 处理
    }
    
    std::string export_type;
    if (!HasLuaExportAnnotation(decl, export_type)) {
        return true;
    }
    
    if (ShouldIgnoreDeclaration(decl)) {
        return true;
    }
    
    ExportInfo info(ExportInfo::Type::Function, decl->getNameAsString());
    info.qualified_name = GetQualifiedName(decl);
    info.source_location = GetSourceLocationString(decl->getLocation());
    info.file_path = GetFilePath(decl->getLocation());
    info.attributes = ParseAnnotationAttributes(export_type);
    info.return_type = ExtractTypeInfo(decl->getReturnType());
    
    ExtractParameterInfo(decl, info.parameter_types, info.parameter_names);
    
    if (ValidateExportInfo(info)) {
        exported_items_.push_back(std::move(info));
    }
    
    return true;
}

bool LuaASTVisitor::VisitEnumDecl(clang::EnumDecl* decl) {
    if (!decl) {
        return true;
    }
    
    std::string export_type;
    if (!HasLuaExportAnnotation(decl, export_type)) {
        return true;
    }
    
    if (ShouldIgnoreDeclaration(decl)) {
        return true;
    }
    
    ExportInfo info(ExportInfo::Type::Enum, decl->getNameAsString());
    info.qualified_name = GetQualifiedName(decl);
    info.source_location = GetSourceLocationString(decl->getLocation());
    info.file_path = GetFilePath(decl->getLocation());
    info.attributes = ParseAnnotationAttributes(export_type);
    
    if (ValidateExportInfo(info)) {
        exported_items_.push_back(std::move(info));
    }
    
    return true;
}

bool LuaASTVisitor::VisitVarDecl(clang::VarDecl* decl) {
    if (!decl) {
        return true;
    }
    
    std::string export_type;
    if (!HasLuaExportAnnotation(decl, export_type)) {
        return true;
    }
    
    if (ShouldIgnoreDeclaration(decl)) {
        return true;
    }
    
    ExportInfo info(ExportInfo::Type::Constant, decl->getNameAsString());
    info.qualified_name = GetQualifiedName(decl);
    info.source_location = GetSourceLocationString(decl->getLocation());
    info.file_path = GetFilePath(decl->getLocation());
    info.attributes = ParseAnnotationAttributes(export_type);
    info.return_type = ExtractTypeInfo(decl->getType());
    
    if (ValidateExportInfo(info)) {
        exported_items_.push_back(std::move(info));
    }
    
    return true;
}

bool LuaASTVisitor::VisitNamespaceDecl(clang::NamespaceDecl* decl) {
    if (!decl) {
        return true;
    }
    
    std::string export_type;
    if (!HasLuaExportAnnotation(decl, export_type)) {
        return true;
    }
    
    if (ShouldIgnoreDeclaration(decl)) {
        return true;
    }
    
    ExportInfo info(ExportInfo::Type::Namespace, decl->getNameAsString());
    info.qualified_name = GetQualifiedName(decl);
    info.source_location = GetSourceLocationString(decl->getLocation());
    info.file_path = GetFilePath(decl->getLocation());
    info.attributes = ParseAnnotationAttributes(export_type);
    
    if (ValidateExportInfo(info)) {
        exported_items_.push_back(std::move(info));
    }
    
    return true;
}

const std::vector<ExportInfo>& LuaASTVisitor::GetExportedItems() const {
    return exported_items_;
}

void LuaASTVisitor::ClearExportedItems() {
    exported_items_.clear();
    errors_.clear();
    processed_files_ = 0;
}

size_t LuaASTVisitor::GetProcessedFileCount() const {
    return processed_files_;
}

const std::vector<std::string>& LuaASTVisitor::GetErrors() const {
    return errors_;
}

bool LuaASTVisitor::HasLuaExportAnnotation(const clang::Decl* decl, std::string& export_type) {
    if (!decl) {
        return false;
    }
    
    for (const auto* attr : decl->attrs()) {
        if (const auto* annotate_attr = llvm::dyn_cast<clang::AnnotateAttr>(attr)) {
            std::string annotation = annotate_attr->getAnnotation().str();
            if (annotation.find("lua_export_") == 0) {
                export_type = annotation;
                return true;
            }
        }
    }
    
    return false;
}

std::map<std::string, std::string> LuaASTVisitor::ParseAnnotationAttributes(const std::string& annotation) {
    std::map<std::string, std::string> attributes;
    
    // 解析格式: "lua_export_type:name:attr1=value1,attr2=value2"
    std::vector<std::string> parts;
    std::stringstream ss(annotation);
    std::string part;
    
    while (std::getline(ss, part, ':')) {
        parts.push_back(part);
    }
    
    if (parts.size() >= 3) {
        std::string attrs_str = parts[2];
        std::stringstream attr_ss(attrs_str);
        std::string attr_pair;
        
        while (std::getline(attr_ss, attr_pair, ',')) {
            auto eq_pos = attr_pair.find('=');
            if (eq_pos != std::string::npos) {
                std::string key = attr_pair.substr(0, eq_pos);
                std::string value = attr_pair.substr(eq_pos + 1);
                
                // 移除前后空白
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                
                attributes[key] = value;
            } else if (!attr_pair.empty()) {
                // 布尔属性，没有值
                attr_pair.erase(0, attr_pair.find_first_not_of(" \t"));
                attr_pair.erase(attr_pair.find_last_not_of(" \t") + 1);
                attributes[attr_pair] = "true";
            }
        }
    }
    
    return attributes;
}

std::string LuaASTVisitor::ExtractTypeInfo(const clang::QualType& type) {
    if (type.isNull()) {
        return "void";
    }
    
    return type.getAsString(context_->getPrintingPolicy());
}

void LuaASTVisitor::ExtractParameterInfo(const clang::FunctionDecl* function_decl,
                                        std::vector<std::string>& param_types,
                                        std::vector<std::string>& param_names) {
    param_types.clear();
    param_names.clear();
    
    for (unsigned i = 0; i < function_decl->getNumParams(); ++i) {
        const auto* param = function_decl->getParamDecl(i);
        param_types.push_back(ExtractTypeInfo(param->getType()));
        param_names.push_back(param->getNameAsString());
    }
}

std::string LuaASTVisitor::GetQualifiedName(const clang::NamedDecl* decl) {
    if (!decl) {
        return "";
    }
    
    std::string qualified_name;
    llvm::raw_string_ostream stream(qualified_name);
    decl->printQualifiedName(stream);
    stream.flush();
    
    return qualified_name;
}

std::string LuaASTVisitor::GetSourceLocationString(const clang::SourceLocation& loc) {
    if (loc.isInvalid()) {
        return "unknown";
    }
    
    const auto& source_manager = context_->getSourceManager();
    clang::PresumedLoc presumed_loc = source_manager.getPresumedLoc(loc);
    
    if (presumed_loc.isInvalid()) {
        return "unknown";
    }
    
    std::ostringstream oss;
    oss << presumed_loc.getFilename() << ":" << presumed_loc.getLine() << ":" << presumed_loc.getColumn();
    return oss.str();
}

std::string LuaASTVisitor::GetFilePath(const clang::SourceLocation& loc) {
    if (loc.isInvalid()) {
        return "";
    }
    
    const auto& source_manager = context_->getSourceManager();
    clang::PresumedLoc presumed_loc = source_manager.getPresumedLoc(loc);
    
    if (presumed_loc.isInvalid()) {
        return "";
    }
    
    return presumed_loc.getFilename();
}

ExportInfo::AccessType LuaASTVisitor::DetermineAccessType(const std::map<std::string, std::string>& attr_map) {
    auto access_it = attr_map.find("access");
    if (access_it == attr_map.end()) {
        // 检查老式的访问类型参数
        if (attr_map.find("readonly") != attr_map.end()) {
            return ExportInfo::AccessType::ReadOnly;
        }
        if (attr_map.find("readwrite") != attr_map.end()) {
            return ExportInfo::AccessType::ReadWrite;
        }
        return ExportInfo::AccessType::None;
    }
    
    const std::string& access = access_it->second;
    if (access == "readonly") {
        return ExportInfo::AccessType::ReadOnly;
    } else if (access == "readwrite") {
        return ExportInfo::AccessType::ReadWrite;
    } else if (access == "writeonly") {
        return ExportInfo::AccessType::WriteOnly;
    }
    
    return ExportInfo::AccessType::None;
}

std::vector<std::string> LuaASTVisitor::ExtractBaseClasses(const clang::CXXRecordDecl* record_decl) {
    std::vector<std::string> base_classes;
    
    if (!record_decl) {
        return base_classes;
    }
    
    for (const auto& base : record_decl->bases()) {
        clang::QualType base_type = base.getType();
        std::string base_name = base_type.getAsString(context_->getPrintingPolicy());
        base_classes.push_back(base_name);
    }
    
    return base_classes;
}

bool LuaASTVisitor::ShouldIgnoreDeclaration(const clang::Decl* decl) {
    if (!decl) {
        return true;
    }
    
    // 检查是否有 EXPORT_LUA_IGNORE 标记
    for (const auto* attr : decl->attrs()) {
        if (const auto* annotate_attr = llvm::dyn_cast<clang::AnnotateAttr>(attr)) {
            std::string annotation = annotate_attr->getAnnotation().str();
            if (annotation.find("lua_export_ignore") == 0) {
                return true;
            }
        }
    }
    
    // 检查是否在系统头文件中
    const auto& source_manager = context_->getSourceManager();
    if (source_manager.isInSystemHeader(decl->getLocation())) {
        return true;
    }
    
    return false;
}

void LuaASTVisitor::RecordError(const std::string& error, const clang::SourceLocation& loc) {
    std::string full_error = error;
    if (loc.isValid()) {
        full_error += " at " + GetSourceLocationString(loc);
    }
    errors_.push_back(full_error);
}

bool LuaASTVisitor::ValidateExportInfo(const ExportInfo& info) {
    if (info.name.empty()) {
        RecordError("Export info has empty name");
        return false;
    }
    
    if (info.type == ExportInfo::Type::Method || info.type == ExportInfo::Type::Function) {
        if (info.return_type.empty()) {
            RecordError("Method/Function has empty return type: " + info.name);
            return false;
        }
    }
    
    return true;
}

} // namespace lua_binding_generator