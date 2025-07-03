/**
 * module_standardization.h - Module Standardization System
 * 
 * Header for comprehensive module standardization including metadata,
 * version management, and signature verification
 */

#ifndef MODULE_STANDARDIZATION_H
#define MODULE_STANDARDIZATION_H

#include "module_dependency_manager.h"
#include "astc_platform_compat.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Module standard version
#define MODULE_STANDARD_VERSION_MAJOR 1
#define MODULE_STANDARD_VERSION_MINOR 0
#define MODULE_STANDARD_VERSION_PATCH 0

// Module metadata structure
typedef struct {
    // Basic information
    char module_name[128];
    char module_id[64];
    ModuleVersion version;
    char author[128];
    char description[256];
    char license[64];
    
    // Build information
    uint32_t build_timestamp;
    char build_id[64];
    char compiler_version[64];
    char build_flags[256];
    
    // Compatibility information
    char abi_version[32];
    char api_version[32];
    uint32_t compatibility_flags;
    
    // Platform requirements
    ASTCPlatformType supported_platforms[8];
    int supported_platform_count;
    ASTCArchitectureType supported_architectures[8];
    int supported_arch_count;
    
    // Dependencies
    DependencySpec dependencies[32];
    int dependency_count;
    
    // Security information
    uint8_t signature[256];
    uint32_t signature_length;
    char certificate_id[64];
    bool is_signed;
    bool is_verified;
    
    // Quality metrics
    int test_coverage_percentage;
    int code_quality_score;
    bool has_documentation;
    bool has_examples;
    
    // Usage statistics
    uint64_t download_count;
    uint64_t usage_count;
    double average_rating;
    
    // Standard compliance
    bool complies_with_standard;
    char compliance_version[32];
    char compliance_notes[256];
} StandardModuleMetadata;

// Module signature information
typedef struct {
    char signer_name[128];
    char signer_email[128];
    char certificate_authority[128];
    time_t signature_timestamp;
    time_t certificate_expiry;
    uint32_t signature_algorithm;
    bool is_self_signed;
    bool is_trusted;
} ModuleSignatureInfo;

// Standardization configuration
typedef struct {
    bool enforce_signatures;
    bool require_metadata;
    bool check_compliance;
    int minimum_quality_score;
    int minimum_test_coverage;
    bool require_documentation;
} StandardizationConfig;

// Standardization statistics
typedef struct {
    uint64_t modules_verified;
    uint64_t signature_checks;
    uint64_t compliance_checks;
    uint64_t failed_verifications;
    int registered_modules;
    int trusted_signers;
} StandardizationStats;

// Core standardization functions

/**
 * Initialize module standardization system
 * @return 0 on success, -1 on error
 */
int module_standardization_init(void);

/**
 * Cleanup module standardization system
 */
void module_standardization_cleanup(void);

/**
 * Register module with standardization system
 * @param module_path Path to module file
 * @return 0 on success, -1 on error
 */
int register_standard_module(const char* module_path);

/**
 * Unregister module from standardization system
 * @param module_name Name of module to unregister
 * @return 0 on success, -1 on error
 */
int unregister_standard_module(const char* module_name);

// Metadata management

/**
 * Extract metadata from module file
 * @param module_path Path to module file
 * @param metadata Pointer to store extracted metadata
 * @return 0 on success, -1 on error
 */
int extract_module_metadata(const char* module_path, StandardModuleMetadata* metadata);

/**
 * Create module metadata
 * @param module_name Name of module
 * @param version_str Version string
 * @param author Author name
 * @param description Module description
 * @param metadata Pointer to store created metadata
 * @return 0 on success, -1 on error
 */
int create_module_metadata(const char* module_name, const char* version_str, const char* author,
                          const char* description, StandardModuleMetadata* metadata);

/**
 * Update module metadata
 * @param module_name Name of module
 * @param metadata New metadata
 * @return 0 on success, -1 on error
 */
