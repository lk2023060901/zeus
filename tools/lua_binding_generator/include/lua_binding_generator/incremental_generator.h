/**
 * @file incremental_generator.h
 * @brief 增量编译生成器 - 只重新生成变更的文件
 * 
 * 这个组件负责跟踪文件变更，实现增量编译功能：
 * 1. 文件依赖跟踪和变更检测
 * 2. 内容哈希缓存机制
 * 3. 智能依赖分析
 * 4. 高效的缓存存储和加载
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <chrono>
#include <filesystem>
#include <functional>
#include <mutex>

namespace lua_binding_generator {

/**
 * @brief 文件信息结构
 */
struct FileInfo {
    std::string file_path;                          ///< 文件路径
    std::time_t last_modified = 0;                  ///< 最后修改时间
    std::string content_hash;                       ///< 文件内容哈希值
    std::vector<std::string> includes;              ///< 包含的头文件依赖
    std::vector<std::string> exports;               ///< 导出的符号列表
    std::string output_file;                        ///< 对应的输出文件
    bool needs_regeneration = false;                ///< 是否需要重新生成
    std::string module_name;                        ///< 所属模块名
};

/**
 * @brief 依赖图节点
 */
struct DependencyNode {
    std::string file_path;                          ///< 文件路径
    std::unordered_set<std::string> dependencies;   ///< 依赖的文件
    std::unordered_set<std::string> dependents;     ///< 依赖此文件的文件
    bool visited = false;                           ///< 遍历标记
};

/**
 * @brief 增量生成结果
 */
struct IncrementalResult {
    bool success = false;                           ///< 是否成功
    std::vector<std::string> processed_files;       ///< 处理的文件列表
    std::vector<std::string> skipped_files;         ///< 跳过的文件列表
    std::vector<std::string> updated_files;         ///< 更新的文件列表
    std::vector<std::string> errors;                ///< 错误信息
    std::vector<std::string> warnings;              ///< 警告信息
    std::chrono::milliseconds elapsed_time{0};      ///< 耗时
    size_t cache_hits = 0;                          ///< 缓存命中数
    size_t cache_misses = 0;                        ///< 缓存未命中数
};

/**
 * @brief 增量编译生成器
 */
class IncrementalGenerator {
public:
    /**
     * @brief 增量编译选项
     */
    struct Options {
        std::string cache_file = ".lua_binding_cache";  ///< 缓存文件路径
        bool force_rebuild = false;                     ///< 强制重新构建
        bool enable_parallel = true;                    ///< 启用并行处理
        int max_threads = 0;                            ///< 最大线程数（0为自动）
        bool verbose = false;                           ///< 详细输出
        std::chrono::seconds cache_expiry{3600};        ///< 缓存过期时间
    };

public:
    /**
     * @brief 构造函数
     */
    IncrementalGenerator();
    explicit IncrementalGenerator(const Options& options);

    /**
     * @brief 析构函数
     */
    ~IncrementalGenerator();

    /**
     * @brief 设置选项
     */
    void SetOptions(const Options& options);

    /**
     * @brief 获取选项
     */
    const Options& GetOptions() const;

    /**
     * @brief 检查文件是否需要重新生成
     * @param file_path 源文件路径
     * @return 是否需要重新生成
     */
    bool NeedsRegeneration(const std::string& file_path);

    /**
     * @brief 分析文件依赖关系
     * @param file_paths 要分析的文件列表
     * @return 分析结果
     */
    IncrementalResult AnalyzeDependencies(const std::vector<std::string>& file_paths);

    /**
     * @brief 执行增量生成
     * @param source_files 源文件列表
     * @param generator_func 生成器函数
     * @return 生成结果
     */
    IncrementalResult Generate(
        const std::vector<std::string>& source_files,
        std::function<bool(const std::string&, std::string&)> generator_func);

    /**
     * @brief 更新文件信息
     * @param file_path 文件路径
     * @param output_file 输出文件路径
     * @param module_name 模块名（可选）
     */
    void UpdateFileInfo(const std::string& file_path, 
                       const std::string& output_file,
                       const std::string& module_name = "");

    /**
     * @brief 获取依赖此文件的所有文件
     * @param file_path 文件路径
     * @return 依赖文件列表
     */
    std::vector<std::string> GetDependents(const std::string& file_path);

    /**
     * @brief 清理过期缓存
     */
    void CleanExpiredCache();

    /**
     * @brief 获取缓存统计信息
     */
    struct CacheStats {
        size_t total_files = 0;
        size_t cached_files = 0;
        size_t cache_hits = 0;
        size_t cache_misses = 0;
        std::chrono::seconds cache_age{0};
        size_t cache_size_bytes = 0;
    };

    CacheStats GetCacheStats() const;

    /**
     * @brief 清除所有缓存
     */
    void ClearCache();

private:
    Options options_;                                           ///< 配置选项
    std::unordered_map<std::string, FileInfo> file_cache_;     ///< 文件缓存
    std::unordered_map<std::string, DependencyNode> dependency_graph_; ///< 依赖图
    mutable std::mutex cache_mutex_;                            ///< 缓存互斥锁
    mutable CacheStats stats_;                                  ///< 缓存统计

