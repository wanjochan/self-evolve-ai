/**
 * observability_system.h - Comprehensive Observability System
 * 
 * Header for real-time collection of execution traces, performance metrics,
 * and AI-driven analysis of code execution patterns
 */

#ifndef OBSERVABILITY_SYSTEM_H
#define OBSERVABILITY_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Trace event types
typedef enum {
    TRACE_FUNCTION_ENTER = 1,
    TRACE_FUNCTION_EXIT = 2,
    TRACE_INSTRUCTION_EXECUTE = 3,
    TRACE_MEMORY_ALLOCATE = 4,
    TRACE_MEMORY_FREE = 5,
    TRACE_SYSTEM_CALL = 6,
    TRACE_EXCEPTION = 7,
    TRACE_BRANCH_TAKEN = 8,
    TRACE_LOOP_ITERATION = 9
} TraceEventType;

// Execution trace event
typedef struct {
    TraceEventType type;
    uint64_t timestamp_ns;
    uint64_t thread_id;
    void* instruction_pointer;
    void* stack_pointer;
    
    union {
        struct {
            char function_name[128];
            int parameter_count;
            uint64_t parameters[8];
        } function_call;
        
        struct {
            int instruction; // ASTNodeType
            uint64_t operands[4];
            int operand_count;
        } instruction;
        
        struct {
            void* address;
            size_t size;
            bool is_allocation;
        } memory;
        
        struct {
            int syscall_number;
            uint64_t arguments[6];
            int64_t return_value;
        } syscall;
        
        struct {
            int exception_type;
            char message[256];
        } exception;
        
        struct {
            void* branch_address;
            bool taken;
            int prediction_accuracy;
        } branch;
    } data;
} TraceEvent;

// Performance metrics snapshot
typedef struct {
    uint64_t timestamp_ns;
    
    // CPU metrics
    double cpu_utilization;
    uint64_t instruction_count;
    uint64_t cycle_count;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t branch_predictions;
    uint64_t branch_mispredictions;
    
    // Memory metrics
    size_t memory_usage_bytes;
    size_t peak_memory_usage;
    uint64_t memory_allocations;
    uint64_t memory_deallocations;
    size_t memory_fragmentation;
    
    // I/O metrics
    uint64_t disk_reads;
    uint64_t disk_writes;
    uint64_t network_packets_sent;
    uint64_t network_packets_received;
    
    // VM metrics
    uint64_t jit_compilations;
    uint64_t garbage_collections;
    uint64_t module_loads;
    uint64_t function_calls;
} PerformanceSnapshot;

// Execution pattern
typedef struct {
    char pattern_id[64];
    char description[256];
    double frequency;
    double performance_impact;
    uint64_t occurrence_count;
    time_t first_seen;
    time_t last_seen;
    
    // Pattern characteristics
    bool is_hot_path;
    bool is_bottleneck;
    bool is_optimization_candidate;
    double optimization_potential;
} ExecutionPattern;

// Resource usage tracking
typedef struct {
    uint64_t timestamp_ns;
    
    // System resources
    double cpu_usage_percent;
    size_t memory_usage_bytes;
    size_t disk_usage_bytes;
    size_t network_bandwidth_used;
    
    // Application resources
    int thread_count;
    int file_descriptor_count;
    int socket_count;
    int module_count;
    
    // Limits and thresholds
    size_t memory_limit;
    double cpu_limit;
    bool resource_pressure;
} ResourceUsage;

// Observability configuration
typedef struct {
    bool tracing_enabled;
    bool metrics_collection_enabled;
    bool pattern_analysis_enabled;
    uint64_t trace_sampling_rate;
    uint64_t metrics_collection_interval_ns;
    size_t max_trace_events;
    size_t trace_buffer_size;
    size_t snapshot_buffer_size;
} ObservabilityConfig;

// Core observability functions

/**
 * Initialize observability system
 * @return 0 on success, -1 on error
 */
int observability_system_init(void);

/**
 * Cleanup observability system
 */
void observability_system_cleanup(void);

/**
 * Configure observability system
 * @param config Configuration settings
 * @return 0 on success, -1 on error
 */
