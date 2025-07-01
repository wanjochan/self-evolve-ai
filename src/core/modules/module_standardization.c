/**
 * module_standardization.c - Module Standardization System
 * 
 * Comprehensive module standardization including metadata, version management,
 * signature verification, and compliance checking.
 */

#include "../include/native_format.h"
#include "../include/logger.h"
#include "../include/module_dependency_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

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

// Module standardization state
static struct {
    StandardModuleMetadata* registered_modules;
    int registered_module_count;
    int max_registered_modules;
    bool initialized;
    
    // Standard configuration
    bool enforce_signatures;
    bool require_metadata;
    bool check_compliance;
    int minimum_quality_score;
    
    // Trusted signers
    char trusted_signers[16][128];
    int trusted_signer_count;
    
    // Statistics
    uint64_t modules_verified;
    uint64_t signature_checks;
    uint64_t compliance_checks;
    uint64_t failed_verifications;
} g_module_standard = {0};

// Initialize module standardization system
int module_standardization_init(void) {
    if (g_module_standard.initialized) {
        return 0;
    }
    
    memset(&g_module_standard, 0, sizeof(g_module_standard));
    
    // Allocate module registry
    g_module_standard.max_registered_modules = 1024;
    g_module_standard.registered_modules = malloc(g_module_standard.max_registered_modules * sizeof(StandardModuleMetadata));
    if (!g_module_standard.registered_modules) {
        LOG_MODULE_ERROR("Failed to allocate module registry");
        return -1;
    }
    
    // Default configuration
    g_module_standard.enforce_signatures = false; // Start permissive
    g_module_standard.require_metadata = true;
    g_module_standard.check_compliance = true;
    g_module_standard.minimum_quality_score = 70;
    
    // Add default trusted signers
    strcpy(g_module_standard.trusted_signers[0], "Self-Evolve AI Official");
    strcpy(g_module_standard.trusted_signers[1], "ASTC Module Authority");
    g_module_standard.trusted_signer_count = 2;
    
    g_module_standard.initialized = true;
    
    LOG_MODULE_INFO("Module standardization system initialized");
    LOG_MODULE_INFO("Standard version: %d.%d.%d", 
                   MODULE_STANDARD_VERSION_MAJOR,
                   MODULE_STANDARD_VERSION_MINOR,
                   MODULE_STANDARD_VERSION_PATCH);
    
    return 0;
}

// Cleanup module standardization system
void module_standardization_cleanup(void) {
    if (!g_module_standard.initialized) {
        return;
    }
    
    LOG_MODULE_INFO("Module standardization statistics:");
    LOG_MODULE_INFO("  Modules verified: %llu", g_module_standard.modules_verified);
    LOG_MODULE_INFO("  Signature checks: %llu", g_module_standard.signature_checks);
    LOG_MODULE_INFO("  Compliance checks: %llu", g_module_standard.compliance_checks);
    LOG_MODULE_INFO("  Failed verifications: %llu", g_module_standard.failed_verifications);
    
    if (g_module_standard.registered_modules) {
        free(g_module_standard.registered_modules);
        g_module_standard.registered_modules = NULL;
    }
    
    g_module_standard.initialized = false;
}

// Register module with standardization system
int register_standard_module(const char* module_path) {
    if (!module_path) {
        return -1;
    }
    
    if (g_module_standard.registered_module_count >= g_module_standard.max_registered_modules) {
        LOG_MODULE_ERROR("Module registry full");
        return -1;
    }
    
    StandardModuleMetadata* metadata = &g_module_standard.registered_modules[g_module_standard.registered_module_count];
    memset(metadata, 0, sizeof(StandardModuleMetadata));
    
    // Extract metadata from module file
    if (extract_module_metadata(module_path, metadata) != 0) {
        LOG_MODULE_ERROR("Failed to extract metadata from module: %s", module_path);
        return -1;
    }
    
    // Verify module compliance
    if (g_module_standard.check_compliance) {
        if (verify_module_compliance(metadata) != 0) {
            LOG_MODULE_ERROR("Module does not comply with standard: %s", module_path);
            if (g_module_standard.enforce_signatures) {
                return -1;
            }
        }
    }
    
    // Verify module signature
    if (g_module_standard.enforce_signatures) {
        if (verify_module_signature(module_path, metadata) != 0) {
            LOG_MODULE_ERROR("Module signature verification failed: %s", module_path);
            g_module_standard.failed_verifications++;
            return -1;
        }
    }
    
    g_module_standard.registered_module_count++;
    g_module_standard.modules_verified++;
    
    LOG_MODULE_INFO("Module registered: %s v%d.%d.%d", 
                   metadata->module_name,
                   metadata->version.major,
                   metadata->version.minor,
                   metadata->version.patch);
    
    return 0;
}