    /**
     * @brief 文件哈希计算器
     */
    class HashCalculator {
    public:
        /**
         * @brief 计算文件内容哈希
         */
        static std::string CalculateFileHash(const std::string& file_path);

        /**
         * @brief 计算字符串哈希
         */
        static std::string CalculateStringHash(const std::string& content);

        /**
         * @brief 计算增量哈希（基于多个因素）
         */
        static std::string CalculateIncrementalHash(
            const std::string& file_path,
            const std::vector<std::string>& dependencies);

    private:
        /**
         * @brief MD5 哈希实现
         */
        static std::string MD5Hash(const std::string& data);
    };

    /**
     * @brief 依赖分析器
     */
    class DependencyAnalyzer {
    public:
        explicit DependencyAnalyzer(bool verbose = false);

        /**
         * @brief 分析文件的包含依赖
         */
        std::vector<std::string> AnalyzeIncludes(const std::string& file_path);

        /**
         * @brief 分析文件的导出符号
         */
        std::vector<std::string> AnalyzeExports(const std::string& file_path);

        /**
         * @brief 构建依赖图
         */
        std::unordered_map<std::string, DependencyNode> BuildDependencyGraph(
            const std::vector<std::string>& file_paths);

        /**
         * @brief 获取传递依赖
         */
        std::unordered_set<std::string> GetTransitiveDependencies(
            const std::string& file_path,
            const std::unordered_map<std::string, DependencyNode>& graph);

    private:
        bool verbose_;

        /**
         * @brief 解析 #include 指令
         */
        std::vector<std::string> ParseIncludes(const std::string& content);

        /**
         * @brief 解析 EXPORT_LUA 宏
         */
        std::vector<std::string> ParseExports(const std::string& content);

        /**
         * @brief 解析文件路径
         */
        std::string ResolveIncludePath(const std::string& include_name,
                                      const std::string& current_file);
    };

    /**
     * @brief 缓存管理器
     */
    class CacheManager {
    public:
        explicit CacheManager(const std::string& cache_file);

        /**
         * @brief 加载缓存
         */
        bool LoadCache(std::unordered_map<std::string, FileInfo>& cache);

        /**
         * @brief 保存缓存
         */
        bool SaveCache(const std::unordered_map<std::string, FileInfo>& cache);

        /**
         * @brief 检查缓存是否有效
         */
        bool IsCacheValid(const std::chrono::seconds& expiry) const;

        /**
         * @brief 获取缓存文件大小
         */
        size_t GetCacheSize() const;

        /**
         * @brief 获取缓存年龄
         */
        std::chrono::seconds GetCacheAge() const;

    private:
        std::string cache_file_path_;

        /**
         * @brief JSON 序列化
         */
        std::string SerializeToJson(const std::unordered_map<std::string, FileInfo>& cache);

        /**
         * @brief JSON 反序列化
         */
        bool DeserializeFromJson(const std::string& json_content,
                                std::unordered_map<std::string, FileInfo>& cache);
    };

    std::unique_ptr<DependencyAnalyzer> dependency_analyzer_;   ///< 依赖分析器
    std::unique_ptr<CacheManager> cache_manager_;               ///< 缓存管理器

    // 内部方法

    /**
     * @brief 加载缓存
     */
    void LoadCache();

    /**
     * @brief 保存缓存
     */
    void SaveCache();

    /**
     * @brief 检查文件是否修改
     */
    bool IsFileModified(const std::string& file_path, const FileInfo& cached_info);

    /**
     * @brief 更新文件信息缓存
     */
    void UpdateFileCache(const std::string& file_path);

    /**
     * @brief 获取文件的最后修改时间
     */
    std::time_t GetFileModificationTime(const std::string& file_path);

    /**
     * @brief 收集需要重新生成的文件
     */
    std::vector<std::string> CollectFilesToRegenerate(const std::vector<std::string>& source_files);

    /**
     * @brief 执行依赖传播（当文件A变更时，找到所有依赖A的文件）
     */
    std::vector<std::string> PropagateDependencyChanges(const std::vector<std::string>& changed_files);

    /**
     * @brief 拓扑排序（确保按依赖顺序处理文件）
     */
    std::vector<std::string> TopologicalSort(const std::vector<std::string>& files);

    /**
     * @brief 并行处理文件
     */
    IncrementalResult ProcessFilesParallel(
        const std::vector<std::string>& files,
        std::function<bool(const std::string&, std::string&)> generator_func);

    /**
     * @brief 串行处理文件
     */
    IncrementalResult ProcessFilesSequential(
        const std::vector<std::string>& files,
        std::function<bool(const std::string&, std::string&)> generator_func);

    /**
     * @brief 验证输出文件
     */
    bool ValidateOutputFile(const std::string& output_file);

    /**
     * @brief 记录统计信息
     */
    void UpdateStats(bool cache_hit);

    /**
     * @brief 日志输出
     */
    void LogMessage(const std::string& message, bool verbose_only = false);
};

} // namespace lua_binding_generator