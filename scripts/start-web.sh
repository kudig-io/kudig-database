#!/bin/bash
# start-web.sh - 启动 KUDIG Workspace Web 服务
# 用法:
#   bash scripts/start-web.sh              # 默认端口 8767
#   PORT=9000 bash scripts/start-web.sh    # 自定义端口
#   bash scripts/start-web.sh --stop       # 停止服务

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
PORT="${PORT:-8767}"
PID_FILE="$PROJECT_ROOT/.web-server.pid"
LOG_FILE="$PROJECT_ROOT/.web-server.log"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

log_info()  { echo -e "${GREEN}[INFO]${NC} $*"; }
log_warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
log_error() { echo -e "${RED}[ERROR]${NC} $*"; }
log_link()  { echo -e "${CYAN}$*${NC}"; }

# ─── 停止服务 ───
stop_server() {
    # 1. 通过 PID 文件停止
    if [[ -f "$PID_FILE" ]]; then
        local pid
        pid=$(cat "$PID_FILE")
        if kill -0 "$pid" 2>/dev/null; then
            kill "$pid" 2>/dev/null || true
            log_info "已停止 Web 服务 (PID: $pid)"
        fi
        rm -f "$PID_FILE"
    fi

    # 2. 清理占用端口的进程
    local port_pids
    port_pids=$(lsof -ti :"$PORT" 2>/dev/null || true)
    if [[ -n "$port_pids" ]]; then
        echo "$port_pids" | xargs kill 2>/dev/null || true
        log_info "已清理端口 $PORT 上的残留进程"
    fi

    sleep 1
}

# ─── 健康检查 ───
health_check() {
    local max_retries=5
    local retry=0
    while [[ $retry -lt $max_retries ]]; do
        if curl -s -o /dev/null -w "%{http_code}" "http://localhost:$PORT/" 2>/dev/null | grep -q "200"; then
            return 0
        fi
        retry=$((retry + 1))
        sleep 1
    done
    return 1
}

# ─── 前置检查 ───
preflight_check() {
    # 检查 Python3
    if ! command -v python3 &>/dev/null; then
        log_error "python3 未安装，请先安装 Python 3"
        exit 1
    fi

    # 检查项目根目录 index.html
    if [[ ! -f "$PROJECT_ROOT/index.html" ]]; then
        log_warn "项目根目录缺少 index.html，创建跳转页面..."
        cat > "$PROJECT_ROOT/index.html" <<'EOF'
<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta http-equiv="refresh" content="0;url=visualizations/index.html">
<title>KUDIG Workspace</title>
</head>
<body>
<p>正在跳转到 <a href="visualizations/index.html">KUDIG Workspace 主页</a>...</p>
</body>
</html>
EOF
    fi

    # 检查 visualizations/index.html
    if [[ ! -f "$PROJECT_ROOT/visualizations/index.html" ]]; then
        log_error "visualizations/index.html 不存在，请先创建可视化主页"
        exit 1
    fi

    # 检查 gitbook/book 构建产物
    if [[ ! -f "$PROJECT_ROOT/gitbook/book/index.html" ]]; then
        log_warn "gitbook/book/ 未构建，KUDIG 文档阅读入口将不可用"
        log_warn "如需使用，请先运行: bash gitbook/build-scripts/start.sh"
    fi
}

# ─── 主流程 ───
main() {
    # 处理 --stop 参数
    if [[ "${1:-}" == "--stop" ]]; then
        stop_server
        log_info "Web 服务已停止"
        exit 0
    fi

    echo ""
    echo -e "${CYAN}╔══════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║   KUDIG Workspace Web Server         ║${NC}"
    echo -e "${CYAN}╚══════════════════════════════════════╝${NC}"
    echo ""

    # 前置检查
    preflight_check

    # 停止已有服务
    stop_server

    # 启动服务
    log_info "启动 Web 服务 (端口: $PORT)..."
    cd "$PROJECT_ROOT"

    python3 -m http.server "$PORT" --bind 0.0.0.0 > "$LOG_FILE" 2>&1 &
    local server_pid=$!
    echo "$server_pid" > "$PID_FILE"

    # 健康检查
    if health_check; then
        log_info "Web 服务已启动 (PID: $server_pid)"
        echo ""
        log_link "  主页入口:     http://localhost:$PORT/"
        log_link "  可视化工具:   http://localhost:$PORT/visualizations/"
        log_link "  知识地图:     http://localhost:$PORT/visualizations/d3-domain-explorer.html"
        log_link "  学习方法论:   http://localhost:$PORT/visualizations/learning-methodology.html"
        log_link "  自主学习:     http://localhost:$PORT/visualizations/self-learning.html"
        if [[ -f "$PROJECT_ROOT/gitbook/book/index.html" ]]; then
            log_link "  文档阅读:     http://localhost:$PORT/gitbook/book/index.html"
        fi
        echo ""
        log_info "停止服务: bash scripts/start-web.sh --stop"
        log_info "日志文件: $LOG_FILE"
    else
        log_error "Web 服务启动失败，请检查日志: $LOG_FILE"
        cat "$LOG_FILE"
        rm -f "$PID_FILE"
        exit 1
    fi
}

main "$@"
