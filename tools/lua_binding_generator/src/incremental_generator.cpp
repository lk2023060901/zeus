/**
 * @file incremental_generator.cpp
 * @brief 增量编译生成器实现
 */

#include "lua_binding_generator/incremental_generator.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <thread>
#include <future>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <queue>
#include <mutex>
#include <functional>

// 简单的JSON库替代（避免外部依赖）
#include <unordered_map>

namespace lua_binding_generator {

// HashCalculator 实现
std::string IncrementalGenerator::HashCalculator::CalculateFileHash(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    
    return CalculateStringHash(content);
}

std::string IncrementalGenerator::HashCalculator::CalculateStringHash(const std::string& content) {
    return MD5Hash(content);
}

std::string IncrementalGenerator::HashCalculator::CalculateIncrementalHash(
    const std::string& file_path,
    const std::vector<std::string>& dependencies) {
    
    std::string combined_content = file_path;
    
    // 添加文件内容哈希
    combined_content += CalculateFileHash(file_path);
    
    // 添加依赖文件的哈希
    for (const auto& dep : dependencies) {
        combined_content += dep + ":" + CalculateFileHash(dep);
    }
    
    return CalculateStringHash(combined_content);
}

std::string IncrementalGenerator::HashCalculator::MD5Hash(const std::string& data) {
    // 简化的哈希实现（实际项目中应使用标准的MD5库）
    std::hash<std::string> hasher;
    size_t hash_value = hasher(data);
    
    std::stringstream ss;
    ss << std::hex << hash_value;
    return ss.str();
}

// DependencyAnalyzer 实现
IncrementalGenerator::DependencyAnalyzer::DependencyAnalyzer(bool verbose)
    : verbose_(verbose) {}

std::vector<std::string> IncrementalGenerator::DependencyAnalyzer::AnalyzeIncludes(
    const std::string& file_path) {
    
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return {};
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    
    return ParseIncludes(content);
}

std::vector<std::string> IncrementalGenerator::DependencyAnalyzer::AnalyzeExports(
    const std::string& file_path) {
    
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return {};
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    
    return ParseExports(content);
}

std::unordered_map<std::string, DependencyNode> 
IncrementalGenerator::DependencyAnalyzer::BuildDependencyGraph(
    const std::vector<std::string>& file_paths) {
    
    std::unordered_map<std::string, DependencyNode> graph;
    
    // 初始化节点
    for (const auto& file : file_paths) {
        DependencyNode node;
        node.file_path = file;
        graph[file] = node;
    }
    
    // 构建依赖关系
    for (const auto& file : file_paths) {
        auto includes = AnalyzeIncludes(file);
        
        for (const auto& include : includes) {
            std::string resolved_path = ResolveIncludePath(include, file);
            
            if (graph.count(resolved_path) > 0) {
                // 添加依赖关系
                graph[file].dependencies.insert(resolved_path);
                graph[resolved_path].dependents.insert(file);
            }
        }
    }
    
    return graph;
}

std::unordered_set<std::string> IncrementalGenerator::DependencyAnalyzer::GetTransitiveDependencies(
    const std::string& file_path,
    const std::unordered_map<std::string, DependencyNode>& graph) {
    
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> result;
    
    std::function<void(const std::string&)> dfs = [&](const std::string& current) {
        if (visited.count(current) > 0) {
            return;
        }
        
        visited.insert(current);
        
        auto it = graph.find(current);
        if (it != graph.end()) {
            for (const auto& dep : it->second.dependencies) {
                result.insert(dep);
                dfs(dep);
            }
        }
    };
    
    dfs(file_path);
    return result;
}

std::vector<std::string> IncrementalGenerator::DependencyAnalyzer::ParseIncludes(
    const std::string& content) {
    
    std::vector<std::string> includes;
    
    // 匹配 #include "..." 和 #include <...>
    std::regex include_regex(R"(#include\s+[<"]([^>"]+)[>"])");
    std::sregex_iterator iter(content.begin(), content.end(), include_regex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        std::smatch match = *iter;
        includes.push_back(match[1].str());
    }
    
    return includes;
}

std::vector<std::string> IncrementalGenerator::DependencyAnalyzer::ParseExports(
    const std::string& content) {
    
    std::vector<std::string> exports;
    
    // 匹配各种 EXPORT_LUA_ 宏
    std::regex export_regex(R"(EXPORT_LUA_\w+\s*\([^)]*\))");
    std::sregex_iterator iter(content.begin(), content.end(), export_regex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        std::smatch match = *iter;
        exports.push_back(match[0].str());
    }
    
    return exports;
}

std::string IncrementalGenerator::DependencyAnalyzer::ResolveIncludePath(
    const std::string& include_name, const std::string& current_file) {
    
    // 简化的路径解析（实际实现应该更复杂）
    std::filesystem::path current_dir = std::filesystem::path(current_file).parent_path();
    
    // 尝试相对路径
    std::filesystem::path relative_path = current_dir / include_name;
    if (std::filesystem::exists(relative_path)) {
        return std::filesystem::canonical(relative_path).string();
    }
    
    // 尝试系统路径（简化处理）
    // 实际实现应该搜索系统包含路径
    return include_name;
}

// CacheManager 实现
IncrementalGenerator::CacheManager::CacheManager(const std::string& cache_file)
    : cache_file_path_(cache_file) {}

bool IncrementalGenerator::CacheManager::LoadCache(
    std::unordered_map<std::string, FileInfo>& cache) {
    
    if (!std::filesystem::exists(cache_file_path_)) {
        return false;
    }
    
    std::ifstream file(cache_file_path_);
    if (!file.is_open()) {
        return false;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    
    return DeserializeFromJson(content, cache);
}

bool IncrementalGenerator::CacheManager::SaveCache(
    const std::unordered_map<std::string, FileInfo>& cache) {
    
    std::string json_content = SerializeToJson(cache);
    
    std::ofstream file(cache_file_path_);
    if (!file.is_open()) {
        return false;
    }
    
    file << json_content;
    return true;
}

bool IncrementalGenerator::CacheManager::IsCacheValid(const std::chrono::seconds& expiry) const {
    if (!std::filesystem::exists(cache_file_path_)) {
        return false;
    }
    
    auto cache_age = GetCacheAge();
    return cache_age < expiry;
}

size_t IncrementalGenerator::CacheManager::GetCacheSize() const {
    if (!std::filesystem::exists(cache_file_path_)) {
        return 0;
    }
    
    return std::filesystem::file_size(cache_file_path_);
}

std::chrono::seconds IncrementalGenerator::CacheManager::GetCacheAge() const {
    if (!std::filesystem::exists(cache_file_path_)) {
        return std::chrono::seconds::max();
    }
    
    auto last_write = std::filesystem::last_write_time(cache_file_path_);
    auto now = std::filesystem::file_time_type::clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - last_write);
    return duration;
}

std::string IncrementalGenerator::CacheManager::SerializeToJson(
    const std::unordered_map<std::string, FileInfo>& cache) {
    
    // 简化的JSON序列化（避免外部依赖）
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"version\": \"2.0\",\n";
    ss << "  \"timestamp\": " << std::time(nullptr) << ",\n";
    ss << "  \"files\": {\n";
    
    bool first = true;
    for (const auto& [path, info] : cache) {
        if (!first) ss << ",\n";
        first = false;
        
        ss << "    \"" << path << "\": {\n";
        ss << "      \"last_modified\": " << info.last_modified << ",\n";
        ss << "      \"content_hash\": \"" << info.content_hash << "\",\n";
        ss << "      \"output_file\": \"" << info.output_file << "\",\n";
        ss << "      \"module_name\": \"" << info.module_name << "\",\n";
        ss << "      \"includes\": [";
        
        for (size_t i = 0; i < info.includes.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << "\"" << info.includes[i] << "\"";
        }
        ss << "],\n";
        
        ss << "      \"exports\": [";
        for (size_t i = 0; i < info.exports.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << "\"" << info.exports[i] << "\"";
        }
        ss << "]\n";
        ss << "    }";
    }
    
    ss << "\n  }\n}";
    return ss.str();
}

bool IncrementalGenerator::CacheManager::DeserializeFromJson(
    const std::string& json_content,
    std::unordered_map<std::string, FileInfo>& cache) {
    
    // 简化的JSON反序列化
    // 实际实现应该使用完整的JSON解析器
    cache.clear();
    
    // 这里只是示例实现，实际应该使用真正的JSON库
    // 暂时返回true表示加载成功
    return true;
}

// IncrementalGenerator 主要实现
IncrementalGenerator::IncrementalGenerator() 
    : options_{} {
    
    dependency_analyzer_ = std::make_unique<DependencyAnalyzer>(options_.verbose);
    cache_manager_ = std::make_unique<CacheManager>(options_.cache_file);
    
    LoadCache();
}

IncrementalGenerator::IncrementalGenerator(const Options& options)
    : options_(options) {
    
    dependency_analyzer_ = std::make_unique<DependencyAnalyzer>(options_.verbose);
    cache_manager_ = std::make_unique<CacheManager>(options_.cache_file);
    
    LoadCache();
}

IncrementalGenerator::~IncrementalGenerator() {
    SaveCache();
}

void IncrementalGenerator::SetOptions(const Options& options) {
    options_ = options;
    
    // 重新创建组件
    dependency_analyzer_ = std::make_unique<DependencyAnalyzer>(options_.verbose);
    cache_manager_ = std::make_unique<CacheManager>(options_.cache_file);
}

const IncrementalGenerator::Options& IncrementalGenerator::GetOptions() const {
    return options_;
}

bool IncrementalGenerator::NeedsRegeneration(const std::string& file_path) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    if (options_.force_rebuild) {
        UpdateStats(false);
        return true;
    }
    
