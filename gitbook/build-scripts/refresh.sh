#!/bin/bash
# refresh.sh - 内容更新后快速刷新 gitbook
# 用法:
#   ./refresh.sh          # 完整刷新：更新符号链接 + 重新生成目录 + 重新构建
#   ./refresh.sh build    # 仅重新构建（不更新符号链接和目录，速度更快）

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MODE="${1:-full}"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info()  { echo -e "${GREEN}[INFO]${NC} $*"; }
log_warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
log_error() { echo -e "${RED}[ERROR]${NC} $*"; }

# 检查 mdbook 是否安装
if ! command -v mdbook &>/dev/null; then
    log_error "mdbook 未安装，请先安装: cargo install mdbook"
    exit 1
fi

# 更新符号链接
update_symlinks() {
    log_info "更新符号链接..."
    local src_dir="$SCRIPT_DIR/src"
    local project_root="$(dirname "$SCRIPT_DIR")"

    # 清理无效的符号链接
    find "$src_dir" -maxdepth 1 -type l ! -exec test -e {} \; -delete 2>/dev/null || true

    # 为所有 domain-* 和 topic-* 目录创建符号链接
    for dir in "$project_root"/domain-* "$project_root"/topic-* "$project_root"/tables; do
        if [[ -d "$dir" ]]; then
            local name=$(basename "$dir")
            local link="$src_dir/$name"
            if [[ ! -L "$link" ]]; then
                ln -sf "../../$name" "$link"
                log_info "  新增符号链接: $name"
            fi
        fi
    done
}

# 生成 SUMMARY.md
generate_summary() {
    log_info "生成 SUMMARY.md..."
    bash "$SCRIPT_DIR/generate-summary.sh"
}

# 构建
build_book() {
    log_info "构建 mdBook..."
    cd "$SCRIPT_DIR"
    mdbook build 2>&1 | grep -v "^$"
    local count
    count=$(find "$SCRIPT_DIR/book" -name "*.html" -not -name "print.html" | wc -l | tr -d ' ')
    log_info "构建完成，共 $count 个 HTML 页面"
}

# 计时
START_TIME=$(date +%s)

case "$MODE" in
    build)
        build_book
        ;;
    full|*)
        update_symlinks
        generate_summary
        build_book
        ;;
esac

END_TIME=$(date +%s)
ELAPSED=$((END_TIME - START_TIME))
log_info "总耗时: ${ELAPSED}s"

# 提示
echo ""
if pgrep -f "mdbook serve" &>/dev/null; then
    log_info "检测到 mdbook serve 正在运行，页面将自动热更新"
else
    log_warn "mdbook serve 未运行，如需预览请执行: bash start.sh"
fi
