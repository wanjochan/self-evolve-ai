/**
 * security_evolution.h - Security Evolution Mechanisms
 * 
 * Header for comprehensive security system for AI evolution
 */

#ifndef SECURITY_EVOLUTION_H
#define SECURITY_EVOLUTION_H

#include "evolution_engine.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Security levels
typedef enum {
    SECURITY_LEVEL_MINIMAL = 0,
    SECURITY_LEVEL_BASIC = 1,
    SECURITY_LEVEL_STANDARD = 2,
    SECURITY_LEVEL_HIGH = 3,
    SECURITY_LEVEL_MAXIMUM = 4
} SecurityLevel;

// Deployment phases
typedef enum {
    DEPLOY_PHASE_SANDBOX = 0,
    DEPLOY_PHASE_CANARY = 1,
    DEPLOY_PHASE_GRADUAL = 2,
    DEPLOY_PHASE_FULL = 3,
    DEPLOY_PHASE_ROLLBACK = 4
} DeploymentPhase;

// Security violation types
typedef enum {
    VIOLATION_MEMORY_ACCESS = 1,
    VIOLATION_RESOURCE_LIMIT = 2,
    VIOLATION_UNAUTHORIZED_SYSCALL = 3,
    VIOLATION_PERFORMANCE_DEGRADATION = 4,
    VIOLATION_INFINITE_LOOP = 5,
    VIOLATION_STACK_OVERFLOW = 6,
    VIOLATION_HEAP_CORRUPTION = 7
} SecurityViolationType;

// Sandbox configuration
typedef struct {
    size_t max_memory_usage;
    uint64_t max_execution_time_ns;
    uint64_t max_instruction_count;
    int max_file_descriptors;
    int max_network_connections;
    bool allow_system_calls;
    bool allow_file_access;
    bool allow_network_access;
    char allowed_paths[16][256];
    int allowed_path_count;
} SandboxConfig;

// Performance monitoring metrics
typedef struct {
    uint64_t execution_time_ns;
    size_t memory_usage_bytes;
    size_t peak_memory_usage;
    uint64_t instruction_count;
    uint64_t system_call_count;
    uint64_t cache_misses;
    double cpu_utilization;
    int error_count;
    int warning_count;
} PerformanceMonitoringMetrics;

// Deployment state
typedef struct {
    char evolution_id[64];
    DeploymentPhase current_phase;
    time_t deployment_start_time;
    time_t phase_start_time;
    double success_rate;
    int total_tests;
    int passed_tests;
    int failed_tests;
    bool is_rollback_ready;
    char rollback_version[64];
} DeploymentState;

// Security violation record
typedef struct {
    SecurityViolationType type;
    time_t timestamp;
    char description[256];
    bool is_critical;
    char evolution_id[64];
    char context[128];
} SecurityViolation;

// Rollback point
typedef struct {
    char version_id[64];
    time_t backup_time;
    char backup_path[256];
    bool is_valid;
    size_t backup_size;
    char checksum[64];
} RollbackPoint;

// Core security functions

/**
 * Initialize security evolution system
 * @return 0 on success, -1 on error
 */
int security_evolution_init(void);

/**
 * Cleanup security evolution system
 */
void security_evolution_cleanup(void);

/**
 * Set security level
 * @param level Security level to set
 * @return 0 on success, -1 on error
 */
int set_security_level(SecurityLevel level);

/**
 * Get current security level
 * @return Current security level
 */
SecurityLevel get_security_level(void);

// Sandbox operations

/**
 * Configure default sandbox
 * @param config Sandbox configuration to populate
 */
void configure_default_sandbox(SandboxConfig* config);

/**
 * Create secure sandbox environment
 * @param evolution_id Evolution identifier
 * @return 0 on success, -1 on error
 */
int create_sandbox_environment(const char* evolution_id);

/**
 * Execute evolution in sandbox
 * @param evolution_id Evolution identifier
 * @param candidate Evolution candidate to test
 * @return 0 on success, -1 on error
 */
int execute_evolution_in_sandbox(const char* evolution_id, const EvolutionCandidate* candidate);

/**
 * Cleanup sandbox environment
 */
void cleanup_sandbox_environment(void);

/**
 * Configure sandbox limits
 * @param config Sandbox configuration
 * @return 0 on success, -1 on error
 */
int configure_sandbox_limits(const SandboxConfig* config);

// Gradual deployment

/**
 * Deploy evolution gradually
 * @param evolution_id Evolution identifier
 * @param candidate Evolution candidate to deploy
 * @return 0 on success, -1 on error
 */
int deploy_evolution_gradually(const char* evolution_id, const EvolutionCandidate* candidate);

/**
 * Deploy canary release
 * @param evolution_id Evolution identifier
 * @param candidate Evolution candidate
 * @param percentage Percentage of traffic (0.0 to 1.0)
 * @return 0 on success, -1 on error
 */
int deploy_canary_release(const char* evolution_id, const EvolutionCandidate* candidate, double percentage);

/**
 * Deploy gradual release
 * @param evolution_id Evolution identifier
 * @param candidate Evolution candidate
 * @param percentage Percentage of traffic (0.0 to 1.0)
 * @return 0 on success, -1 on error
 */