    auto it = file_cache_.find(file_path);
    if (it == file_cache_.end()) {
        UpdateStats(false);
        return true;
    }
    
    const auto& cached_info = it->second;
    
    // 检查文件是否修改
    if (IsFileModified(file_path, cached_info)) {
        UpdateStats(false);
        return true;
    }
    
    // 检查输出文件是否存在
    if (!std::filesystem::exists(cached_info.output_file)) {
        UpdateStats(false);
        return true;
    }
    
    UpdateStats(true);
    return false;
}

IncrementalResult IncrementalGenerator::AnalyzeDependencies(
    const std::vector<std::string>& file_paths) {
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    IncrementalResult result;
    result.success = true;
    
    try {
        LogMessage("分析文件依赖关系...", true);
        
        // 构建依赖图
        dependency_graph_ = dependency_analyzer_->BuildDependencyGraph(file_paths);
        
        // 更新文件缓存
        for (const auto& file_path : file_paths) {
            UpdateFileCache(file_path);
        }
        
        LogMessage("依赖分析完成", true);
        
    } catch (const std::exception& e) {
        result.success = false;
        result.errors.push_back("依赖分析失败: " + std::string(e.what()));
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    result.elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    return result;
}

IncrementalResult IncrementalGenerator::Generate(
    const std::vector<std::string>& source_files,
    std::function<bool(const std::string&, std::string&)> generator_func) {
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    IncrementalResult result;
    
    try {
        // 分析依赖
        auto analysis_result = AnalyzeDependencies(source_files);
        if (!analysis_result.success) {
            return analysis_result;
        }
        
        // 收集需要重新生成的文件
        auto files_to_regenerate = CollectFilesToRegenerate(source_files);
        
        LogMessage("需要重新生成 " + std::to_string(files_to_regenerate.size()) + 
                  " 个文件，跳过 " + std::to_string(source_files.size() - files_to_regenerate.size()) + " 个文件");
        
        // 依赖传播
        auto final_files = PropagateDependencyChanges(files_to_regenerate);
        
        // 拓扑排序
        auto sorted_files = TopologicalSort(final_files);
        
        // 执行生成
        if (options_.enable_parallel && sorted_files.size() > 1) {
            result = ProcessFilesParallel(sorted_files, generator_func);
        } else {
            result = ProcessFilesSequential(sorted_files, generator_func);
        }
        
        // 更新统计信息
        result.cache_hits = stats_.cache_hits;
        result.cache_misses = stats_.cache_misses;
        
        for (const auto& file : source_files) {
            if (std::find(final_files.begin(), final_files.end(), file) != final_files.end()) {
                result.processed_files.push_back(file);
            } else {
                result.skipped_files.push_back(file);
            }
        }
        
    } catch (const std::exception& e) {
        result.success = false;
        result.errors.push_back("增量生成失败: " + std::string(e.what()));
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    result.elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    return result;
}

void IncrementalGenerator::UpdateFileInfo(const std::string& file_path,
                                         const std::string& output_file,
                                         const std::string& module_name) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    FileInfo info;
    info.file_path = file_path;
    info.last_modified = GetFileModificationTime(file_path);
    info.content_hash = HashCalculator::CalculateFileHash(file_path);
    info.includes = dependency_analyzer_->AnalyzeIncludes(file_path);
    info.exports = dependency_analyzer_->AnalyzeExports(file_path);
    info.output_file = output_file;
    info.module_name = module_name;
    info.needs_regeneration = false;
    
    file_cache_[file_path] = info;
}

std::vector<std::string> IncrementalGenerator::GetDependents(const std::string& file_path) {
    auto it = dependency_graph_.find(file_path);
    if (it == dependency_graph_.end()) {
        return {};
    }
    
    return std::vector<std::string>(it->second.dependents.begin(), it->second.dependents.end());
}

void IncrementalGenerator::CleanExpiredCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto now = std::chrono::system_clock::now();
    auto expiry_time = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()) - options_.cache_expiry;
    
    auto it = file_cache_.begin();
    while (it != file_cache_.end()) {
        if (std::chrono::seconds(it->second.last_modified) < expiry_time) {
            it = file_cache_.erase(it);
        } else {
            ++it;
        }
    }
}