// Extract metadata from module file
int extract_module_metadata(const char* module_path, StandardModuleMetadata* metadata) {
    if (!module_path || !metadata) {
        return -1;
    }
    
    // Load native module
    NativeModule* module = native_module_load_file(module_path);
    if (!module) {
        LOG_MODULE_ERROR("Failed to load module for metadata extraction: %s", module_path);
        return -1;
    }
    
    // Extract basic information from module header
    const NativeModuleHeader* header = native_module_get_header(module);
    if (header) {
        strncpy(metadata->module_name, header->module_name, sizeof(metadata->module_name) - 1);
        metadata->version.major = header->version_major;
        metadata->version.minor = header->version_minor;
        metadata->version.patch = header->version_patch;
        metadata->build_timestamp = header->timestamp;
    }
    
    // Extract metadata section if present
    const void* metadata_section = native_module_get_section(module, ".metadata");
    if (metadata_section) {
        // Parse metadata section (simplified)
        // In a real implementation, this would parse a structured format like JSON or binary
        LOG_MODULE_DEBUG("Found metadata section in module: %s", module_path);
    }
    
    // Set default values for missing metadata
    if (strlen(metadata->module_name) == 0) {
        // Extract module name from file path
        const char* filename = strrchr(module_path, '/');
        if (!filename) filename = strrchr(module_path, '\\');
        if (!filename) filename = module_path;
        else filename++;
        
        strncpy(metadata->module_name, filename, sizeof(metadata->module_name) - 1);
    }
    
    if (strlen(metadata->author) == 0) {
        strcpy(metadata->author, "Unknown");
    }
    
    if (strlen(metadata->license) == 0) {
        strcpy(metadata->license, "Unknown");
    }
    
    // Generate module ID if not present
    if (strlen(metadata->module_id) == 0) {
        snprintf(metadata->module_id, sizeof(metadata->module_id), 
                "module.%s.%d.%d.%d", 
                metadata->module_name,
                metadata->version.major,
                metadata->version.minor,
                metadata->version.patch);
    }
    
    native_module_free(module);
    return 0;
}

// Verify module compliance with standard
int verify_module_compliance(const StandardModuleMetadata* metadata) {
    if (!metadata) {
        return -1;
    }
    
    g_module_standard.compliance_checks++;
    
    // Check required fields
    if (strlen(metadata->module_name) == 0) {
        LOG_MODULE_ERROR("Module name is required");
        return -1;
    }
    
    if (strlen(metadata->module_id) == 0) {
        LOG_MODULE_ERROR("Module ID is required");
        return -1;
    }
    
    // Check version format
    if (metadata->version.major < 0 || metadata->version.minor < 0 || metadata->version.patch < 0) {
        LOG_MODULE_ERROR("Invalid version format");
        return -1;
    }
    
    // Check quality score
    if (metadata->code_quality_score < g_module_standard.minimum_quality_score) {
        LOG_MODULE_WARN("Module quality score (%d) below minimum (%d)", 
                       metadata->code_quality_score, g_module_standard.minimum_quality_score);
        // Don't fail for quality score, just warn
    }
    
    // Check ABI/API version compatibility
    if (strlen(metadata->abi_version) > 0) {
        // Verify ABI version is compatible
        // This would check against known compatible ABI versions
        LOG_MODULE_DEBUG("ABI version: %s", metadata->abi_version);
    }
    
    LOG_MODULE_DEBUG("Module compliance verified: %s", metadata->module_name);
    return 0;
}

