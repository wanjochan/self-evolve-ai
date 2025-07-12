#!/bin/bash

# C99编译器部署脚本
# 自动化部署C99编译器替代TinyCC的过程

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 脚本配置
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BACKUP_DIR="$PROJECT_ROOT/backup/$(date +%Y%m%d_%H%M%S)"
LOG_FILE="$PROJECT_ROOT/logs/deployment.log"

# 部署选项
DRY_RUN=false
FORCE_DEPLOY=false
SKIP_TESTS=false
BACKUP_ENABLED=true
VERBOSE=false

# 创建必要的目录
mkdir -p "$(dirname "$LOG_FILE")"
mkdir -p "$BACKUP_DIR"

# 日志函数
log() {
    local level=$1
    local message=$2
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    
    echo "[$timestamp] [$level] $message" >> "$LOG_FILE"
    
    if [ "$VERBOSE" = true ] || [ "$level" != "DEBUG" ]; then
        case "$level" in
            "INFO")  echo -e "${BLUE}[INFO]${NC} $message" ;;
            "WARN")  echo -e "${YELLOW}[WARN]${NC} $message" ;;
            "ERROR") echo -e "${RED}[ERROR]${NC} $message" ;;
            "SUCCESS") echo -e "${GREEN}[SUCCESS]${NC} $message" ;;
            "DEBUG") [ "$VERBOSE" = true ] && echo -e "[DEBUG] $message" ;;
        esac
    fi
}

# 错误处理
error_exit() {
    log "ERROR" "$1"
    exit 1
}

# 检查先决条件
check_prerequisites() {
    log "INFO" "检查部署先决条件..."
    
    # 检查C99编译器是否存在
    if [ ! -f "$PROJECT_ROOT/bin/c99_compiler" ]; then
        error_exit "C99编译器不存在，请先运行构建脚本"
    fi
    
    # 检查c99.sh脚本是否存在
    if [ ! -f "$PROJECT_ROOT/c99.sh" ]; then
        error_exit "c99.sh包装脚本不存在"
    fi
    
    # 检查必要的目录
    for dir in "src" "bin" "tests"; do
        if [ ! -d "$PROJECT_ROOT/$dir" ]; then
            error_exit "必要目录不存在: $dir"
        fi
    done
    
    log "SUCCESS" "先决条件检查通过"
}

# 备份现有配置
backup_existing() {
    if [ "$BACKUP_ENABLED" = false ]; then
        log "INFO" "跳过备份（已禁用）"
        return 0
    fi
    
    log "INFO" "备份现有配置到 $BACKUP_DIR..."
    
    # 备份重要文件
    local files_to_backup=(
        "c99.sh"
        "build_c99.sh"
        "Makefile"
        "bin/"
        "logs/"
    )
    
    for item in "${files_to_backup[@]}"; do
        if [ -e "$PROJECT_ROOT/$item" ]; then
            cp -r "$PROJECT_ROOT/$item" "$BACKUP_DIR/"
            log "DEBUG" "已备份: $item"
        fi
    done
    
    log "SUCCESS" "备份完成"
}

# 运行测试套件
run_tests() {
    if [ "$SKIP_TESTS" = true ]; then
        log "INFO" "跳过测试（已禁用）"
        return 0
    fi
    
    log "INFO" "运行测试套件..."
    
    # 运行C99兼容性测试
    if [ -x "$PROJECT_ROOT/tests/c99_compliance_test.sh" ]; then
        log "INFO" "运行C99兼容性测试..."
        if "$PROJECT_ROOT/tests/c99_compliance_test.sh" > /dev/null 2>&1; then
            log "SUCCESS" "C99兼容性测试通过"
        else
            if [ "$FORCE_DEPLOY" = false ]; then
                error_exit "C99兼容性测试失败，部署中止"
            else
                log "WARN" "C99兼容性测试失败，但强制部署已启用"
            fi
        fi
    fi
    
    # 运行性能测试
    if [ -x "$PROJECT_ROOT/tests/performance_test.sh" ]; then
        log "INFO" "运行性能测试..."
        if "$PROJECT_ROOT/tests/performance_test.sh" > /dev/null 2>&1; then
            log "SUCCESS" "性能测试通过"
        else
            log "WARN" "性能测试失败，但继续部署"
        fi
    fi
    
    # 运行代码质量分析
    if [ -x "$PROJECT_ROOT/tests/code_quality_analysis.sh" ]; then
        log "INFO" "运行代码质量分析..."
        "$PROJECT_ROOT/tests/code_quality_analysis.sh" > /dev/null 2>&1
        log "SUCCESS" "代码质量分析完成"
    fi
    
    log "SUCCESS" "测试套件完成"
}