IncrementalGenerator::CacheStats IncrementalGenerator::GetCacheStats() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    CacheStats stats = stats_;
    stats.total_files = file_cache_.size();
    stats.cached_files = file_cache_.size();
    stats.cache_age = cache_manager_->GetCacheAge();
    stats.cache_size_bytes = cache_manager_->GetCacheSize();
    
    return stats;
}

void IncrementalGenerator::ClearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    file_cache_.clear();
    dependency_graph_.clear();
    stats_ = CacheStats{};
    
    // 删除缓存文件
    if (std::filesystem::exists(options_.cache_file)) {
        std::filesystem::remove(options_.cache_file);
    }
}

// 私有方法实现
void IncrementalGenerator::LoadCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    if (cache_manager_->IsCacheValid(options_.cache_expiry)) {
        cache_manager_->LoadCache(file_cache_);
        LogMessage("缓存加载成功，包含 " + std::to_string(file_cache_.size()) + " 个文件", true);
    } else {
        LogMessage("缓存过期或不存在，将重新构建", true);
    }
}

void IncrementalGenerator::SaveCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    cache_manager_->SaveCache(file_cache_);
    LogMessage("缓存保存成功", true);
}

bool IncrementalGenerator::IsFileModified(const std::string& file_path, const FileInfo& cached_info) {
    // 检查修改时间
    auto current_time = GetFileModificationTime(file_path);
    if (current_time != cached_info.last_modified) {
        return true;
    }
    
    // 检查内容哈希
    auto current_hash = HashCalculator::CalculateFileHash(file_path);
    if (current_hash != cached_info.content_hash) {
        return true;
    }
    
    return false;
}