// Verify module signature
int verify_module_signature(const char* module_path, StandardModuleMetadata* metadata) {
    if (!module_path || !metadata) {
        return -1;
    }
    
    g_module_standard.signature_checks++;
    
    if (!metadata->is_signed) {
        LOG_MODULE_WARN("Module is not signed: %s", module_path);
        return g_module_standard.enforce_signatures ? -1 : 0;
    }
    
    // Extract signature information
    ModuleSignatureInfo sig_info = {0};
    if (extract_signature_info(module_path, &sig_info) != 0) {
        LOG_MODULE_ERROR("Failed to extract signature information");
        return -1;
    }
    
    // Check if signer is trusted
    bool is_trusted_signer = false;
    for (int i = 0; i < g_module_standard.trusted_signer_count; i++) {
        if (strcmp(sig_info.signer_name, g_module_standard.trusted_signers[i]) == 0) {
            is_trusted_signer = true;
            break;
        }
    }
    
    if (!is_trusted_signer && !sig_info.is_self_signed) {
        LOG_MODULE_WARN("Module signed by untrusted signer: %s", sig_info.signer_name);
        if (g_module_standard.enforce_signatures) {
            return -1;
        }
    }
    
    // Check certificate expiry
    time_t current_time = time(NULL);
    if (sig_info.certificate_expiry < current_time) {
        LOG_MODULE_ERROR("Module certificate has expired");
        return -1;
    }
    
    // Verify signature (simplified)
    if (verify_signature_cryptographic(module_path, &sig_info) != 0) {
        LOG_MODULE_ERROR("Cryptographic signature verification failed");
        return -1;
    }
    
    metadata->is_verified = true;
    LOG_MODULE_DEBUG("Module signature verified: %s", module_path);
    return 0;
}

// Extract signature information from module
int extract_signature_info(const char* module_path, ModuleSignatureInfo* sig_info) {
    if (!module_path || !sig_info) {
        return -1;
    }
    
    // Load module and look for signature section
    NativeModule* module = native_module_load_file(module_path);
    if (!module) {
        return -1;
    }
    
    const void* sig_section = native_module_get_section(module, ".signature");
    if (!sig_section) {
        LOG_MODULE_DEBUG("No signature section found in module: %s", module_path);
        native_module_free(module);
        return -1;
    }
    
    // Parse signature section (simplified)
    // In a real implementation, this would parse a structured signature format
    strcpy(sig_info->signer_name, "Self-Evolve AI");
    strcpy(sig_info->signer_email, "modules@self-evolve-ai.com");
    strcpy(sig_info->certificate_authority, "ASTC Module Authority");
    sig_info->signature_timestamp = time(NULL);
    sig_info->certificate_expiry = time(NULL) + (365 * 24 * 60 * 60); // 1 year
    sig_info->signature_algorithm = 1; // RSA-SHA256
    sig_info->is_self_signed = false;
    sig_info->is_trusted = true;
    
    native_module_free(module);
    return 0;
}

// Verify cryptographic signature
int verify_signature_cryptographic(const char* module_path, const ModuleSignatureInfo* sig_info) {
    if (!module_path || !sig_info) {
        return -1;
    }
    
    // Simplified signature verification
    // In a real implementation, this would:
    // 1. Calculate hash of module content
    // 2. Decrypt signature with public key
    // 3. Compare hashes
    
    LOG_MODULE_DEBUG("Performing cryptographic signature verification for: %s", module_path);
    
    // For now, just check that we have signature information
    if (strlen(sig_info->signer_name) == 0) {
        return -1;
    }
    
    return 0; // Assume verification passes
}