int update_module_metadata(const char* module_name, const StandardModuleMetadata* metadata);

/**
 * Validate module metadata
 * @param metadata Metadata to validate
 * @return 0 if valid, -1 if invalid
 */
int validate_module_metadata(const StandardModuleMetadata* metadata);

// Compliance verification

/**
 * Verify module compliance with standard
 * @param metadata Module metadata to check
 * @return 0 if compliant, -1 if not compliant
 */
int verify_module_compliance(const StandardModuleMetadata* metadata);

/**
 * Check module quality requirements
 * @param metadata Module metadata to check
 * @return 0 if meets requirements, -1 if not
 */
int check_module_quality_requirements(const StandardModuleMetadata* metadata);

/**
 * Verify module platform compatibility
 * @param metadata Module metadata
 * @param target_platform Target platform
 * @param target_arch Target architecture
 * @return true if compatible, false otherwise
 */
bool verify_module_platform_compatibility(const StandardModuleMetadata* metadata,
                                         ASTCPlatformType target_platform,
                                         ASTCArchitectureType target_arch);

// Signature verification

/**
 * Verify module signature
 * @param module_path Path to module file
 * @param metadata Module metadata
 * @return 0 if signature valid, -1 if invalid
 */
int verify_module_signature(const char* module_path, StandardModuleMetadata* metadata);

/**
 * Extract signature information from module
 * @param module_path Path to module file
 * @param sig_info Pointer to store signature information
 * @return 0 on success, -1 on error
 */
int extract_signature_info(const char* module_path, ModuleSignatureInfo* sig_info);

/**
 * Verify cryptographic signature
 * @param module_path Path to module file
 * @param sig_info Signature information
 * @return 0 if valid, -1 if invalid
 */
int verify_signature_cryptographic(const char* module_path, const ModuleSignatureInfo* sig_info);

/**
 * Sign module with certificate
 * @param module_path Path to module file
 * @param certificate_path Path to certificate file
 * @param private_key_path Path to private key file
 * @return 0 on success, -1 on error
 */
int sign_module_with_certificate(const char* module_path, const char* certificate_path, const char* private_key_path);

// Module registry functions

/**
 * Find registered module by name
 * @param module_name Name of module to find
 * @return Pointer to module metadata, NULL if not found
 */
const StandardModuleMetadata* find_registered_module(const char* module_name);

/**
 * Find registered module by ID
 * @param module_id ID of module to find
 * @return Pointer to module metadata, NULL if not found
 */
const StandardModuleMetadata* find_registered_module_by_id(const char* module_id);

/**
 * List all registered modules
 */
void list_registered_standard_modules(void);

/**
 * Get registered module count
 * @return Number of registered modules
 */
int get_registered_module_count(void);

/**
 * Export module registry to file
 * @param filename File to export to
 * @return 0 on success, -1 on error
 */
int export_module_registry(const char* filename);

/**
 * Import module registry from file
 * @param filename File to import from
 * @return 0 on success, -1 on error
 */
int import_module_registry(const char* filename);

// Configuration functions

/**
 * Configure standardization system
 * @param enforce_signatures Enforce signature verification
 * @param require_metadata Require complete metadata
 * @param check_compliance Check standard compliance
 * @param minimum_quality_score Minimum quality score required
 */
void configure_module_standardization(bool enforce_signatures, bool require_metadata, 
                                     bool check_compliance, int minimum_quality_score);

/**
 * Set standardization configuration
 * @param config Configuration structure
 * @return 0 on success, -1 on error
 */
int set_standardization_config(const StandardizationConfig* config);

/**
 * Get standardization configuration
 * @param config Pointer to store configuration
 */
void get_standardization_config(StandardizationConfig* config);

// Trusted signer management

/**
 * Add trusted signer
 * @param signer_name Name of trusted signer
 * @return 0 on success, -1 on error
 */
int add_trusted_signer(const char* signer_name);