int configure_observability_system(const ObservabilityConfig* config);

/**
 * Get observability configuration
 * @param config Pointer to store configuration
 */
void get_observability_config(ObservabilityConfig* config);

// Trace collection

/**
 * Record trace event
 * @param type Event type
 * @param instruction_pointer Current instruction pointer
 * @param event_data Type-specific event data
 * @return 0 on success, -1 on error
 */
int record_trace_event(TraceEventType type, void* instruction_pointer, const void* event_data);

/**
 * Enable/disable tracing
 * @param enabled Whether to enable tracing
 */
void set_tracing_enabled(bool enabled);

/**
 * Set trace sampling rate
 * @param rate Sampling rate (1 = every event, 10 = every 10th event)
 */
void set_trace_sampling_rate(uint64_t rate);

/**
 * Get trace events
 * @param events Array to store trace events
 * @param max_events Maximum number of events to return
 * @param start_index Starting index in trace buffer
 * @return Number of events returned
 */
int get_trace_events(TraceEvent* events, size_t max_events, size_t start_index);

/**
 * Clear trace buffer
 */
void clear_trace_buffer(void);

// Performance monitoring

/**
 * Capture performance snapshot
 * @return 0 on success, -1 on error
 */
int capture_performance_snapshot(void);

/**
 * Enable/disable metrics collection
 * @param enabled Whether to enable metrics collection
 */
void set_metrics_collection_enabled(bool enabled);

/**
 * Set metrics collection interval
 * @param interval_ns Interval in nanoseconds
 */
void set_metrics_collection_interval(uint64_t interval_ns);

/**
 * Get performance snapshots
 * @param snapshots Array to store snapshots
 * @param max_snapshots Maximum number of snapshots to return
 * @param start_index Starting index in snapshot buffer
 * @return Number of snapshots returned
 */
int get_performance_snapshots(PerformanceSnapshot* snapshots, size_t max_snapshots, size_t start_index);

/**
 * Get latest performance snapshot
 * @param snapshot Pointer to store latest snapshot
 * @return 0 on success, -1 on error
 */
int get_latest_performance_snapshot(PerformanceSnapshot* snapshot);

// Pattern analysis

/**
 * Analyze execution patterns
 * @return Number of patterns detected, -1 on error
 */
int analyze_execution_patterns(void);

/**
 * Enable/disable pattern analysis
 * @param enabled Whether to enable pattern analysis
 */
void set_pattern_analysis_enabled(bool enabled);

/**
 * Get execution patterns
 * @param patterns Array to store patterns
 * @param max_patterns Maximum number of patterns to return
 * @return Number of patterns returned
 */
int get_execution_patterns(ExecutionPattern* patterns, size_t max_patterns);

/**
 * Find patterns by type
 * @param pattern_type Pattern type to search for
 * @param patterns Array to store matching patterns
 * @param max_patterns Maximum number of patterns to return
 * @return Number of matching patterns found
 */
int find_patterns_by_type(const char* pattern_type, ExecutionPattern* patterns, size_t max_patterns);

/**
 * Clear execution patterns
 */
void clear_execution_patterns(void);

// Pattern analysis functions

/**
 * Analyze function call patterns
 */
void analyze_function_call_patterns(void);

/**
 * Analyze memory access patterns
 */
void analyze_memory_access_patterns(void);

/**
 * Analyze branch prediction patterns
 */
void analyze_branch_patterns(void);

/**
 * Analyze performance bottlenecks
 */
void analyze_performance_bottlenecks(void);

/**
 * Identify optimization opportunities
 */
void identify_optimization_opportunities(void);

// Resource monitoring

/**
 * Record resource usage
 * @param usage Resource usage data
 * @return 0 on success, -1 on error
 */
int record_resource_usage(const ResourceUsage* usage);

/**
 * Get resource usage history
 * @param usage_history Array to store usage history
 * @param max_entries Maximum number of entries to return
 * @param start_index Starting index in history buffer
 * @return Number of entries returned
 */
int get_resource_usage_history(ResourceUsage* usage_history, size_t max_entries, size_t start_index);

