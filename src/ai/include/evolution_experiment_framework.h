/**
 * evolution_experiment_framework.h - Evolution Experiment Framework
 * 
 * Header for complete framework for hypothesis generation, A/B testing,
 * automatic validation, and production deployment
 */

#ifndef EVOLUTION_EXPERIMENT_FRAMEWORK_H
#define EVOLUTION_EXPERIMENT_FRAMEWORK_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Experiment types
typedef enum {
    EXPERIMENT_TYPE_PERFORMANCE = 1,
    EXPERIMENT_TYPE_MEMORY = 2,
    EXPERIMENT_TYPE_ALGORITHM = 3,
    EXPERIMENT_TYPE_ARCHITECTURE = 4,
    EXPERIMENT_TYPE_OPTIMIZATION = 5,
    EXPERIMENT_TYPE_FEATURE = 6
} ExperimentType;

// Experiment status
typedef enum {
    EXPERIMENT_STATUS_CREATED = 0,
    EXPERIMENT_STATUS_HYPOTHESIS_GENERATED = 1,
    EXPERIMENT_STATUS_DESIGN_COMPLETE = 2,
    EXPERIMENT_STATUS_RUNNING = 3,
    EXPERIMENT_STATUS_ANALYZING = 4,
    EXPERIMENT_STATUS_COMPLETE = 5,
    EXPERIMENT_STATUS_FAILED = 6,
    EXPERIMENT_STATUS_CANCELLED = 7
} ExperimentStatus;

// Hypothesis structure
typedef struct {
    char hypothesis_id[64];
    char description[512];
    ExperimentType type;
    double expected_improvement;
    double confidence_level;
    char rationale[1024];
    
    // Metrics to measure
    char primary_metric[128];
    char secondary_metrics[8][128];
    int secondary_metric_count;
    
    // Success criteria
    double minimum_improvement_threshold;
    double statistical_significance_threshold;
    int minimum_sample_size;
} ExperimentHypothesis;

// A/B test configuration
typedef struct {
    char test_id[64];
    char control_version[64];
    char treatment_version[64];
    double traffic_split; // 0.0 to 1.0 (percentage for treatment)
    
    // Test parameters
    int duration_hours;
    int minimum_samples_per_group;
    double significance_level; // e.g., 0.05 for 95% confidence
    double power; // e.g., 0.8 for 80% power
    
    // Monitoring
    bool enable_real_time_monitoring;
    int monitoring_interval_minutes;
    double early_stopping_threshold;
} ABTestConfig;

// Experiment results
typedef struct {
    char experiment_id[64];
    ExperimentStatus status;
    time_t start_time;
    time_t end_time;
    
    // Statistical results
    double control_mean;
    double treatment_mean;
    double effect_size;
    double p_value;
    double confidence_interval_lower;
    double confidence_interval_upper;
    bool is_statistically_significant;
    
    // Sample sizes
    int control_samples;
    int treatment_samples;
    
    // Metrics
    double primary_metric_improvement;
    double secondary_metric_improvements[8];
    
    // Decision
    bool recommend_deployment;
    char decision_rationale[512];
} ExperimentResults;

// Experiment summary
typedef struct {
    char experiment_id[64];
    ExperimentType type;
    ExperimentStatus status;
    time_t created_time;
    time_t start_time;
    time_t end_time;
    double expected_improvement;
    double actual_improvement;
    bool is_successful;
    bool is_deployed;
} ExperimentSummary;

// Framework configuration
typedef struct {
    bool auto_hypothesis_generation;
    int hypothesis_generation_interval_hours;
    double default_significance_level;
    double default_power;
    int default_minimum_samples;
    int max_concurrent_experiments;
    bool enable_early_stopping;
    double early_stopping_threshold;
} ExperimentFrameworkConfig;

// Core framework functions

/**
 * Initialize evolution experiment framework
 * @return 0 on success, -1 on error
 */
int evolution_experiment_framework_init(void);

/**
 * Cleanup evolution experiment framework
 */
void evolution_experiment_framework_cleanup(void);

/**
 * Configure experiment framework
 * @param config Framework configuration
 * @return 0 on success, -1 on error
 */
int configure_experiment_framework(const ExperimentFrameworkConfig* config);

