#!/bin/bash
# start-web.sh - 启动 KUDIG 本地预览服务
#
# 默认模式: Astro 开发服务器 (热重载, 服务 web/ 构建的知识库站点)
#   bash scripts/start-web.sh              # 默认端口 4321 (Astro 默认)
#   PORT=3000 bash scripts/start-web.sh    # 自定义端口
#
# 静态模式: 伺服 visualizations/ 等根目录独立 HTML 工具 (不依赖 Astro)
#   bash scripts/start-web.sh --static     # 默认端口 8767
#
# 构建预览: 先构建 Astro 产物再本地预览 (接近生产环境)
#   bash scripts/start-web.sh --preview
#
# 停止: bash scripts/start-web.sh --stop

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
WEB_DIR="$PROJECT_ROOT/web"
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
            log_info "已停止本地服务 (PID: $pid)"
        fi
        rm -f "$PID_FILE"
    fi

    # 2. 清理常见端口上的残留进程 (astro 4321 / static 8767)
    local port_pids
    for p in 4321 8767 "${PORT:-}"; do
        [[ -z "$p" ]] && continue
        port_pids=$(lsof -ti :"$p" 2>/dev/null || true)
        if [[ -n "$port_pids" ]]; then
            echo "$port_pids" | xargs kill 2>/dev/null || true
            log_info "已清理端口 $p 上的残留进程"
        fi
    done

    sleep 1
}

# ─── 健康检查 ───
health_check() {
    local port="$1"
    local max_retries=10
    local retry=0
    while [[ $retry -lt $max_retries ]]; do
        if curl -s -o /dev/null -w "%{http_code}" "http://localhost:$port/" 2>/dev/null | grep -qE "200|301|302"; then
            return 0
        fi
        retry=$((retry + 1))
        sleep 1
    done
    return 1
}

# ─── 前置检查: Astro 模式 ───
preflight_astro() {
    # 检查 web/ 目录
    if [[ ! -d "$WEB_DIR" ]]; then
        log_error "web/ 目录不存在，无法启动 Astro 开发服务器"
        exit 1
    fi

    # 检查 node
    if ! command -v node &>/dev/null; then
        log_error "node 未安装，请先安装 Node.js 18+ (推荐使用 nvm)"
        exit 1
    fi

    # 检查 npm
    if ! command -v npm &>/dev/null; then
        log_error "npm 未安装"
        exit 1
    fi

    # 检查依赖是否已安装
    if [[ ! -d "$WEB_DIR/node_modules" ]]; then
        log_warn "web/node_modules 不存在，自动执行 npm install..."
        (cd "$WEB_DIR" && npm install) || {
            log_error "npm install 失败，请手动进入 web/ 排查"
            exit 1
        }
    fi
}

# ─── 前置检查: 静态模式 ───
preflight_static() {
    if ! command -v python3 &>/dev/null; then
        log_error "python3 未安装（静态模式依赖 python3 -m http.server）"
        exit 1
    fi

    if [[ ! -f "$PROJECT_ROOT/visualizations/index.html" ]]; then
        log_warn "visualizations/index.html 不存在，可视化工具入口将不可用"
    fi

    # 确保根跳转页存在
    if [[ ! -f "$PROJECT_ROOT/index.html" ]]; then
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
        log_info "已创建根跳转页 index.html"
    fi
}

# ─── 启动 Astro 开发服务器 ───
start_astro() {
    local port="${PORT:-4321}"
    echo ""
    echo -e "${CYAN}╔══════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║   KUDIG Astro Dev Server             ║${NC}"
    echo -e "${CYAN}╚══════════════════════════════════════╝${NC}"
    echo ""

    preflight_astro
    stop_server

    log_info "启动 Astro 开发服务器 (端口: $port)..."
    cd "$WEB_DIR"

    # astro dev 是前台进程，后台化并记录 PID
    PORT="$port" npm run dev -- --port "$port" --host > "$LOG_FILE" 2>&1 &
    local server_pid=$!
    echo "$server_pid" > "$PID_FILE"
    cd "$PROJECT_ROOT"

    # 健康检查 (Astro 启动较慢，给予更多重试)
    if health_check "$port"; then
        log_info "Astro 开发服务器已启动 (PID: $server_pid)"
        echo ""
        log_link "  知识库主页:   http://localhost:$port/"
        echo ""
        log_info "停止服务: bash scripts/start-web.sh --stop"
        log_info "日志文件: $LOG_FILE"
        log_info "提示: Astro 提供热重载，修改 web/src/ 下文件会自动刷新"
    else
        log_error "Astro 服务器启动失败，请检查日志: $LOG_FILE"
        tail -20 "$LOG_FILE" 2>/dev/null || true
        rm -f "$PID_FILE"
        exit 1
    fi
}

