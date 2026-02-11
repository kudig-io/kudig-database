#!/bin/bash
# refresh.sh - 快速刷新 gitbook
# 用法:
#   ./refresh.sh          # 重新生成 SUMMARY.md + 构建 + 重启服务
#   ./refresh.sh build    # 仅重新构建（不重新生成 SUMMARY.md）
#   ./refresh.sh serve    # 重新生成 + 构建 + 启动服务（端口默认 3000）

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PORT="${PORT:-3000}"
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

# 停止已有的 mdbook serve 进程
stop_serve() {
    local pids
    pids=$(pgrep -f "mdbook serve" 2>/dev/null || true)
    if [[ -n "$pids" ]]; then
        log_info "停止已有的 mdbook serve 进程..."
        kill $pids 2>/dev/null || true
        sleep 1
    fi
}

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

# 启动服务
start_serve() {
    log_info "启动 mdBook 服务 (端口: $PORT)..."
    cd "$SCRIPT_DIR"
    mdbook serve --port "$PORT" &
    sleep 2
    if curl -s -o /dev/null -w "" "http://localhost:$PORT" 2>/dev/null; then
        log_info "服务已启动: http://localhost:$PORT"
    else
        log_warn "服务启动中，请稍等后访问: http://localhost:$PORT"
    fi
}

# 计时
START_TIME=$(date +%s)

case "$MODE" in
    build)
        build_book
        ;;
    serve)
        stop_serve
        update_symlinks
        generate_summary
        build_book
        start_serve
        ;;
    full|*)
        stop_serve
        update_symlinks
        generate_summary
        build_book
        start_serve
        ;;
esac

END_TIME=$(date +%s)
ELAPSED=$((END_TIME - START_TIME))
log_info "总耗时: ${ELAPSED}s"