/**
 * Get framework configuration
 * @param config Pointer to store configuration
 */
void get_experiment_framework_config(ExperimentFrameworkConfig* config);

// Hypothesis generation

/**
 * Generate hypothesis from observability data
 * @param hypothesis Pointer to store generated hypothesis
 * @return 0 on success, -1 on error
 */
int generate_hypothesis_from_data(ExperimentHypothesis* hypothesis);

/**
 * Generate hypothesis manually
 * @param type Experiment type
 * @param description Hypothesis description
 * @param expected_improvement Expected improvement (0.0 to 1.0)
 * @param hypothesis Pointer to store generated hypothesis
 * @return 0 on success, -1 on error
 */
int generate_manual_hypothesis(ExperimentType type, const char* description, 
                              double expected_improvement, ExperimentHypothesis* hypothesis);

/**
 * Validate hypothesis
 * @param hypothesis Hypothesis to validate
 * @return true if valid, false otherwise
 */
bool validate_hypothesis(const ExperimentHypothesis* hypothesis);

/**
 * Auto-generate hypotheses
 * @return Number of hypotheses generated, -1 on error
 */
int auto_generate_hypotheses(void);

// A/B test design and execution

/**
 * Design A/B test for hypothesis
 * @param hypothesis Hypothesis to test
 * @param ab_test Pointer to store A/B test configuration
 * @return 0 on success, -1 on error
 */
int design_ab_test(const ExperimentHypothesis* hypothesis, ABTestConfig* ab_test);

/**
 * Create new experiment
 * @param hypothesis Experiment hypothesis
 * @param experiment_id Experiment identifier
 * @return 0 on success, -1 on error
 */
int create_experiment(const ExperimentHypothesis* hypothesis, const char* experiment_id);

/**
 * Run A/B test experiment
 * @param experiment_id Experiment identifier
 * @return 0 on success, -1 on error
 */
int run_ab_test_experiment(const char* experiment_id);

/**
 * Stop experiment
 * @param experiment_id Experiment identifier
 * @param reason Reason for stopping
 * @return 0 on success, -1 on error
 */
int stop_experiment(const char* experiment_id, const char* reason);

/**
 * Cancel experiment
 * @param experiment_id Experiment identifier
 * @return 0 on success, -1 on error
 */
int cancel_experiment(const char* experiment_id);

// Results analysis

/**
 * Analyze experiment results
 * @param experiment_id Experiment identifier
 * @return 0 on success, -1 on error
 */
int analyze_experiment_results(const char* experiment_id);

/**
 * Get experiment results
 * @param experiment_id Experiment identifier
 * @param results Pointer to store results
 * @return 0 on success, -1 on error
 */
int get_experiment_results(const char* experiment_id, ExperimentResults* results);

/**
 * Calculate statistical significance
 * @param control_mean Control group mean
 * @param treatment_mean Treatment group mean
 * @param control_variance Control group variance
 * @param treatment_variance Treatment group variance
 * @param control_samples Control group sample size
 * @param treatment_samples Treatment group sample size
 * @return p-value
 */
double calculate_statistical_significance(double control_mean, double treatment_mean,
                                        double control_variance, double treatment_variance,
                                        int control_samples, int treatment_samples);

/**
 * Calculate effect size
 * @param control_mean Control group mean
 * @param treatment_mean Treatment group mean
 * @param pooled_std_dev Pooled standard deviation
 * @return Effect size (Cohen's d)
 */
double calculate_effect_size(double control_mean, double treatment_mean, double pooled_std_dev);

// Monitoring and validation

/**
 * Collect experiment metrics
 * @param experiment_id Experiment identifier
 * @return 0 on success, -1 on error
 */
int collect_experiment_metrics(const char* experiment_id);

/**
 * Check early stopping conditions
 * @param experiment_id Experiment identifier
 * @return true if should stop early, false otherwise
 */
bool check_early_stopping_conditions(const char* experiment_id);

/**
 * Monitor experiment progress
 * @param experiment_id Experiment identifier
 * @return 0 on success, -1 on error
 */
int monitor_experiment_progress(const char* experiment_id);

/**
 * Validate experiment integrity
 * @param experiment_id Experiment identifier
 * @return true if valid, false otherwise
 */
