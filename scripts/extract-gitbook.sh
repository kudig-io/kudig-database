#!/bin/bash
# extract-gitbook.sh - 安全解压 KUDIG Gitbook ZIP 文件
# 用法:
#   bash scripts/extract-gitbook.sh                    # 解压最新的 ZIP 文件
#   bash scripts/extract-gitbook.sh <zip-file-path>    # 解压指定的 ZIP 文件

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

log_info()  { echo -e "${GREEN}[INFO]${NC} $*"; }
log_warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
log_error() { echo -e "${RED}[ERROR]${NC} $*"; }

# 查找最新的 ZIP 文件
find_latest_zip() {
    local export_dir="$PROJECT_ROOT/gitbook/export"
    local build_dir="$PROJECT_ROOT/gitbook/build-scripts"

    local latest_zip=""

    # 检查 export 目录
    if [[ -d "$export_dir" ]]; then
        latest_zip=$(find "$export_dir" -name "*.zip" -type f 2>/dev/null | sort | tail -1)
    fi

    # 检查 build-scripts 目录
    if [[ -d "$build_dir" ]]; then
        local build_latest=$(find "$build_dir" -name "*.zip" -type f 2>/dev/null | sort | tail -1)
        if [[ -n "$build_latest" ]] && [[ "$build_latest" > "$latest_zip" ]]; then
            latest_zip="$build_latest"
        fi
    fi

    echo "$latest_zip"
}

# 验证 ZIP 文件
validate_zip() {
    local zip_file="$1"

    if [[ ! -f "$zip_file" ]]; then
        log_error "文件不存在: $zip_file"
        return 1
    fi

    log_info "验证 ZIP 文件完整性..."
    if ! unzip -t "$zip_file" >/dev/null 2>&1; then
        log_error "ZIP 文件已损坏: $zip_file"
        return 1
    fi

    log_info "ZIP 文件验证通过"
    return 0
}

# 安全解压
safe_extract() {
    local zip_file="$1"
    local target_dir="$2"

    log_info "创建目标目录: $target_dir"
    mkdir -p "$target_dir"

    log_info "开始解压..."
    log_warn "注意: 如果 ZIP 文件在 Windows 上创建，可能会出现路径分隔符警告"

    # 使用 unzip 解压，处理可能的路径问题
    cd "$target_dir"

    # 尝试解压，捕获警告但不中断
    if unzip -o -q "$zip_file" 2>&1 | grep -v "warning:" | grep -i "error"; then
        log_error "解压过程中出现错误"
        return 1
    fi

    # 检查解压结果
    local file_count=$(find "$target_dir" -type f | wc -l | tr -d ' ')
    if [[ $file_count -eq 0 ]]; then
        log_error "解压后目录为空"
        return 1
    fi

    log_info "解压完成，共 $file_count 个文件"
    return 0
}

# 修复路径分隔符问题（如果需要）
fix_path_separators() {
    local target_dir="$1"

    log_info "检查路径分隔符问题..."

    # 查找包含反斜杠的文件或目录
    local bad_paths=$(find "$target_dir" -name "*\\*" 2>/dev/null | wc -l | tr -d ' ')

    if [[ $bad_paths -gt 0 ]]; then
        log_warn "发现 $bad_paths 个包含反斜杠的路径，这可能是 Windows 创建 ZIP 导致的"
        log_warn "建议重新在 macOS/Linux 上创建 ZIP 文件，或使用以下命令手动修复:"
        log_warn "  cd $target_dir"
        log_warn "  find . -name '*\\\\*' -exec sh -c 'mv \"\$1\" \"\$(echo \$1 | tr '\\\\\\\\' '/')\"' _ {} \\;"
    else
        log_info "路径分隔符正常"
    fi
}

# 主流程
main() {
    echo ""
    echo -e "${CYAN}╔══════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║   KUDIG Gitbook ZIP 解压工具             ║${NC}"
    echo -e "${CYAN}╚══════════════════════════════════════════╝${NC}"
    echo ""

    local zip_file="${1:-}"

    # 如果没有指定文件，查找最新的
    if [[ -z "$zip_file" ]]; then
        log_info "未指定 ZIP 文件，查找最新的..."
        zip_file=$(find_latest_zip)

        if [[ -z "$zip_file" ]]; then
            log_error "未找到任何 ZIP 文件"
            exit 1
        fi

        log_info "找到最新的 ZIP 文件: $zip_file"
    fi

    # 验证 ZIP 文件
    if ! validate_zip "$zip_file"; then
        exit 1
    fi

    # 创建解压目录
    local timestamp=$(date +%Y%m%d-%H%M%S)
    local zip_basename=$(basename "$zip_file" .zip)
    local target_dir="$PROJECT_ROOT/gitbook/extracted/${zip_basename}-${timestamp}"

    # 执行解压
    if ! safe_extract "$zip_file" "$target_dir"; then
        log_error "解压失败"
        exit 1
    fi

    # 检查路径问题
    fix_path_separators "$target_dir"

    # 显示结果
    echo ""
    log_info "✓ 解压成功完成"
    log_info "  解压目录: $target_dir"
    log_info "  文件大小: $(du -sh "$target_dir" | cut -f1)"
    echo ""
    log_info "下一步操作:"
    log_info "  查看文件:   ls -la $target_dir"
    log_info "  打开文档:   open $target_dir/index.html (如果有)"
    echo ""
}

main "$@"