/**
 * Get current resource usage
 * @param usage Pointer to store current usage
 * @return 0 on success, -1 on error
 */
int get_current_resource_usage(ResourceUsage* usage);

/**
 * Check resource pressure
 * @return true if under resource pressure, false otherwise
 */
bool check_resource_pressure(void);

// Helper functions

/**
 * Get current time in nanoseconds
 * @return Current time in nanoseconds
 */
uint64_t get_current_time_ns(void);

/**
 * Get current thread ID
 * @return Current thread ID
 */
uint64_t get_current_thread_id(void);

/**
 * Get current stack pointer
 * @return Current stack pointer
 */
void* get_current_stack_pointer(void);

/**
 * Get CPU utilization
 * @return CPU utilization percentage
 */
double get_cpu_utilization(void);

/**
 * Calculate memory fragmentation
 * @return Memory fragmentation in bytes
 */
size_t calculate_memory_fragmentation(void);

/**
 * Get disk read count
 * @return Number of disk reads
 */
uint64_t get_disk_read_count(void);

/**
 * Get disk write count
 * @return Number of disk writes
 */
uint64_t get_disk_write_count(void);

/**
 * Get network packets sent
 * @return Number of network packets sent
 */
uint64_t get_network_packets_sent(void);

/**
 * Get network packets received
 * @return Number of network packets received
 */
uint64_t get_network_packets_received(void);

// Statistics and information

/**
 * Get observability statistics
 * @param trace_events Pointer to store trace event count
 * @param snapshots Pointer to store snapshot count
 * @param patterns Pointer to store pattern count
 * @param optimizations Pointer to store optimization suggestion count
 */
void get_observability_stats(uint64_t* trace_events, uint64_t* snapshots, uint64_t* patterns, uint64_t* optimizations);

/**
 * Get trace buffer status
 * @param buffer_size Pointer to store buffer size
 * @param current_count Pointer to store current event count
 * @param head_index Pointer to store head index
 * @return 0 on success, -1 on error
 */
int get_trace_buffer_status(size_t* buffer_size, size_t* current_count, size_t* head_index);

/**
 * Get performance snapshot buffer status
 * @param buffer_size Pointer to store buffer size
 * @param current_count Pointer to store current snapshot count
 * @param head_index Pointer to store head index
 * @return 0 on success, -1 on error
 */
int get_snapshot_buffer_status(size_t* buffer_size, size_t* current_count, size_t* head_index);

// Utility functions

/**
 * Get trace event type string
 * @param type Trace event type
 * @return String representation
 */
const char* get_trace_event_type_string(TraceEventType type);

/**
 * Format trace event to string
 * @param event Trace event
 * @param buffer Buffer to store formatted string
 * @param buffer_size Size of buffer
 * @return 0 on success, -1 on error
 */
int format_trace_event(const TraceEvent* event, char* buffer, size_t buffer_size);

/**
 * Format performance snapshot to string
 * @param snapshot Performance snapshot
 * @param buffer Buffer to store formatted string
 * @param buffer_size Size of buffer
 * @return 0 on success, -1 on error
 */
int format_performance_snapshot(const PerformanceSnapshot* snapshot, char* buffer, size_t buffer_size);

/**
 * Export trace data to file
 * @param filename File to export to
 * @param format Export format ("json", "csv", "binary")
 * @return 0 on success, -1 on error
 */
int export_trace_data(const char* filename, const char* format);

/**
 * Export performance data to file
 * @param filename File to export to
 * @param format Export format ("json", "csv", "binary")
 * @return 0 on success, -1 on error
 */
int export_performance_data(const char* filename, const char* format);

// Error codes
#define OBSERVABILITY_SUCCESS           0
#define OBSERVABILITY_ERROR_INVALID     -1
#define OBSERVABILITY_ERROR_NOT_INIT    -2
#define OBSERVABILITY_ERROR_BUFFER_FULL -3
#define OBSERVABILITY_ERROR_NO_DATA     -4

#ifdef __cplusplus
}
#endif

#endif // OBSERVABILITY_SYSTEM_H
