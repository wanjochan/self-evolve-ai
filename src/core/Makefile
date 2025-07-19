# Stage 1 Core Modules Makefile
# 修复编译依赖问题，确保所有模块能够正确构建

CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2 -fPIC -I.
LDFLAGS = -shared

# 核心文件
CORE_SOURCES = astc.c
CORE_OBJECTS = $(CORE_SOURCES:.c=.o)

# 模块文件
MODULE_SOURCES = modules/pipeline_module.c modules/c99bin_module.c modules/compiler_module.c modules/libc_module.c
MODULE_OBJECTS = $(MODULE_SOURCES:.c=.o)

# 目标文件
TARGETS = libastc_core.so \
          modules/pipeline_module.so \
          modules/c99bin_module.so \
          modules/compiler_module.so \
          modules/libc_module.so

.PHONY: all clean test

all: $(TARGETS)

# 构建核心库
libastc_core.so: $(CORE_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

# 构建模块
modules/%.so: modules/%.o libastc_core.so
	$(CC) $(LDFLAGS) -o $@ $< -L. -lastc_core

# 编译规则
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 测试编译
test: all
	@echo "✅ 所有Stage 1核心模块编译成功"
	@echo "📦 生成的库文件:"
	@ls -la *.so modules/*.so 2>/dev/null || true

# 清理
clean:
	rm -f $(CORE_OBJECTS) $(MODULE_OBJECTS) $(TARGETS)
	@echo "🧹 清理完成"

# 安装
install: all
	mkdir -p ../../lib
	cp *.so ../../lib/
	cp modules/*.so ../../lib/
	@echo "📦 已安装到 ../../lib/"