# ─── 启动 Astro 构建预览 ───
start_preview() {
    local port="${PORT:-4321}"
    echo ""
    echo -e "${CYAN}╔══════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║   KUDIG Astro Build Preview           ║${NC}"
    echo -e "${CYAN}╚══════════════════════════════════════╝${NC}"
    echo ""

    preflight_astro
    stop_server

    log_info "构建 Astro 静态站点..."
    (cd "$WEB_DIR" && npm run build) || {
        log_error "Astro 构建失败，请检查"
        exit 1
    }

    log_info "启动预览服务器 (端口: $port)..."
    cd "$WEB_DIR"
    npm run preview -- --port "$port" --host > "$LOG_FILE" 2>&1 &
    local server_pid=$!
    echo "$server_pid" > "$PID_FILE"
    cd "$PROJECT_ROOT"

    if health_check "$port"; then
        log_info "预览服务器已启动 (PID: $server_pid)"
        echo ""
        log_link "  预览地址:     http://localhost:$port/"
        echo ""
        log_info "停止服务: bash scripts/start-web.sh --stop"
    else
        log_error "预览服务器启动失败，请检查日志: $LOG_FILE"
        tail -20 "$LOG_FILE" 2>/dev/null || true
        rm -f "$PID_FILE"
        exit 1
    fi
}

# ─── 启动静态文件服务器 (visualizations) ───
start_static() {
    local port="${PORT:-8767}"
    echo ""
    echo -e "${CYAN}╔══════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║   KUDIG Static Server (visualizations) ║${NC}"
    echo -e "${CYAN}╚══════════════════════════════════════╝${NC}"
    echo ""

    preflight_static
    stop_server

    log_info "启动静态文件服务器 (端口: $port)..."
    cd "$PROJECT_ROOT"

    python3 -m http.server "$port" --bind 0.0.0.0 > "$LOG_FILE" 2>&1 &
    local server_pid=$!
    echo "$server_pid" > "$PID_FILE"

    if health_check "$port"; then
        log_info "静态服务器已启动 (PID: $server_pid)"
        echo ""
        log_link "  主页入口:     http://localhost:$port/"
        log_link "  可视化工具:   http://localhost:$port/visualizations/"
        log_link "  知识地图:     http://localhost:$port/visualizations/d3-domain-explorer.html"
        log_link "  学习方法论:   http://localhost:$port/visualizations/learning-methodology.html"
        log_link "  自主学习:     http://localhost:$port/visualizations/self-learning.html"
        echo ""
        log_info "停止服务: bash scripts/start-web.sh --stop"
        log_info "日志文件: $LOG_FILE"
    else
        log_error "静态服务器启动失败，请检查日志: $LOG_FILE"
        cat "$LOG_FILE"
        rm -f "$PID_FILE"
        exit 1
    fi
}

# ─── 用法说明 ───
usage() {
    cat <<'EOF'
用法: bash scripts/start-web.sh [模式] [选项]

模式:
  (无参数)     Astro 开发服务器 (默认, 热重载, 端口 4321)
  --preview    先构建 Astro 产物再预览 (接近生产环境)
  --static     静态文件服务器, 伺服 visualizations/ 等独立 HTML 工具 (端口 8767)
  --stop       停止所有本地服务
  --help       显示此帮助

环境变量:
  PORT         自定义端口 (例: PORT=3000 bash scripts/start-web.sh)

示例:
  bash scripts/start-web.sh                       # Astro dev @ :4321
  PORT=3000 bash scripts/start-web.sh             # Astro dev @ :3000
  bash scripts/start-web.sh --preview             # 构建后预览
  bash scripts/start-web.sh --static              # 可视化工具 @ :8767
  bash scripts/start-web.sh --stop                # 停止服务
EOF
}

# ─── 主流程 ───
main() {
    case "${1:-}" in
        --stop)
            stop_server
            log_info "本地服务已停止"
            exit 0
            ;;
        --static)
            start_static
            ;;
        --preview)
            start_preview
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        ""|--dev)
            start_astro
            ;;
        *)
            log_error "未知参数: $1"
            usage
            exit 1
            ;;
    esac
}

main "$@"
