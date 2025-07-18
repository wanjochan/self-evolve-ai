# ARM64架构优化编译标志
# 自动生成，请勿手动编辑

# 基础ARM64优化标志
ARM64_BASE_FLAGS = -march=armv8-a -mtune=cortex-a72 -mcpu=cortex-a72

# 性能优化标志
ARM64_PERF_FLAGS = -O3 -ffast-math -funroll-loops -fprefetch-loop-arrays

# 向量化优化
ARM64_VECTOR_FLAGS = -ftree-vectorize -fvect-cost-model=dynamic

# 内存对齐优化
ARM64_ALIGN_FLAGS = -falign-functions=16 -falign-loops=16 -falign-jumps=16

# ARM64特定优化
ARM64_SPECIFIC_FLAGS = -fomit-frame-pointer -fno-stack-protector

# Apple Silicon特定优化
APPLE_SILICON_FLAGS = -mcpu=apple-a14 -mtune=apple-a14

# Linux ARM64特定优化
LINUX_ARM64_FLAGS = -mcpu=cortex-a72 -mtune=cortex-a72

# 组合标志
ARM64_ALL_FLAGS = $(ARM64_BASE_FLAGS) $(ARM64_PERF_FLAGS) $(ARM64_VECTOR_FLAGS) $(ARM64_ALIGN_FLAGS) $(ARM64_SPECIFIC_FLAGS)

ARM64_PLATFORM_FLAGS = $(ARM64_BASE_FLAGS)

# 最终ARM64编译标志
ARM64_CFLAGS = $(ARM64_ALL_FLAGS) $(ARM64_PLATFORM_FLAGS)
ARM64_CXXFLAGS = $(ARM64_CFLAGS) -std=c++17
ARM64_LDFLAGS = -Wl,-O1 -Wl,--as-needed