/**
 * Remove trusted signer
 * @param signer_name Name of signer to remove
 * @return 0 on success, -1 on error
 */
int remove_trusted_signer(const char* signer_name);

/**
 * Check if signer is trusted
 * @param signer_name Name of signer to check
 * @return true if trusted, false otherwise
 */
bool is_trusted_signer(const char* signer_name);

/**
 * List trusted signers
 * @param signers Array to store signer names
 * @param max_signers Maximum number of signers to return
 * @return Number of trusted signers
 */
int list_trusted_signers(char signers[][128], int max_signers);

// Statistics and monitoring

/**
 * Get standardization statistics
 * @param modules_verified Pointer to store verified module count
 * @param signature_checks Pointer to store signature check count
 * @param compliance_checks Pointer to store compliance check count
 * @param failed_verifications Pointer to store failed verification count
 */
void get_standardization_stats(uint64_t* modules_verified, uint64_t* signature_checks, 
                              uint64_t* compliance_checks, uint64_t* failed_verifications);

/**
 * Get detailed standardization statistics
 * @param stats Pointer to store detailed statistics
 */
void get_detailed_standardization_stats(StandardizationStats* stats);

/**
 * Reset standardization statistics
 */
void reset_standardization_stats(void);

// Utility functions

/**
 * Generate module ID from name and version
 * @param module_name Module name
 * @param version Module version
 * @param module_id Buffer to store generated ID
 * @param id_size Size of ID buffer
 * @return 0 on success, -1 on error
 */
int generate_module_id(const char* module_name, const ModuleVersion* version, char* module_id, size_t id_size);

/**
 * Validate module name
 * @param module_name Module name to validate
 * @return true if valid, false otherwise
 */
bool validate_module_name(const char* module_name);

/**
 * Calculate module quality score
 * @param metadata Module metadata
 * @return Quality score (0-100)
 */
int calculate_module_quality_score(const StandardModuleMetadata* metadata);

/**
 * Check module compatibility
 * @param metadata1 First module metadata
 * @param metadata2 Second module metadata
 * @return true if compatible, false otherwise
 */
bool check_module_compatibility(const StandardModuleMetadata* metadata1, const StandardModuleMetadata* metadata2);

/**
 * Dump module metadata to string
 * @param metadata Module metadata
 * @param buffer Buffer to store string
 * @param buffer_size Size of buffer
 * @return 0 on success, -1 on error
 */
int dump_module_metadata_to_string(const StandardModuleMetadata* metadata, char* buffer, size_t buffer_size);

/**
 * Load module metadata from string
 * @param metadata_string String representation of metadata
 * @param metadata Pointer to store loaded metadata
 * @return 0 on success, -1 on error
 */
int load_module_metadata_from_string(const char* metadata_string, StandardModuleMetadata* metadata);

// Error codes
#define MODULE_STANDARD_SUCCESS           0
#define MODULE_STANDARD_ERROR_INVALID     -1
#define MODULE_STANDARD_ERROR_NOT_FOUND   -2
#define MODULE_STANDARD_ERROR_SIGNATURE   -3
#define MODULE_STANDARD_ERROR_COMPLIANCE  -4
#define MODULE_STANDARD_ERROR_QUALITY     -5
#define MODULE_STANDARD_ERROR_PLATFORM    -6

// Quality score thresholds
#define MODULE_QUALITY_EXCELLENT  90
#define MODULE_QUALITY_GOOD       75
#define MODULE_QUALITY_ACCEPTABLE 60
#define MODULE_QUALITY_POOR       40

// Signature algorithms
#define SIGNATURE_ALGORITHM_RSA_SHA256    1
#define SIGNATURE_ALGORITHM_ECDSA_SHA256  2
#define SIGNATURE_ALGORITHM_ED25519       3

#ifdef __cplusplus
}
#endif

#endif // MODULE_STANDARDIZATION_H