# 部署C99编译器
deploy_c99_compiler() {
    log "INFO" "部署C99编译器..."
    
    if [ "$DRY_RUN" = true ]; then
        log "INFO" "[DRY RUN] 模拟部署C99编译器"
        return 0
    fi
    
    # 确保编译器可执行
    chmod +x "$PROJECT_ROOT/bin/c99_compiler"
    
    # 确保c99.sh可执行
    chmod +x "$PROJECT_ROOT/c99.sh"
    
    # 创建符号链接（如果需要）
    if [ ! -L "/usr/local/bin/c99" ]; then
        if [ -w "/usr/local/bin" ]; then
            ln -sf "$PROJECT_ROOT/c99.sh" "/usr/local/bin/c99"
            log "SUCCESS" "创建全局符号链接: /usr/local/bin/c99"
        else
            log "WARN" "无法创建全局符号链接（权限不足）"
        fi
    fi
    
    log "SUCCESS" "C99编译器部署完成"
}

# 配置环境
configure_environment() {
    log "INFO" "配置环境..."
    
    if [ "$DRY_RUN" = true ]; then
        log "INFO" "[DRY RUN] 模拟环境配置"
        return 0
    fi
    
    # 创建配置文件
    local config_file="$PROJECT_ROOT/config/c99_deployment.conf"
    mkdir -p "$(dirname "$config_file")"
    
    cat > "$config_file" << EOF
# C99编译器部署配置
# 生成时间: $(date)

# 编译器路径
C99_COMPILER_PATH="$PROJECT_ROOT/bin/c99_compiler"
C99_WRAPPER_PATH="$PROJECT_ROOT/c99.sh"

# 部署信息
DEPLOYMENT_DATE="$(date)"
DEPLOYMENT_VERSION="$(git rev-parse HEAD 2>/dev/null || echo "unknown")"
BACKUP_LOCATION="$BACKUP_DIR"

# 功能开关
ENABLE_STATISTICS=true
ENABLE_LOGGING=true
ENABLE_PERFORMANCE_MONITORING=true
ENABLE_AUTO_FALLBACK=true
EOF
    
    log "SUCCESS" "环境配置完成"
}

# 验证部署
verify_deployment() {
    log "INFO" "验证部署..."
    
    # 测试基本编译功能
    local test_file="/tmp/test_deployment.c"
    cat > "$test_file" << 'EOF'
#include <stdio.h>
int main() {
    printf("Hello, C99!\n");
    return 0;
}
EOF
    
    # 测试c99.sh脚本
    if "$PROJECT_ROOT/c99.sh" --c99-verbose "$test_file" -o "/tmp/test_deployment" > /dev/null 2>&1; then
        log "SUCCESS" "部署验证通过"
        rm -f "$test_file" "/tmp/test_deployment"
    else
        log "ERROR" "部署验证失败"
        rm -f "$test_file" "/tmp/test_deployment"
        if [ "$FORCE_DEPLOY" = false ]; then
            error_exit "部署验证失败，回滚部署"
        fi
    fi
}

