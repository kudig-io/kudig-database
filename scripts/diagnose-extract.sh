#!/bin/bash
# diagnose-extract.sh - 诊断 ZIP 解压问题
# 用法: bash scripts/diagnose-extract.sh [zip-file]

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
log_check() { echo -e "${CYAN}[CHECK]${NC} $*"; }

# 诊断 ZIP 文件
diagnose_zip() {
    local zip_file="$1"

    echo ""
    echo -e "${CYAN}╔══════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║   KUDIG ZIP 解压诊断工具                 ║${NC}"
    echo -e "${CYAN}╚══════════════════════════════════════════╝${NC}"
    echo ""

    # 1. 检查文件是否存在
    log_check "检查文件是否存在..."
    if [[ ! -f "$zip_file" ]]; then
        log_error "文件不存在: $zip_file"
        return 1
    fi
    log_info "✓ 文件存在"

    # 2. 检查文件大小
    log_check "检查文件大小..."
    local file_size=$(stat -f%z "$zip_file" 2>/dev/null || stat -c%s "$zip_file" 2>/dev/null)
    local file_size_mb=$((file_size / 1024 / 1024))
    log_info "  文件大小: ${file_size_mb}MB ($file_size bytes)"

    if [[ $file_size -eq 0 ]]; then
        log_error "文件大小为 0，无法解压"
        return 1
    fi
    log_info "✓ 文件大小正常"

    # 3. 检查文件类型
    log_check "检查文件类型..."
    local file_type=$(file "$zip_file")
    log_info "  文件类型: $file_type"

    if ! echo "$file_type" | grep -qi "zip"; then
        log_error "文件不是有效的 ZIP 格式"
        return 1
    fi
    log_info "✓ 文件格式正确"

    # 4. 检查 ZIP 完整性
    log_check "检查 ZIP 完整性..."
    if unzip -t "$zip_file" >/dev/null 2>&1; then
        log_info "✓ ZIP 文件完整，无损坏"
    else
        log_error "ZIP 文件已损坏或不完整"
        log_info "尝试查看具体错误..."
        unzip -t "$zip_file" 2>&1 | grep -i "error\|fail" | head -10
        return 1
    fi

    # 5. 检查路径分隔符问题
    log_check "检查路径分隔符..."
    local test_extract=$(mktemp -d)
    local original_dir=$(pwd)
    cd "$test_extract"

    local warnings=$(unzip -t "$zip_file" 2>&1 | grep -c "warning:" || true)
    if [[ $warnings -gt 0 ]]; then
        log_warn "⚠ 发现 $warnings 个警告（可能是 Windows 路径分隔符问题）"
        log_info "  这是常见问题，不影响解压，但可能导致路径混乱"
    else
        log_info "✓ 路径分隔符正常"
    fi

    cd "$original_dir"

    # 6. 检查中文文件名
    log_check "检查中文字符文件名..."
    local chinese_names=$(unzip -l "$zip_file" 2>&1 | grep -c "['\u4e00-'\u9fa5]" || true)
    if [[ $chinese_names -gt 0 ]]; then
        log_warn "⚠ 发现 $chinese_names 个包含中文字符的文件"
        log_info "  在某些系统上可能会有编码问题"
    else
        log_info "✓ 无中文字符文件名"
    fi

    # 7. 检查权限
    log_check "检查文件权限..."
    if [[ -r "$zip_file" ]] && [[ -f "$zip_file" ]]; then
        log_info "✓ 文件可读"
    else
        log_warn "⚠ 文件权限可能有问题，尝试修复..."
        chmod +r "$zip_file" 2>/dev/null || true
        if [[ -r "$zip_file" ]]; then
            log_info "✓ 文件权限已修复，现在可读"
        else
            log_error "文件不可读，请手动检查权限"
            return 1
        fi
    fi

    # 8. 检查磁盘空间
    log_check "检查磁盘空间..."
    local available_space=$(df -m "$(dirname "$zip_file")" | tail -1 | awk '{print $4}')
    local required_space=$((file_size_mb * 3))  # 需要约 3 倍的空间

    log_info "  可用空间: ${available_space}MB"
    log_info "  预估需要: ${required_space}MB"

    if [[ $available_space -lt $required_space ]]; then
        log_warn "⚠ 磁盘空间可能不足"
        log_info "  建议至少保留 ${required_space}MB 可用空间"
    else
        log_info "✓ 磁盘空间充足"
    fi

    # 9. 检查 unzip 工具
    log_check "检查 unzip 工具..."
    if command -v unzip &>/dev/null; then
        local unzip_version=$(unzip -version 2>&1 | head -1)
        log_info "  unzip 版本: $unzip_version"
        log_info "✓ unzip 工具可用"
    else
        log_error "未找到 unzip 工具"
        log_info "安装方法: brew install unzip (macOS) 或 apt-get install unzip (Linux)"
        return 1
    fi

    # 10. 测试解压
    log_check "测试解压到临时目录..."
    local extract_output=$(unzip -q -o "$zip_file" -d "$test_extract" 2>&1)
    local extract_exit=$?

    # 检查是否有真正的错误（忽略警告）
    local real_errors=$(echo "$extract_output" | grep -v "warning:" | grep -i "error\|fail" || true)

    if [[ $extract_exit -eq 0 ]] && [[ -z "$real_errors" ]]; then
        local extracted_count=$(find "$test_extract" -type f | wc -l | tr -d ' ')
        log_info "✓ 测试解压成功，共 $extracted_count 个文件"

        # 检查是否有路径分隔符警告
        if echo "$extract_output" | grep -q "warning:"; then
            log_warn "⚠ 解压时有路径分隔符警告（Windows 创建的 ZIP）"
            log_info "  这不影响使用，但可能导致目录结构略有异常"
        fi
    else
        log_error "测试解压失败"
        if [[ -n "$real_errors" ]]; then
            log_info "错误详情:"
            echo "$real_errors" | head -5
        fi
        return 1
    fi

    # 清理
    rm -rf "$test_extract"

    # 总结
    echo ""
    echo -e "${GREEN}╔══════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║   诊断完成 - ZIP 文件正常                ║${NC}"
    echo -e "${GREEN}╚══════════════════════════════════════════╝${NC}"
    echo ""
    log_info "该 ZIP 文件可以正常解压"
    log_info "如果之前解压失败，可能的原因："
    log_info "  1. 目标目录权限不足"
    log_info "  2. 磁盘空间不足"
    log_info "  3. 路径分隔符警告（不影响使用）"
    log_info "  4. 使用了不兼容的解压工具"
    echo ""
    log_info "推荐解压命令："
    log_info "  unzip -o $zip_file -d <目标目录>"
    echo ""
    log_info "或使用我们的专用解压脚本："
    log_info "  bash scripts/extract-gitbook.sh $zip_file"
    echo ""
}

# 主流程
main() {
    local zip_file="${1:-}"

    if [[ -z "$zip_file" ]]; then
        log_error "请指定 ZIP 文件路径"
        echo ""
        log_info "用法: bash scripts/diagnose-extract.sh <zip-file>"
        echo ""
        log_info "可用的 ZIP 文件："
        find "$PROJECT_ROOT/gitbook" -name "*.zip" -type f 2>/dev/null | while read f; do
            local size=$(du -sh "$f" | cut -f1)
            echo "  $f ($size)"
        done
        exit 1
    fi

    diagnose_zip "$zip_file"
}

main "$@"