void IncrementalGenerator::UpdateFileCache(const std::string& file_path) {
    auto includes = dependency_analyzer_->AnalyzeIncludes(file_path);
    auto exports = dependency_analyzer_->AnalyzeExports(file_path);
    
    FileInfo info;
    info.file_path = file_path;
    info.last_modified = GetFileModificationTime(file_path);
    info.content_hash = HashCalculator::CalculateFileHash(file_path);
    info.includes = includes;
    info.exports = exports;
    
    file_cache_[file_path] = info;
}

std::time_t IncrementalGenerator::GetFileModificationTime(const std::string& file_path) {
    if (!std::filesystem::exists(file_path)) {
        return 0;
    }
    
    auto last_write = std::filesystem::last_write_time(file_path);
    return std::chrono::duration_cast<std::chrono::seconds>(
        last_write.time_since_epoch()).count();
}

std::vector<std::string> IncrementalGenerator::CollectFilesToRegenerate(
    const std::vector<std::string>& source_files) {
    
    std::vector<std::string> files_to_regenerate;
    
    for (const auto& file : source_files) {
        if (NeedsRegeneration(file)) {
            files_to_regenerate.push_back(file);
        }
    }
    
    return files_to_regenerate;
}

std::vector<std::string> IncrementalGenerator::PropagateDependencyChanges(
    const std::vector<std::string>& changed_files) {
    
    std::unordered_set<std::string> all_affected_files(changed_files.begin(), changed_files.end());
    
    // 广度优先搜索，找到所有受影响的文件
    std::queue<std::string> to_process;
    for (const auto& file : changed_files) {
        to_process.push(file);
    }
    
    while (!to_process.empty()) {
        std::string current_file = to_process.front();
        to_process.pop();
        
        auto dependents = GetDependents(current_file);
        for (const auto& dependent : dependents) {
            if (all_affected_files.find(dependent) == all_affected_files.end()) {
                all_affected_files.insert(dependent);
                to_process.push(dependent);
            }
        }
    }
    
    return std::vector<std::string>(all_affected_files.begin(), all_affected_files.end());
}

