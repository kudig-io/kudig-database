#!/bin/bash
# start.sh - 首次启动 / 初始化 gitbook
# 用法:
#   ./start.sh              # 初始化并启动服务（端口默认 3000）
#   PORT=8080 ./start.sh    # 指定端口启动

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PORT="${PORT:-3000}"

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
pids=$(pgrep -f "mdbook serve" 2>/dev/null || true)
if [[ -n "$pids" ]]; then
    log_info "停止已有的 mdbook serve 进程..."
    kill $pids 2>/dev/null || true
    sleep 1
fi

START_TIME=$(date +%s)

# 1. 创建/更新符号链接
log_info "更新符号链接..."
src_dir="$SCRIPT_DIR/src"
project_root="$(dirname "$SCRIPT_DIR")"

find "$src_dir" -maxdepth 1 -type l ! -exec test -e {} \; -delete 2>/dev/null || true

for dir in "$project_root"/domain-* "$project_root"/topic-* "$project_root"/tables; do
    if [[ -d "$dir" ]]; then
        name=$(basename "$dir")
        link="$src_dir/$name"
        if [[ ! -L "$link" ]]; then
            ln -sf "../../$name" "$link"
            log_info "  新增符号链接: $name"
        fi
    fi
done

# 2. 生成 SUMMARY.md
log_info "生成 SUMMARY.md..."
bash "$SCRIPT_DIR/generate-summary.sh"

# 3. 构建
log_info "构建 mdBook..."
cd "$SCRIPT_DIR"
mdbook build 2>&1 | grep -v "^$"
count=$(find "$SCRIPT_DIR/book" -name "*.html" -not -name "print.html" | wc -l | tr -d ' ')
log_info "构建完成，共 $count 个 HTML 页面"

# 4. 启动服务
log_info "启动 mdBook 服务 (端口: $PORT)..."
mdbook serve --port "$PORT" &
sleep 2
if curl -s -o /dev/null -w "" "http://localhost:$PORT" 2>/dev/null; then
    log_info "服务已启动: http://localhost:$PORT"
else
    log_warn "服务启动中，请稍等后访问: http://localhost:$PORT"
fi

END_TIME=$(date +%s)
ELAPSED=$((END_TIME - START_TIME))
log_info "总耗时: ${ELAPSED}s"