int deploy_gradual_release(const char* evolution_id, const EvolutionCandidate* candidate, double percentage);

/**
 * Get deployment state
 * @param evolution_id Evolution identifier
 * @param state Pointer to store deployment state
 * @return 0 on success, -1 on error
 */
int get_deployment_state(const char* evolution_id, DeploymentState* state);

// Rollback system

/**
 * Create rollback point
 * @param evolution_id Evolution identifier
 * @return 0 on success, -1 on error
 */
int create_rollback_point(const char* evolution_id);

/**
 * Perform rollback
 * @param evolution_id Evolution identifier
 * @return 0 on success, -1 on error
 */
int perform_rollback(const char* evolution_id);

/**
 * List available rollback points
 * @param rollback_points Array to store rollback points
 * @param max_points Maximum number of points to return
 * @return Number of rollback points found
 */
int list_rollback_points(RollbackPoint* rollback_points, int max_points);

/**
 * Validate rollback point
 * @param version_id Version identifier
 * @return true if valid, false otherwise
 */
bool validate_rollback_point(const char* version_id);

/**
 * Remove rollback point
 * @param version_id Version identifier
 * @return 0 on success, -1 on error
 */
int remove_rollback_point(const char* version_id);

// Performance monitoring

/**
 * Monitor deployment performance
 * @param evolution_id Evolution identifier
 * @param duration_seconds Monitoring duration in seconds
 * @return 0 on success, -1 on error
 */
int monitor_deployment_performance(const char* evolution_id, int duration_seconds);

/**
 * Monitor canary deployment
 * @param evolution_id Evolution identifier
 * @param duration_seconds Monitoring duration in seconds
 * @return 0 on success, -1 on error
 */
int monitor_canary_deployment(const char* evolution_id, int duration_seconds);

/**
 * Monitor gradual deployment
 * @param evolution_id Evolution identifier
 * @param duration_seconds Monitoring duration in seconds
 * @return 0 on success, -1 on error
 */
int monitor_gradual_deployment(const char* evolution_id, int duration_seconds);

/**
 * Capture performance metrics
 * @param metrics Pointer to store metrics
 */
void capture_performance_metrics(PerformanceMonitoringMetrics* metrics);

/**
 * Capture baseline metrics
 */
void capture_baseline_metrics(void);

/**
 * Detect performance degradation
 * @return 0 if no degradation, -1 if degradation detected
 */
int detect_performance_degradation(void);

// Security violation management

/**
 * Record security violation
 * @param type Violation type
 * @param description Violation description
 * @param is_critical Whether violation is critical
 * @return 0 on success, -1 on error
 */
int record_security_violation(SecurityViolationType type, const char* description, bool is_critical);

/**
 * Check security violations
 * @return 0 if no violations, -1 if violations detected
 */
int check_security_violations(void);

/**
 * Analyze security violations
 * @return Number of violations found
 */
int analyze_security_violations(void);

/**
 * Get security violations
 * @param violations Array to store violations
 * @param max_violations Maximum number of violations to return
 * @return Number of violations found
 */
int get_security_violations(SecurityViolation* violations, int max_violations);

/**
 * Clear security violations
 * @return 0 on success, -1 on error
 */
int clear_security_violations(void);

// Helper functions

/**
 * Enable security monitoring
 * @param instance_name VM instance name
 */
void enable_security_monitoring(const char* instance_name);

/**
 * Install security hooks
 * @param instance_name VM instance name
 */
void install_security_hooks(const char* instance_name);

/**
 * Load candidate in sandbox
 * @param candidate Evolution candidate
 * @return 0 on success, -1 on error
 */
int load_candidate_in_sandbox(const EvolutionCandidate* candidate);

/**
 * Run candidate in sandbox
 * @param candidate Evolution candidate
 * @return 0 on success, -1 on error
 */
int run_candidate_in_sandbox(const EvolutionCandidate* candidate);

// Statistics and information

/**
 * Get security evolution statistics
 * @param sandbox_executions Pointer to store sandbox execution count
 * @param violations Pointer to store violation count
 * @param deployments Pointer to store successful deployment count
 * @param rollbacks Pointer to store rollback count
 */
void get_security_evolution_stats(uint64_t* sandbox_executions, uint64_t* violations, 
                                 uint64_t* deployments, uint64_t* rollbacks);

/**
 * Get security level string
 * @param level Security level
 * @return String representation
 */
const char* get_security_level_string(SecurityLevel level);

/**
 * Get deployment phase string
 * @param phase Deployment phase
 * @return String representation
 */
const char* get_deployment_phase_string(DeploymentPhase phase);

/**
 * Get violation type string
 * @param type Violation type
 * @return String representation
 */
const char* get_violation_type_string(SecurityViolationType type);

// Error codes
#define SECURITY_SUCCESS           0
#define SECURITY_ERROR_INVALID     -1
#define SECURITY_ERROR_VIOLATION   -2
#define SECURITY_ERROR_ROLLBACK    -3
#define SECURITY_ERROR_SANDBOX     -4
#define SECURITY_ERROR_DEPLOYMENT  -5

#ifdef __cplusplus
}
#endif

#endif // SECURITY_EVOLUTION_H
