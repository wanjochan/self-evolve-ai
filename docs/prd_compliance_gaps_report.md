# PRD Compliance Gaps Report

## Executive Summary

**Current PRD Compliance: 15%** 

The codebase has **massive deviations** from PRD.md specifications. The three-layer architecture is completely violated with components scattered across wrong layers and inconsistent naming.

## PRD.md Requirements vs Current State

### Layer 1: Loader Requirements

#### ✅ PRD Specification
```
Layer 1 Loader: loader_{arch}.exe
- 执行入口，未来参考cosmopolitan等制作跨架构统一入口loader.exe
```

#### ❌ Current Implementation
- **6 different loader implementations** instead of 1
- **11 ASTC loader files** in wrong layer
- **No loader_{arch}.exe** executables
- **Scattered across** src/core/loader/, src/legacy/, bin/

#### 🔧 Required Changes
1. Consolidate 6 loaders into 1 unified implementation
2. Generate `loader_x64_64.exe`, `loader_arm64_64.exe` executables
3. Remove all loader ASTC files from Layer 3
4. Move loader source to `src/layer1/`

### Layer 2: Native Modules Requirements

#### ✅ PRD Specification
```
Layer 2 Runtime: {module}_{arch}_{bits}.native
- .native原生字节码模块，其中最重要是vm模块用于加载astc运行
```

#### ❌ Current Implementation
- **Wrong magic number**: Files use "RTME" instead of "NATV"
- **Wrong naming**: `libc_minimal.native` instead of `libc_x64_64.native`
- **Missing standard modules**: No `vm_x64_64.native` in bin/
- **ASTC files in wrong layer**: 17 runtime/vm ASTC files should be .native
- **Multiple LibC versions**: 6 different libc implementations

#### 🔧 Required Changes
1. Fix .native format to use "NATV" magic number
2. Standardize naming: `vm_x64_64.native`, `libc_x64_64.native`
3. Convert all runtime ASTC files to .native format
4. Consolidate LibC implementations into single module
5. Move native module source to `src/layer2/`

### Layer 3: ASTC Programs Requirements

#### ✅ PRD Specification
```
Layer 3 Program: {programName}.astc
- 用户程序（比如c99、evolver{version}）ASTC字节码
```

#### ❌ Current Implementation
- **69 ASTC files** instead of ~3 programs
- **Wrong naming**: `c99_program.astc` instead of `c99.astc`
- **Layer violations**: Runtime, loader, and tool files in Layer 3
- **Massive redundancy**: 9 versions of c99_runtime, 6 versions of evolver0_runtime

#### 🔧 Required Changes
1. Reduce 69 files to 3: `c99.astc`, `evolver0.astc`, `evolver1.astc`
2. Move all non-program files to correct layers
3. Eliminate redundant variants
4. Move ASTC program source to `src/layer3/`

## Directory Structure Compliance

### ✅ PRD Implied Structure
```
src/
├── layer1/          # Loader implementation
├── layer2/          # Native modules (vm, libc)
└── layer3/          # ASTC programs

bin/
├── layer1/          # loader_{arch}.exe
├── layer2/          # {module}_{arch}_{bits}.native
└── layer3/          # {programName}.astc
```

### ❌ Current Structure
```
src/
├── core/            # Mixed Layer 1, 2, 3 components
├── legacy/          # Duplicate implementations
├── compiler/        # Should be tools/
├── tools/           # Unrelated automation tools
└── ai/              # Should be in layer3/

bin/                 # All files mixed together
├── 69 .astc files   # Should be 3 in layer3/
├── 7 .native files  # Should be 2 in layer2/
└── No .exe files    # Should have loaders in layer1/
```

## Critical Compliance Violations

### 1. Architecture Violation (Severity: CRITICAL)
- **Issue**: Three layers completely mixed
- **Impact**: Cannot achieve PRD goals
- **Fix**: Complete restructuring required

### 2. Naming Convention Violation (Severity: HIGH)
- **Issue**: No files follow PRD naming
- **Impact**: Loader cannot find modules
- **Fix**: Rename all files to PRD convention

### 3. File Format Violation (Severity: HIGH)
- **Issue**: .native files use wrong format
- **Impact**: Module loading fails
- **Fix**: Implement proper NATV format

### 4. Redundancy Violation (Severity: MEDIUM)
- **Issue**: 96% of files are duplicates
- **Impact**: Confusion, maintenance burden
- **Fix**: Delete redundant files

## Compliance Roadmap

### Phase 1: Directory Restructuring (Week 1)
```
✅ Create src/layer1/, src/layer2/, src/layer3/
✅ Create bin/layer1/, bin/layer2/, bin/layer3/
✅ Move legacy/ to archive/
```

### Phase 2: Layer 1 Consolidation (Week 1)
```
✅ Merge 6 loaders into unified implementation
✅ Generate loader_x64_64.exe
✅ Remove loader ASTC files
```

### Phase 3: Layer 2 Standardization (Week 2)
```
✅ Fix .native format (NATV magic)
✅ Create vm_x64_64.native
✅ Create libc_x64_64.native
✅ Remove runtime ASTC files
```

### Phase 4: Layer 3 Cleanup (Week 2)
```
✅ Keep only c99.astc, evolver0.astc, evolver1.astc
✅ Delete 66 redundant ASTC files
✅ Move tools to separate directory
```

### Phase 5: Build System Update (Week 3)
```
✅ Update CMakeLists.txt for three layers
✅ Create layer-specific build targets
✅ Remove obsolete build scripts
```

### Phase 6: Testing & Validation (Week 3)
```
✅ Test loader → vm → program flow
✅ Validate PRD compliance
✅ Performance benchmarking
```

## Success Metrics

### Compliance Score Targets
- **Current**: 15% PRD compliant
- **Phase 1**: 30% (structure)
- **Phase 3**: 60% (layers separated)
- **Phase 6**: 95% (fully compliant)

### File Count Targets
- **Current**: 69 ASTC + 7 native + 6 loaders = 82 files
- **Target**: 3 ASTC + 2 native + 1 loader = 6 files
- **Reduction**: 93% file count reduction

### Architecture Clarity
- **Current**: Components scattered across layers
- **Target**: Clear separation of Layer 1/2/3 responsibilities

## Risk Assessment

### High Risk
- **Breaking changes**: Existing scripts may fail
- **Build system**: Requires complete rebuild
- **Testing**: Need comprehensive validation

### Medium Risk
- **File format**: .native format changes
- **Naming**: Scripts hardcoded to old names

### Low Risk
- **Directory structure**: Easy to revert
- **File deletion**: Can be recovered from git

## Implementation Priority

### Critical Path (Must Fix)
1. Directory restructuring
2. Layer 1 consolidation
3. Layer 2 standardization
4. Layer 3 cleanup

### Nice to Have (Future)
1. Cross-platform loaders
2. Additional architectures
3. Advanced module features

## Conclusion

The codebase requires **fundamental restructuring** to achieve PRD compliance. Current 15% compliance is due to:

- ✅ ASTC bytecode format exists
- ✅ Some naming follows patterns
- ✅ Basic three-layer concept present

**Major work required**:
- Complete directory restructuring
- File format standardization  
- Massive file cleanup (93% reduction)
- Build system overhaul

**Estimated effort**: 3 weeks of focused development

**Success criteria**: Achieve 95% PRD compliance with clean three-layer architecture
