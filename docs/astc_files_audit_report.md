# ASTC Files Audit Report

## Executive Summary

Found **69 ASTC files** in bin/ directory with massive redundancy and naming inconsistencies. Only **~10 unique programs** exist, with the rest being duplicates, variants, or misplaced files.

## File Categorization

### 🎯 **Layer 3 Programs (PRD Compliant)**
These should be the only ASTC files according to PRD.md:

1. **c99.astc** (missing - should consolidate c99_program*.astc)
   - `c99_program.astc` (317 bytes) ⭐ **BASE**
   - `c99_program_env.astc` (2,662 bytes)
   - `c99_program_fixed.astc` (2,560 bytes) 
   - `c99_program_self_hosted.astc` (2,544 bytes)
   - `c99_program_test.astc` (372 bytes)

2. **evolver0.astc** (missing - should consolidate evolver0_program*.astc)
   - `evolver0_program.astc` (6,848 bytes) ⭐ **BASE**
   - `evolver0_program_c99.astc` (1,487 bytes)
   - `evolver0_program_independent.astc` (6,848 bytes) - duplicate
   - `evolver0_program_minimal.astc` (169 bytes)
   - `evolver0_program_test.astc` (29,018 bytes)

3. **evolver1.astc** (missing - should consolidate evolver1_program*.astc)
   - `evolver1_program.astc` (28,042 bytes) ⭐ **BASE**
   - `evolver1_program_c99.astc` (1,487 bytes)
   - `evolver1_program_self.astc` (1,016 bytes)

4. **program_c99.astc** (10,387 bytes) - Unclear purpose

### ❌ **Layer 2 Components (Misplaced)**
These belong in Layer 2 (.native format), not Layer 3:

#### Runtime/VM Files (Should be .native)
- `c99_runtime*.astc` (9 variants) → Should be `vm_x64_64.native`
- `evolver0_runtime*.astc` (6 variants) → Should be `vm_x64_64.native`
- `evolver1_runtime*.astc` (2 variants) → Should be `vm_x64_64.native`
- `vm_x64_64.astc` (1,667 bytes) → Should be `vm_x64_64.native`
- `vm_x64_64_native.astc` (559 bytes) → Should be `vm_x64_64.native`

#### LibC Modules (Should be .native)
- `core_libc*.astc` (4 variants) → Should be `libc_x64_64.native`
- `libc_minimal.astc` (164 bytes) → Should be `libc_x64_64.native`
- `libc_os.astc` (308 bytes) → Should be `libc_x64_64.native`
- `libc_version_manager.astc` (394 bytes) → Should be `libc_x64_64.native`
- `libc_x64_64.astc` (749 bytes) → Should be `libc_x64_64.native`
- `libc_x64_64_native.astc` (749 bytes) → Should be `libc_x64_64.native`

### ❌ **Layer 1 Components (Misplaced)**
These belong in Layer 1 (loader executables), not Layer 3:

- `core_loader*.astc` (4 variants) → Should be `loader_x64_64.exe`
- `evolver0_loader*.astc` (2 variants) → Should be `loader_x64_64.exe`
- `evolver1_loader*.astc` (2 variants) → Should be `loader_x64_64.exe`
- `loader*.astc` (3 variants) → Should be `loader_x64_64.exe`
- `universal_loader.astc` (681 bytes) → Should be `loader_x64_64.exe`

### 🔧 **Development Tools (Should be separate)**
These are development tools, not user programs:

#### Compilers
- `tool_c2astc*.astc` (6 variants) → Development tools
- `tool_astc2rt*.astc` (5 variants) → Development tools
- `tool_astc2native_new.astc` (1,923 bytes) → Development tools
- `tool_astc2asm.astc` (411 bytes) → Development tools

#### Compiler Components
- `compiler_astc2rt*.astc` (4 variants) → Development tools

#### Module Management
- `module_call_optimizer.astc` (1,028 bytes) → Development tools
- `rt_module_manager.astc` (140 bytes) → Development tools
- `tool_rt_builder.astc` (22 bytes) → Development tools

### 🗑️ **Empty/Broken Files**
- `loader_unified.astc` (22 bytes) - Too small
- `simple_runtime_new.astc` (22 bytes) - Too small
- `tool_rt_builder.astc` (22 bytes) - Too small
- `tool_c2astc_self_hosted.astc` (48 bytes) - Too small

## PRD Compliance Analysis

### ✅ What Should Remain (Layer 3 Programs)
```
c99.astc           # C99 compiler program
evolver0.astc      # Evolution engine v0
evolver1.astc      # Evolution engine v1
```

### ❌ What Should Be Moved/Converted

#### To Layer 2 (.native modules)
```
vm_x64_64.native      # Consolidated from all runtime*.astc
libc_x64_64.native    # Consolidated from all libc*.astc
```

#### To Layer 1 (loader executables)
```
loader_x64_64.exe     # Consolidated from all loader*.astc
```

#### To tools/ directory
```
tools/c2astc.exe      # From tool_c2astc*.astc
tools/astc2native.exe # From tool_astc2*.astc
tools/astc2asm.exe    # From tool_astc2asm.astc
```

## Redundancy Analysis

### Massive Duplication
- **C99 Runtime**: 9 different versions
- **Evolver0 Runtime**: 6 different versions  
- **LibC Module**: 6 different versions
- **Loader**: 11 different versions
- **C2ASTC Tool**: 6 different versions
- **ASTC2RT Tool**: 5 different versions

### Naming Inconsistencies
- Mixed conventions: `_enhanced`, `_new`, `_fixed`, `_independent`, `_self`
- No clear versioning system
- Random suffixes without documentation

## File Size Analysis

### Suspiciously Small Files (< 100 bytes)
- `loader_unified.astc` (22 bytes)
- `simple_runtime_new.astc` (22 bytes)
- `tool_rt_builder.astc` (22 bytes)
- `tool_c2astc_self_hosted.astc` (48 bytes)

### Large Files (> 10KB)
- `evolver1_program.astc` (28,042 bytes)
- `evolver0_program_test.astc` (29,018 bytes)
- `program_c99.astc` (10,387 bytes)

## Cleanup Actions Required

### Files to Keep (3 files)
```
bin/layer3/c99.astc           # From c99_program.astc
bin/layer3/evolver0.astc      # From evolver0_program.astc  
bin/layer3/evolver1.astc      # From evolver1_program.astc
```

### Files to Delete (66 files)
- All `*_enhanced`, `*_new`, `*_fixed`, `*_independent` variants
- All runtime/vm ASTC files (move to Layer 2)
- All libc ASTC files (move to Layer 2)
- All loader ASTC files (move to Layer 1)
- All tool ASTC files (move to tools/)
- All empty/broken files

### Files to Convert
- Runtime ASTC → `vm_x64_64.native`
- LibC ASTC → `libc_x64_64.native`
- Loader ASTC → `loader_x64_64.exe`
- Tool ASTC → Tool executables

## Next Steps

1. **Identify canonical versions** of each program
2. **Rename to PRD convention** (c99.astc, evolver0.astc, evolver1.astc)
3. **Move misplaced files** to correct layers
4. **Delete redundant variants**
5. **Update build system** to prevent future duplication

## Impact Assessment

- **Cleanup**: Remove 66 of 69 files (96% reduction)
- **Compliance**: Achieve 100% PRD naming compliance
- **Clarity**: Clear separation of Layer 1/2/3 components
- **Maintenance**: Eliminate version confusion