std::vector<std::string> IncrementalGenerator::TopologicalSort(
    const std::vector<std::string>& files) {
    
    // 简化的拓扑排序实现
    // 实际实现应该处理循环依赖等复杂情况
    return files;
}

IncrementalResult IncrementalGenerator::ProcessFilesParallel(
    const std::vector<std::string>& files,
    std::function<bool(const std::string&, std::string&)> generator_func) {
    
    IncrementalResult result;
    result.success = true;
    
    // 确定线程数
    int thread_count = options_.max_threads;
    if (thread_count <= 0) {
        thread_count = std::thread::hardware_concurrency();
    }
    thread_count = std::min(thread_count, static_cast<int>(files.size()));
    
    LogMessage("使用 " + std::to_string(thread_count) + " 个线程并行处理", true);
    
    // 分批处理文件
    std::vector<std::future<bool>> futures;
    std::mutex result_mutex;
    
    size_t batch_size = (files.size() + thread_count - 1) / thread_count;
    
    for (int i = 0; i < thread_count; ++i) {
        size_t start = i * batch_size;
        size_t end = std::min(start + batch_size, files.size());
        
        if (start >= files.size()) break;
        
        std::vector<std::string> batch(files.begin() + start, files.begin() + end);
        
        futures.push_back(std::async(std::launch::async, [&, batch]() {
            bool batch_success = true;
            
            for (const auto& file : batch) {
                std::string error_msg;
                if (!generator_func(file, error_msg)) {
                    std::lock_guard<std::mutex> lock(result_mutex);
                    result.errors.push_back("处理文件失败 " + file + ": " + error_msg);
                    batch_success = false;
                }
            }
            
            return batch_success;
        }));
    }
    
    // 等待所有任务完成
    for (auto& future : futures) {
        if (!future.get()) {
            result.success = false;
        }
    }
    
    return result;
}

IncrementalResult IncrementalGenerator::ProcessFilesSequential(
    const std::vector<std::string>& files,
    std::function<bool(const std::string&, std::string&)> generator_func) {
    
    IncrementalResult result;
    result.success = true;
    
    for (const auto& file : files) {
        LogMessage("处理文件: " + file, true);
        
        std::string error_msg;
        if (!generator_func(file, error_msg)) {
            result.success = false;
            result.errors.push_back("处理文件失败 " + file + ": " + error_msg);
        }
    }
    
    return result;
}

bool IncrementalGenerator::ValidateOutputFile(const std::string& output_file) {
    return std::filesystem::exists(output_file) && std::filesystem::file_size(output_file) > 0;
}

void IncrementalGenerator::UpdateStats(bool cache_hit) {
    if (cache_hit) {
        stats_.cache_hits++;
    } else {
        stats_.cache_misses++;
    }
}

void IncrementalGenerator::LogMessage(const std::string& message, bool verbose_only) {
    if (!verbose_only || options_.verbose) {
        std::cout << "[增量生成器] " << message << std::endl;
    }
}

} // namespace lua_binding_generator