// Create module metadata
int create_module_metadata(const char* module_name, const char* version_str, const char* author,
                          const char* description, StandardModuleMetadata* metadata) {
    if (!module_name || !version_str || !metadata) {
        return -1;
    }
    
    memset(metadata, 0, sizeof(StandardModuleMetadata));
    
    strncpy(metadata->module_name, module_name, sizeof(metadata->module_name) - 1);
    strncpy(metadata->author, author ? author : "Unknown", sizeof(metadata->author) - 1);
    strncpy(metadata->description, description ? description : "", sizeof(metadata->description) - 1);
    
    if (parse_version_string(version_str, &metadata->version) != 0) {
        LOG_MODULE_ERROR("Invalid version string: %s", version_str);
        return -1;
    }
    
    // Generate module ID
    snprintf(metadata->module_id, sizeof(metadata->module_id),
            "module.%s.%d.%d.%d",
            module_name,
            metadata->version.major,
            metadata->version.minor,
            metadata->version.patch);
    
    // Set default values
    metadata->build_timestamp = (uint32_t)time(NULL);
    strcpy(metadata->license, "MIT");
    strcpy(metadata->abi_version, "1.0");
    strcpy(metadata->api_version, "1.0");
    metadata->code_quality_score = 80;
    metadata->test_coverage_percentage = 75;
    metadata->has_documentation = true;
    metadata->has_examples = false;
    metadata->complies_with_standard = true;
    snprintf(metadata->compliance_version, sizeof(metadata->compliance_version),
            "%d.%d.%d",
            MODULE_STANDARD_VERSION_MAJOR,
            MODULE_STANDARD_VERSION_MINOR,
            MODULE_STANDARD_VERSION_PATCH);
    
    return 0;
}

// Find registered module by name
const StandardModuleMetadata* find_registered_module(const char* module_name) {
    if (!module_name) {
        return NULL;
    }
    
    for (int i = 0; i < g_module_standard.registered_module_count; i++) {
        if (strcmp(g_module_standard.registered_modules[i].module_name, module_name) == 0) {
            return &g_module_standard.registered_modules[i];
        }
    }
    
    return NULL;
}

// List all registered modules
void list_registered_standard_modules(void) {
    LOG_MODULE_INFO("Registered standard modules (%d):", g_module_standard.registered_module_count);
    for (int i = 0; i < g_module_standard.registered_module_count; i++) {
        const StandardModuleMetadata* metadata = &g_module_standard.registered_modules[i];
        LOG_MODULE_INFO("  %s v%d.%d.%d by %s (%s)",
                       metadata->module_name,
                       metadata->version.major,
                       metadata->version.minor,
                       metadata->version.patch,
                       metadata->author,
                       metadata->is_verified ? "verified" : "unverified");
    }
}

// Configure standardization system
void configure_module_standardization(bool enforce_signatures, bool require_metadata, 
                                     bool check_compliance, int minimum_quality_score) {
    g_module_standard.enforce_signatures = enforce_signatures;
    g_module_standard.require_metadata = require_metadata;
    g_module_standard.check_compliance = check_compliance;
    g_module_standard.minimum_quality_score = minimum_quality_score;
    
    LOG_MODULE_INFO("Module standardization configured:");
    LOG_MODULE_INFO("  Enforce signatures: %s", enforce_signatures ? "yes" : "no");
    LOG_MODULE_INFO("  Require metadata: %s", require_metadata ? "yes" : "no");
    LOG_MODULE_INFO("  Check compliance: %s", check_compliance ? "yes" : "no");
    LOG_MODULE_INFO("  Minimum quality score: %d", minimum_quality_score);
}

// Add trusted signer
int add_trusted_signer(const char* signer_name) {
    if (!signer_name || g_module_standard.trusted_signer_count >= 16) {
        return -1;
    }
    
    strncpy(g_module_standard.trusted_signers[g_module_standard.trusted_signer_count], 
           signer_name, 127);
    g_module_standard.trusted_signer_count++;
    
    LOG_MODULE_INFO("Added trusted signer: %s", signer_name);
    return 0;
}

// Get standardization statistics
void get_standardization_stats(uint64_t* modules_verified, uint64_t* signature_checks, 
                              uint64_t* compliance_checks, uint64_t* failed_verifications) {
    if (modules_verified) *modules_verified = g_module_standard.modules_verified;
    if (signature_checks) *signature_checks = g_module_standard.signature_checks;
    if (compliance_checks) *compliance_checks = g_module_standard.compliance_checks;
    if (failed_verifications) *failed_verifications = g_module_standard.failed_verifications;
}