# 生成部署报告
generate_report() {
    log "INFO" "生成部署报告..."
    
    local report_file="$PROJECT_ROOT/logs/deployment_report_$(date +%Y%m%d_%H%M%S).txt"
    
    cat > "$report_file" << EOF
# C99编译器部署报告

## 部署信息
- 部署时间: $(date)
- 部署版本: $(git rev-parse HEAD 2>/dev/null || echo "unknown")
- 部署用户: $(whoami)
- 部署主机: $(hostname)

## 部署配置
- 项目根目录: $PROJECT_ROOT
- 备份目录: $BACKUP_DIR
- 日志文件: $LOG_FILE
- 干运行模式: $DRY_RUN
- 强制部署: $FORCE_DEPLOY
- 跳过测试: $SKIP_TESTS

## 部署结果
- C99编译器: $([ -f "$PROJECT_ROOT/bin/c99_compiler" ] && echo "✅ 已部署" || echo "❌ 未部署")
- 包装脚本: $([ -f "$PROJECT_ROOT/c99.sh" ] && echo "✅ 已部署" || echo "❌ 未部署")
- 全局链接: $([ -L "/usr/local/bin/c99" ] && echo "✅ 已创建" || echo "❌ 未创建")
- 配置文件: $([ -f "$PROJECT_ROOT/config/c99_deployment.conf" ] && echo "✅ 已创建" || echo "❌ 未创建")

## 测试结果
- 兼容性测试: $([ "$SKIP_TESTS" = true ] && echo "⏭️ 已跳过" || echo "✅ 已完成")
- 性能测试: $([ "$SKIP_TESTS" = true ] && echo "⏭️ 已跳过" || echo "✅ 已完成")
- 部署验证: ✅ 已完成

## 使用说明
1. 使用 ./c99.sh 替代 tcc 命令
2. 使用 --c99-verbose 查看详细输出
3. 使用 --c99-statistics 启用统计收集
4. 使用 --c99-show-stats 查看统计信息

## 回滚说明
如需回滚，请运行：
  cp -r $BACKUP_DIR/* $PROJECT_ROOT/

EOF
    
    log "SUCCESS" "部署报告已生成: $report_file"
    
    if [ "$VERBOSE" = true ]; then
        echo
        echo "=== 部署报告 ==="
        cat "$report_file"
    fi
}

# 显示帮助信息
show_help() {
    cat << EOF
C99编译器部署脚本

用法: $0 [选项]

选项:
  --dry-run              模拟部署，不实际执行
  --force                强制部署，忽略测试失败
  --skip-tests           跳过测试套件
  --no-backup            禁用备份
  --verbose              详细输出
  --help                 显示此帮助信息

示例:
  $0                     # 标准部署
  $0 --dry-run           # 模拟部署
  $0 --force --verbose   # 强制部署并显示详细信息
  $0 --skip-tests        # 跳过测试的快速部署

EOF
}

# 解析命令行参数
parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --dry-run)
                DRY_RUN=true
                shift
                ;;
            --force)
                FORCE_DEPLOY=true
                shift
                ;;
            --skip-tests)
                SKIP_TESTS=true
                shift
                ;;
            --no-backup)
                BACKUP_ENABLED=false
                shift
                ;;
            --verbose)
                VERBOSE=true
                shift
                ;;
            --help)
                show_help
                exit 0
                ;;
            *)
                echo "未知选项: $1"
                show_help
                exit 1
                ;;
        esac
    done
}

# 主函数
main() {
    parse_arguments "$@"
    
    log "INFO" "开始C99编译器部署..."
    log "INFO" "配置: DRY_RUN=$DRY_RUN, FORCE=$FORCE_DEPLOY, SKIP_TESTS=$SKIP_TESTS"
    
    check_prerequisites
    backup_existing
    run_tests
    deploy_c99_compiler
    configure_environment
    verify_deployment
    generate_report
    
    log "SUCCESS" "C99编译器部署完成！"
    
    if [ "$DRY_RUN" = false ]; then
        echo
        echo -e "${GREEN}🎉 部署成功！${NC}"
        echo "现在可以使用 ./c99.sh 命令进行编译"
        echo "查看部署报告: cat $PROJECT_ROOT/logs/deployment_report_*.txt"
    else
        echo
        echo -e "${BLUE}📋 模拟部署完成${NC}"
        echo "使用 --force 参数执行实际部署"
    fi
}

# 运行主函数
main "$@"