bool validate_experiment_integrity(const char* experiment_id);

// Deployment and rollout

/**
 * Deploy successful experiment
 * @param experiment_id Experiment identifier
 * @return 0 on success, -1 on error
 */
int deploy_successful_experiment(const char* experiment_id);

/**
 * Rollback experiment deployment
 * @param experiment_id Experiment identifier
 * @return 0 on success, -1 on error
 */
int rollback_experiment_deployment(const char* experiment_id);

/**
 * Schedule experiment deployment
 * @param experiment_id Experiment identifier
 * @param deployment_time Scheduled deployment time
 * @return 0 on success, -1 on error
 */
int schedule_experiment_deployment(const char* experiment_id, time_t deployment_time);

// Information and management

/**
 * List active experiments
 * @param experiments Array to store experiment summaries
 * @param max_experiments Maximum number of experiments to return
 * @return Number of active experiments
 */
int list_active_experiments(ExperimentSummary* experiments, int max_experiments);

/**
 * List completed experiments
 * @param experiments Array to store experiment summaries
 * @param max_experiments Maximum number of experiments to return
 * @return Number of completed experiments
 */
int list_completed_experiments(ExperimentSummary* experiments, int max_experiments);

/**
 * Get experiment by ID
 * @param experiment_id Experiment identifier
 * @param hypothesis Pointer to store hypothesis
 * @param ab_test Pointer to store A/B test config
 * @param results Pointer to store results
 * @return 0 on success, -1 on error
 */
int get_experiment_by_id(const char* experiment_id, ExperimentHypothesis* hypothesis,
                        ABTestConfig* ab_test, ExperimentResults* results);

/**
 * Get experiment count
 * @return Number of experiments
 */
int get_experiment_count(void);

/**
 * Get active experiment count
 * @return Number of active experiments
 */
int get_active_experiment_count(void);

// Statistics and reporting

/**
 * Get experiment framework statistics
 * @param total_experiments Pointer to store total experiment count
 * @param successful_experiments Pointer to store successful experiment count
 * @param failed_experiments Pointer to store failed experiment count
 * @param deployed_improvements Pointer to store deployed improvement count
 */
void get_experiment_framework_stats(uint64_t* total_experiments, uint64_t* successful_experiments,
                                   uint64_t* failed_experiments, uint64_t* deployed_improvements);

/**
 * Generate experiment report
 * @param experiment_id Experiment identifier
 * @param report_buffer Buffer to store report
 * @param buffer_size Size of report buffer
 * @return 0 on success, -1 on error
 */
int generate_experiment_report(const char* experiment_id, char* report_buffer, size_t buffer_size);

/**
 * Export experiment data
 * @param filename File to export to
 * @param format Export format ("json", "csv")
 * @return 0 on success, -1 on error
 */
int export_experiment_data(const char* filename, const char* format);

// Utility functions

/**
 * Get experiment type string
 * @param type Experiment type
 * @return String representation
 */
const char* get_experiment_type_string(ExperimentType type);

/**
 * Get experiment status string
 * @param status Experiment status
 * @return String representation
 */
const char* get_experiment_status_string(ExperimentStatus status);

/**
 * Calculate required sample size
 * @param effect_size Expected effect size
 * @param power Statistical power (e.g., 0.8)
 * @param significance_level Significance level (e.g., 0.05)
 * @return Required sample size per group
 */
int calculate_required_sample_size(double effect_size, double power, double significance_level);

/**
 * Estimate experiment duration
 * @param required_samples Required sample size
 * @param traffic_rate Expected traffic rate (samples per hour)
 * @param traffic_split Treatment traffic split
 * @return Estimated duration in hours
 */
int estimate_experiment_duration(int required_samples, double traffic_rate, double traffic_split);

// Error codes
#define EXPERIMENT_SUCCESS           0
#define EXPERIMENT_ERROR_INVALID     -1
#define EXPERIMENT_ERROR_NOT_FOUND   -2
#define EXPERIMENT_ERROR_RUNNING     -3
#define EXPERIMENT_ERROR_FAILED      -4
#define EXPERIMENT_ERROR_CANCELLED   -5

#ifdef __cplusplus
}
#endif

#endif // EVOLUTION_EXPERIMENT_FRAMEWORK_H
