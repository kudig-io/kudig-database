#!/usr/bin/env bash
# =============================================================================
# K8s Node NotReady - Phase 2 Deep Diagnosis (Read-only, requires SSH)
# 深度诊断脚本 - SSH 登录故障节点，检查系统级组件状态
#
# Usage: bash diagnose-deep.sh <node-ip>
# Risk: NONE (read-only operations via SSH)
# Source: SKILL-NODE-001 D2.1-D2.10
# =============================================================================
set -euo pipefail

# --- 颜色定义 / Color Definitions ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# --- 全局变量 / Global Variables ---
FINDINGS=()
WARNINGS=()
ERRORS=()
SSH_OPTS="-o ConnectTimeout=10 -o StrictHostKeyChecking=no -o BatchMode=yes"

# --- 工具函数 / Utility Functions ---
print_header() {
    echo ""
    echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}${BOLD}  $1${NC}"
    echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════════════${NC}"
}

print_section() {
    echo ""
    echo -e "${CYAN}${BOLD}── $1 ──${NC}"
}

print_ok() {
    echo -e "  ${GREEN}[OK]${NC} $1"
}

print_warn() {
    echo -e "  ${YELLOW}[WARN]${NC} $1"
    WARNINGS+=("$1")
}

print_error() {
    echo -e "  ${RED}[ERROR]${NC} $1"
    ERRORS+=("$1")
}

print_info() {
    echo -e "  ${BLUE}[INFO]${NC} $1"
}

add_finding() {
    FINDINGS+=("$1")
}

# SSH 命令封装，带超时和错误处理 / SSH command wrapper with timeout and error handling
# Usage: run_ssh "command" [timeout_seconds]
run_ssh() {
    local cmd="$1"
    local timeout_sec="${2:-15}"
    local output
    local rc=0
    # 使用 timeout 命令确保 SSH 不会无限挂起
    output=$(timeout "${timeout_sec}" ssh $SSH_OPTS "$NODE_IP" "$cmd" 2>&1) || rc=$?
    echo "$output"
    # timeout 命令退出码 124 表示超时，将超时信息输出到 stderr 供上层感知
    if [[ $rc -eq 124 ]]; then
        echo -e "  ${YELLOW}[WARN]${NC} SSH command timed out after ${timeout_sec}s: $cmd" >&2
    fi
    # 诊断脚本中，远程命令的非零返回码是诊断信息的一部分，不视为脚本错误
    # 超时信息通过 stderr 输出，上层可通过检查输出感知超时状态
    return 0
}

# --- 参数验证 / Argument Validation ---
if [[ $# -lt 1 ]]; then
    echo -e "${RED}Error: Missing required argument.${NC}"
    echo ""
    echo "Usage: bash diagnose-deep.sh <node-ip>"
    echo ""
    echo "  <node-ip>  IP address of the node to diagnose (SSH must be accessible)"
    echo ""
    echo "Examples:"
    echo "  bash diagnose-deep.sh 10.0.1.100"
    echo "  bash diagnose-deep.sh 192.168.1.50"
    echo ""
    echo "Prerequisites:"
    echo "  - SSH access to the target node"
    echo "  - SSH key-based authentication configured"
    exit 1
fi

NODE_IP="$1"

# --- 检查 SSH 可达性 / Check SSH connectivity ---
print_info "Testing SSH connectivity to $NODE_IP..."
if ! ssh $SSH_OPTS "$NODE_IP" "echo 'SSH connection successful'" &>/dev/null; then
    echo -e "${RED}Error: Cannot establish SSH connection to $NODE_IP${NC}"
    echo "  Please verify:"
    echo "    1. The IP address is correct"
    echo "    2. SSH service is running on the target node"
    echo "    3. SSH key-based authentication is configured"
    echo "    4. Network connectivity exists between this host and $NODE_IP"
    exit 1
fi

print_header "K8s Node NotReady - Phase 2 Deep Diagnosis"
echo -e "  Target Node IP: ${BOLD}${NODE_IP}${NC}"
echo -e "  Timestamp:      $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "  Risk Level:     ${GREEN}NONE (read-only via SSH)${NC}"

# =============================================================================
# D2.1: 检查 kubelet 服务状态
# Check kubelet service status
# Command: ssh <node-ip> "systemctl status kubelet"
# =============================================================================
print_section "D2.1: Kubelet Service Status / kubelet 服务状态"

KUBELET_STATUS=$(run_ssh "systemctl status kubelet" 10)
echo "$KUBELET_STATUS" | head -20 | while IFS= read -r line; do
    if echo "$line" | grep -q "Active:.*running"; then
        echo -e "  ${GREEN}$line${NC}"
    elif echo "$line" | grep -q "Active:.*dead\|Active:.*failed"; then
        echo -e "  ${RED}$line${NC}"
    elif echo "$line" | grep -q "Active:.*auto-restart\|Active:.*activating"; then
        echo -e "  ${YELLOW}$line${NC}"
    else
        echo "  $line"
    fi
done

# 判断 kubelet 状态 / Determine kubelet status
if echo "$KUBELET_STATUS" | grep -q "Active: active (running)"; then
    print_ok "kubelet is running"
    add_finding "D2.1: kubelet 进程运行中，问题可能在运行时层面或网络层面"
elif echo "$KUBELET_STATUS" | grep -q "Active: inactive (dead)"; then
    print_error "kubelet is not running (inactive/dead) - RC-001"
    add_finding "D2.1: kubelet 未运行 (inactive/dead) - RC-001"
elif echo "$KUBELET_STATUS" | grep -q "Active: activating (auto-restart)"; then
    print_error "kubelet is crash-looping (auto-restart) - RC-001"
    add_finding "D2.1: kubelet 不断崩溃重启 (auto-restart) - RC-001"
elif echo "$KUBELET_STATUS" | grep -q "Active: failed"; then
    print_error "kubelet has failed to start - RC-001"
    add_finding "D2.1: kubelet 启动失败 (failed) - RC-001"
elif echo "$KUBELET_STATUS" | grep -q "not-found\|could not be found"; then
    print_error "kubelet service unit not found!"
    add_finding "D2.1: kubelet 服务未安装或 unit 文件丢失"
else
    print_warn "kubelet status is unknown"
    add_finding "D2.1: kubelet 状态未知"
fi

# =============================================================================
# D2.2: 检查 kubelet 日志
# Check kubelet logs
# Command: ssh <node-ip> "journalctl -u kubelet --since '30 minutes ago' --no-pager -n 200"
# =============================================================================
print_section "D2.2: Kubelet Logs (Last 30 min) / kubelet 日志（最近30分钟）"

KUBELET_LOGS=$(run_ssh "journalctl -u kubelet --since '30 minutes ago' --no-pager -n 200" 15)

if [[ -z "$KUBELET_LOGS" || "$KUBELET_LOGS" == *"No entries"* ]]; then
    print_warn "No kubelet logs found in the last 30 minutes"
    add_finding "D2.2: 最近30分钟无 kubelet 日志 - kubelet 可能已停止较长时间"
else
    # 显示最后20行日志，高亮错误 / Show last 20 lines, highlight errors
    print_info "Showing last 20 log entries (of up to 200):"
    echo "$KUBELET_LOGS" | tail -20 | while IFS= read -r line; do
        if echo "$line" | grep -qi "error\|fatal\|panic\|fail"; then
            echo -e "  ${RED}$line${NC}"
        elif echo "$line" | grep -qi "warn"; then
            echo -e "  ${YELLOW}$line${NC}"
        else
            echo "  $line"
        fi
    done

    # 分析日志关键词 / Analyze log keywords
    echo ""
    print_info "Log keyword analysis / 日志关键词分析:"
    
    if echo "$KUBELET_LOGS" | grep -qi "connection refused\|dial tcp.*connect: connection refused"; then
        print_error "Found 'connection refused' - 网络不通或 apiserver 不可达 (RC-006)"
        add_finding "D2.2: connection refused - RC-006"
    fi
    if echo "$KUBELET_LOGS" | grep -qi "x509: certificate has expired\|certificate signed by unknown authority"; then
        print_error "Found certificate error - 证书问题 (RC-007)"
        add_finding "D2.2: certificate error - RC-007"
    fi
    if echo "$KUBELET_LOGS" | grep -qi "PLEG is not healthy"; then
        print_error "Found 'PLEG is not healthy' - PLEG 不健康 (RC-008)"
        add_finding "D2.2: PLEG is not healthy - RC-008"
    fi
    if echo "$KUBELET_LOGS" | grep -qi "container runtime is not running\|runtime connect using default endpoints"; then
        print_error "Found container runtime failure - 容器运行时故障 (RC-002)"
        add_finding "D2.2: container runtime failure - RC-002"
    fi
    if echo "$KUBELET_LOGS" | grep -qi "failed to garbage collect.*disk\|no space left on device"; then
        print_error "Found disk space issue - 磁盘空间不足 (RC-003)"
        add_finding "D2.2: disk space issue - RC-003"
    fi
    if echo "$KUBELET_LOGS" | grep -qi "OOM\|oom_kill"; then
        print_error "Found OOM event - 内存压力 (RC-004)"
        add_finding "D2.2: OOM event - RC-004"
    fi
    if echo "$KUBELET_LOGS" | grep -qi "too many open files"; then
        print_error "Found 'too many open files' - 资源耗尽 (RC-003/RC-005)"
        add_finding "D2.2: too many open files - RC-003/RC-005"
    fi
    if echo "$KUBELET_LOGS" | grep -qi "node not found"; then
        print_error "Found 'node not found' - 节点对象可能被意外删除"
        add_finding "D2.2: node not found - 节点对象可能被删除"
    fi
    if echo "$KUBELET_LOGS" | grep -qi "failed to renew lease"; then
        print_error "Found 'failed to renew lease' - Lease 续租失败"
        add_finding "D2.2: failed to renew lease - 检查网络和 apiserver"
    fi
    if echo "$KUBELET_LOGS" | grep -qi "use of closed network connection"; then
        print_warn "Found 'use of closed network connection' - 网络连接异常 (RC-006)"
        add_finding "D2.2: closed network connection - RC-006"
    fi
    if echo "$KUBELET_LOGS" | grep -qi "shutting down gracefully"; then
        print_info "Found 'shutting down gracefully' - 节点可能正在优雅关机 (v1.28+ GracefulNodeShutdown)"
        add_finding "D2.2: graceful shutdown detected - 可能是计划内关机"
    fi
    
    # 统计错误数量 / Count error occurrences
    ERROR_COUNT=$(echo "$KUBELET_LOGS" | grep -ci "error\|fatal\|panic" || true)
    if [[ "$ERROR_COUNT" -gt 0 ]]; then
        print_info "Total error/fatal/panic entries: $ERROR_COUNT"
    else
        print_ok "No error/fatal/panic entries found in kubelet logs"
    fi
fi

# =============================================================================
# D2.3: 检查容器运行时（containerd）服务状态
# Check container runtime (containerd) service status
# Command: ssh <node-ip> "systemctl status containerd"
# =============================================================================
print_section "D2.3: Container Runtime Status / 容器运行时状态"

CONTAINERD_STATUS=$(run_ssh "systemctl status containerd" 10)
echo "$CONTAINERD_STATUS" | head -15 | while IFS= read -r line; do
    if echo "$line" | grep -q "Active:.*running"; then
        echo -e "  ${GREEN}$line${NC}"
    elif echo "$line" | grep -q "Active:.*dead\|Active:.*failed"; then
        echo -e "  ${RED}$line${NC}"
    elif echo "$line" | grep -q "Active:.*auto-restart\|Active:.*activating"; then
        echo -e "  ${YELLOW}$line${NC}"
    else
        echo "  $line"
    fi
done

if echo "$CONTAINERD_STATUS" | grep -q "Active: active (running)"; then
    print_ok "containerd is running"
elif echo "$CONTAINERD_STATUS" | grep -q "Active: inactive (dead)\|Active: failed"; then
    print_error "containerd is not running - RC-002"
    add_finding "D2.3: containerd 未运行 - RC-002，需要重启"
elif echo "$CONTAINERD_STATUS" | grep -q "Active: activating (auto-restart)"; then
    print_error "containerd is crash-looping - RC-002"
    add_finding "D2.3: containerd 不断崩溃 - RC-002"
elif echo "$CONTAINERD_STATUS" | grep -q "not-found\|could not be found"; then
    # 尝试检查 CRI-O / Try checking CRI-O
    print_info "containerd service not found, checking CRI-O..."
    CRIO_STATUS=$(run_ssh "systemctl status crio" 10)
    if echo "$CRIO_STATUS" | grep -q "Active: active (running)"; then
        print_ok "CRI-O is running (alternative runtime)"
    elif [[ -n "$CRIO_STATUS" ]]; then
        echo "$CRIO_STATUS" | head -5 | while IFS= read -r line; do
            echo "  $line"
        done
    else
        print_error "Neither containerd nor CRI-O found"
        add_finding "D2.3: 未找到容器运行时 - RC-002"
    fi
fi

# =============================================================================
# D2.4: 检查容器运行时日志
# Check container runtime logs
# Command: ssh <node-ip> "journalctl -u containerd --since '30 minutes ago' --no-pager -n 100"
# =============================================================================
print_section "D2.4: Container Runtime Logs (Last 30 min) / 容器运行时日志"

CONTAINERD_LOGS=$(run_ssh "journalctl -u containerd --since '30 minutes ago' --no-pager -n 100" 15)

if [[ -z "$CONTAINERD_LOGS" || "$CONTAINERD_LOGS" == *"No entries"* ]]; then
    print_warn "No containerd logs found in the last 30 minutes"
else
    print_info "Showing last 10 log entries:"
    echo "$CONTAINERD_LOGS" | tail -10 | while IFS= read -r line; do
        if echo "$line" | grep -qi "error\|fatal\|fail"; then
            echo -e "  ${RED}$line${NC}"
        elif echo "$line" | grep -qi "warn"; then
            echo -e "  ${YELLOW}$line${NC}"
        else
            echo "  $line"
        fi
    done

    # 分析日志关键词 / Analyze log keywords
    if echo "$CONTAINERD_LOGS" | grep -qi "failed to create shim"; then
        print_error "Found 'failed to create shim' - shim 创建失败，可能磁盘满或 PID 耗尽"
        add_finding "D2.4: failed to create shim - 磁盘满或 PID 耗尽"
    fi
    if echo "$CONTAINERD_LOGS" | grep -qi "context deadline exceeded"; then
        print_error "Found 'context deadline exceeded' - containerd 内部操作超时"
        add_finding "D2.4: context deadline exceeded - 可能磁盘 I/O 过慢"
    fi
    if echo "$CONTAINERD_LOGS" | grep -qi "no space left on device"; then
        print_error "Found 'no space left on device' - 磁盘空间不足 (RC-003)"
        add_finding "D2.4: no space left on device - RC-003"
    fi
    if echo "$CONTAINERD_LOGS" | grep -qi "plugin.*error"; then
        print_warn "Found plugin error in containerd logs"
        add_finding "D2.4: containerd plugin error detected"
    fi
fi

# =============================================================================
# D2.5: 检查系统资源压力
# Check system resource pressure
# Commands: df -h, free -m, PID count, df -i
# =============================================================================
print_section "D2.5: System Resource Pressure / 系统资源压力"

# --- 磁盘使用 / Disk Usage ---
echo -e "  ${BOLD}Disk Usage / 磁盘使用:${NC}"
DISK_OUTPUT=$(run_ssh "df -h / /var/lib/kubelet /var/lib/containerd /var/log 2>/dev/null || df -h /" 10)
echo "$DISK_OUTPUT" | while IFS= read -r line; do
    if echo "$line" | grep -q "Filesystem"; then
        echo "  $line"
    else
        # 提取使用率百分比 / Extract usage percentage
        USE_PCT=$(echo "$line" | awk '{print $5}' | tr -d '%')
        if [[ -n "$USE_PCT" && "$USE_PCT" =~ ^[0-9]+$ ]]; then
            if [[ $USE_PCT -ge 85 ]]; then
                echo -e "  ${RED}$line${NC}"
            elif [[ $USE_PCT -ge 70 ]]; then
                echo -e "  ${YELLOW}$line${NC}"
            else
                echo -e "  ${GREEN}$line${NC}"
            fi
        else
            echo "  $line"
        fi
    fi
done

# 检查磁盘阈值 / Check disk thresholds
DISK_CRITICAL=$(run_ssh "df -h / /var/lib/kubelet /var/lib/containerd /var/log 2>/dev/null | awk 'NR>1{gsub(/%/,\"\",\$5); if(\$5>=85) print \$6, \$5\"%\"}'" 10)
if [[ -n "$DISK_CRITICAL" ]]; then
    while IFS= read -r line; do
        print_error "Disk usage >= 85%: $line (RC-003)"
        add_finding "D2.5: 磁盘使用率超阈值 $line - RC-003"
    done <<< "$DISK_CRITICAL"
fi

echo ""

# --- 内存使用 / Memory Usage ---
echo -e "  ${BOLD}Memory Usage / 内存使用:${NC}"
MEMORY_OUTPUT=$(run_ssh "free -m" 10)
echo "$MEMORY_OUTPUT" | while IFS= read -r line; do
    echo "  $line"
done

AVAIL_MEM=$(echo "$MEMORY_OUTPUT" | awk '/Mem:/{print $7}')
if [[ -n "$AVAIL_MEM" && "$AVAIL_MEM" =~ ^[0-9]+$ ]]; then
    if [[ $AVAIL_MEM -lt 100 ]]; then
        print_error "Available memory: ${AVAIL_MEM}Mi (< 100Mi threshold) - RC-004"
        add_finding "D2.5: 可用内存 ${AVAIL_MEM}Mi < 100Mi - RC-004"
    elif [[ $AVAIL_MEM -lt 256 ]]; then
        print_warn "Available memory: ${AVAIL_MEM}Mi (low)"
    else
        print_ok "Available memory: ${AVAIL_MEM}Mi"
    fi
fi

echo ""

# --- PID 使用 / PID Usage ---
echo -e "  ${BOLD}PID Usage / PID 使用:${NC}"
PID_OUTPUT=$(run_ssh "echo 'Current PIDs:' && ps aux --no-heading | wc -l && echo 'Max PIDs:' && cat /proc/sys/kernel/pid_max" 10)
echo "$PID_OUTPUT" | while IFS= read -r line; do
    echo "  $line"
done

CURRENT_PIDS=$(echo "$PID_OUTPUT" | awk '/Current PIDs:/{getline; print}')
MAX_PIDS=$(echo "$PID_OUTPUT" | awk '/Max PIDs:/{getline; print}')
if [[ -n "$CURRENT_PIDS" && -n "$MAX_PIDS" && "$CURRENT_PIDS" =~ ^[0-9]+$ && "$MAX_PIDS" =~ ^[0-9]+$ ]]; then
    PID_USAGE_PCT=$((CURRENT_PIDS * 100 / MAX_PIDS))
    if [[ $PID_USAGE_PCT -ge 90 ]]; then
        print_error "PID usage: ${PID_USAGE_PCT}% ($CURRENT_PIDS / $MAX_PIDS) - RC-005"
        add_finding "D2.5: PID 使用率 ${PID_USAGE_PCT}% - RC-005"
    elif [[ $PID_USAGE_PCT -ge 70 ]]; then
        print_warn "PID usage: ${PID_USAGE_PCT}% ($CURRENT_PIDS / $MAX_PIDS)"
    else
        print_ok "PID usage: ${PID_USAGE_PCT}% ($CURRENT_PIDS / $MAX_PIDS)"
    fi
fi

echo ""

# --- inode 使用 / inode Usage ---
echo -e "  ${BOLD}Inode Usage / inode 使用:${NC}"
INODE_OUTPUT=$(run_ssh "df -i / /var/lib/kubelet /var/lib/containerd 2>/dev/null || df -i /" 10)
echo "$INODE_OUTPUT" | while IFS= read -r line; do
    if echo "$line" | grep -q "Filesystem"; then
        echo "  $line"
    else
        USE_PCT=$(echo "$line" | awk '{print $5}' | tr -d '%')
        if [[ -n "$USE_PCT" && "$USE_PCT" =~ ^[0-9]+$ ]]; then
            if [[ $USE_PCT -ge 90 ]]; then
                echo -e "  ${RED}$line${NC}"
            elif [[ $USE_PCT -ge 70 ]]; then
                echo -e "  ${YELLOW}$line${NC}"
            else
                echo -e "  ${GREEN}$line${NC}"
            fi
        else
            echo "  $line"
        fi
    fi
done

INODE_CRITICAL=$(run_ssh "df -i / /var/lib/kubelet /var/lib/containerd 2>/dev/null | awk 'NR>1{gsub(/%/,\"\",\$5); if(\$5>=90) print \$6, \$5\"%\"}'" 10)
if [[ -n "$INODE_CRITICAL" ]]; then
    while IFS= read -r line; do
        print_error "Inode usage >= 90%: $line (RC-003)"
        add_finding "D2.5: inode 使用率超阈值 $line - RC-003"
    done <<< "$INODE_CRITICAL"
fi

# =============================================================================
# D2.6: 检查 PLEG 健康状态
# Check PLEG health status
# Commands: grep PLEG from kubelet logs, curl healthz
# =============================================================================
print_section "D2.6: PLEG Health / PLEG 健康状态"

# 检查 kubelet 日志中的 PLEG 相关信息
PLEG_LOGS=$(run_ssh "journalctl -u kubelet --since '30 minutes ago' --no-pager | grep -i 'PLEG\|pleg'" 10)
if [[ -n "$PLEG_LOGS" ]]; then
    PLEG_COUNT=$(echo "$PLEG_LOGS" | grep -ci "PLEG is not healthy" || true)
    if [[ "$PLEG_COUNT" -gt 0 ]]; then
        print_error "PLEG is not healthy (found $PLEG_COUNT occurrences) - RC-008"
        add_finding "D2.6: PLEG is not healthy ($PLEG_COUNT 次) - RC-008"
    fi
    if echo "$PLEG_LOGS" | grep -qi "Unable to retrieve pods"; then
        print_error "GenericPLEG: Unable to retrieve pods - container runtime 查询失败"
        add_finding "D2.6: PLEG unable to retrieve pods - 关联 RC-002"
    fi
    print_info "Recent PLEG log entries:"
    echo "$PLEG_LOGS" | tail -5 | while IFS= read -r line; do
        echo -e "  ${YELLOW}$line${NC}"
    done
else
    print_ok "No PLEG issues found in recent kubelet logs"
fi

# 检查 kubelet healthz 端点
echo ""
HEALTHZ_OUTPUT=$(run_ssh "curl -sk --max-time 5 https://localhost:10250/healthz 2>&1" 10)
if [[ "$HEALTHZ_OUTPUT" == "ok" ]]; then
    print_ok "kubelet healthz endpoint: ok"
else
    print_warn "kubelet healthz endpoint: $HEALTHZ_OUTPUT"
    if [[ "$HEALTHZ_OUTPUT" != *"Connection refused"* && "$HEALTHZ_OUTPUT" != *"curl"* ]]; then
        add_finding "D2.6: kubelet healthz 返回非 ok: $HEALTHZ_OUTPUT"
    fi
fi

# =============================================================================
# D2.7: 检查节点到 apiserver 的网络连通性
# Check network connectivity to apiserver
# Commands: read kubelet.conf, nc/curl test
# =============================================================================
print_section "D2.7: Network to Apiserver / 到 apiserver 的网络连通性"

# 获取 apiserver 地址
APISERVER_LINE=$(run_ssh "cat /etc/kubernetes/kubelet.conf 2>/dev/null | grep server || cat /var/lib/kubelet/kubeconfig 2>/dev/null | grep server || echo 'config not found'" 10)
print_info "Apiserver config: $APISERVER_LINE"

# 提取 apiserver IP 和端口
APISERVER_URL=$(echo "$APISERVER_LINE" | sed -n 's|.*\(https\?://[^ "]*\).*|&1|p' | head -1 || true)
if [[ -n "$APISERVER_URL" ]]; then
    APISERVER_HOST=$(echo "$APISERVER_URL" | sed 's|https\?://||' | sed 's|:.*||')
    APISERVER_PORT=$(echo "$APISERVER_URL" | sed 's|.*:\([0-9]*\).*|\1|' || echo "6443")
    
    print_info "Testing TCP connectivity to $APISERVER_HOST:$APISERVER_PORT..."
    
    # TCP 连通性测试 (使用 bash 内置 /dev/tcp 实现跨平台兼容)
    TCP_RESULT=$(run_ssh "bash -c '</dev/tcp/$APISERVER_HOST/$APISERVER_PORT' 2>&1 && echo 'TCP_OK' || echo 'TCP_FAILED'" 15)
    if echo "$TCP_RESULT" | grep -q "TCP_OK"; then
        print_ok "TCP connection to apiserver successful"
        
        # TLS/HTTPS 测试
        CURL_RESULT=$(run_ssh "curl -sk --max-time 5 ${APISERVER_URL}/healthz 2>&1" 10)
        if [[ "$CURL_RESULT" == "ok" ]]; then
            print_ok "Apiserver healthz: ok (TLS connection successful)"
        elif echo "$CURL_RESULT" | grep -qi "TLS\|SSL\|certificate\|x509"; then
            print_error "TLS handshake failed - 证书问题 (RC-007)"
            add_finding "D2.7: TLS 握手失败 - RC-007"
        elif [[ -n "$CURL_RESULT" ]]; then
            print_warn "Apiserver healthz response: $CURL_RESULT"
        fi
    else
        print_error "TCP connection to apiserver failed - 网络分区 (RC-006)"
        print_info "Check: firewall rules, routing, switches"
        add_finding "D2.7: TCP 连接 apiserver 失败 - RC-006"
    fi
else
    print_warn "Could not extract apiserver URL from kubelet config"
fi

# =============================================================================
# D2.8: 检查 kubelet 证书有效期
# Check kubelet certificate validity
# Commands: openssl x509 on client and serving certs
# =============================================================================
print_section "D2.8: Certificate Validity / 证书有效期"

# 检查 kubelet 客户端证书
echo -e "  ${BOLD}Kubelet Client Certificate:${NC}"
CLIENT_CERT=$(run_ssh "openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates -subject 2>/dev/null || echo 'Certificate file not found or not readable'" 10)
echo "$CLIENT_CERT" | while IFS= read -r line; do
    if echo "$line" | grep -qi "not found\|not readable"; then
        echo -e "  ${RED}$line${NC}"
    elif echo "$line" | grep -q "notAfter"; then
        echo -e "  ${YELLOW}$line${NC}"
    else
        echo "  $line"
    fi
done

if echo "$CLIENT_CERT" | grep -q "notAfter"; then
    CERT_EXPIRY=$(echo "$CLIENT_CERT" | grep "notAfter" | sed 's/notAfter=//')
    # Cross-platform date parsing (macOS BSD date vs GNU date)
    # Strip timezone suffix if present for consistent parsing
    CERT_EXPIRY_CLEAN="${CERT_EXPIRY// GMT/}"
    CERT_EXPIRY_CLEAN="${CERT_EXPIRY_CLEAN// UTC/}"
    # 尝试多种日期格式解析 / Try multiple date formats for cross-platform compatibility
    CERT_EPOCH=$(date -jf "%b %d %H:%M:%S %Y %Z" "$CERT_EXPIRY" +%s 2>/dev/null || \
                 date -jf "%b %d %H:%M:%S %Y" "$CERT_EXPIRY_CLEAN" +%s 2>/dev/null || \
                 date -d "$CERT_EXPIRY_CLEAN" +%s 2>/dev/null || \
                 echo "")
    NOW_EPOCH=$(date +%s)
    
    if [[ -n "$CERT_EPOCH" ]]; then
        DAYS_LEFT=$(( (CERT_EPOCH - NOW_EPOCH) / 86400 ))
        if [[ $DAYS_LEFT -lt 0 ]]; then
            print_error "Client certificate EXPIRED ${DAYS_LEFT#-} days ago - RC-007"
            add_finding "D2.8: kubelet 客户端证书已过期 ${DAYS_LEFT#-} 天 - RC-007"
        elif [[ $DAYS_LEFT -lt 7 ]]; then
            print_warn "Client certificate expires in $DAYS_LEFT days"
            add_finding "D2.8: kubelet 客户端证书将在 ${DAYS_LEFT} 天后过期 - 建议预防性轮转"
        else
            print_ok "Client certificate valid for $DAYS_LEFT more days"
        fi
    fi
elif echo "$CLIENT_CERT" | grep -qi "not found"; then
    print_error "Client certificate file not found - RC-007"
    add_finding "D2.8: kubelet 客户端证书文件不存在 - RC-007"
fi

echo ""

# 检查 kubelet serving 证书
echo -e "  ${BOLD}Kubelet Serving Certificate:${NC}"
SERVING_CERT=$(run_ssh "openssl x509 -in /var/lib/kubelet/pki/kubelet.crt -noout -dates -subject 2>/dev/null || echo 'Certificate file not found or not readable'" 10)
echo "$SERVING_CERT" | while IFS= read -r line; do
    if echo "$line" | grep -qi "not found\|not readable"; then
        echo -e "  ${YELLOW}$line${NC}"
    else
        echo "  $line"
    fi
done

if echo "$SERVING_CERT" | grep -q "notAfter"; then
    CERT_EXPIRY=$(echo "$SERVING_CERT" | grep "notAfter" | sed 's/notAfter=//')
    # Cross-platform date parsing (macOS BSD date vs GNU date)
    CERT_EXPIRY_CLEAN="${CERT_EXPIRY// GMT/}"
    CERT_EXPIRY_CLEAN="${CERT_EXPIRY_CLEAN// UTC/}"
    CERT_EPOCH=$(date -jf "%b %d %H:%M:%S %Y %Z" "$CERT_EXPIRY" +%s 2>/dev/null || \
                 date -jf "%b %d %H:%M:%S %Y" "$CERT_EXPIRY_CLEAN" +%s 2>/dev/null || \
                 date -d "$CERT_EXPIRY_CLEAN" +%s 2>/dev/null || \
                 echo "")
    NOW_EPOCH=$(date +%s)
    
    if [[ -n "$CERT_EPOCH" ]]; then
        DAYS_LEFT=$(( (CERT_EPOCH - NOW_EPOCH) / 86400 ))
        if [[ $DAYS_LEFT -lt 0 ]]; then
            print_error "Serving certificate EXPIRED ${DAYS_LEFT#-} days ago"
            add_finding "D2.8: kubelet serving 证书已过期"
        elif [[ $DAYS_LEFT -lt 7 ]]; then
            print_warn "Serving certificate expires in $DAYS_LEFT days"
        else
            print_ok "Serving certificate valid for $DAYS_LEFT more days"
        fi
    fi
fi

# =============================================================================
# D2.9: 检查内核日志
# Check kernel logs
# Command: ssh <node-ip> "dmesg -T | tail -50"
# =============================================================================
print_section "D2.9: Kernel Logs (dmesg) / 内核日志"

DMESG_OUTPUT=$(run_ssh "dmesg -T | tail -50" 10)

if [[ -z "$DMESG_OUTPUT" ]]; then
    print_warn "Could not retrieve kernel logs"
else
    # 显示最近的关键条目 / Show recent critical entries
    CRITICAL_DMESG=$(echo "$DMESG_OUTPUT" | grep -iE "Out of memory|oom_kill|Hardware Error|MCE|I/O error|device not responding|soft lockup|nf_conntrack.*table full|EXT4-fs error|XFS error|panic|BUG|oops" || true)
    
    if [[ -n "$CRITICAL_DMESG" ]]; then
        print_error "Critical kernel log entries found:"
        echo "$CRITICAL_DMESG" | while IFS= read -r line; do
            echo -e "    ${RED}$line${NC}"
        done
        
        if echo "$CRITICAL_DMESG" | grep -qi "Out of memory\|oom_kill"; then
            add_finding "D2.9: OOM Killer 触发 - RC-004"
            # 显示被杀的进程信息
            OOM_DETAILS=$(echo "$DMESG_OUTPUT" | grep -i "Killed process" | tail -3 || true)
            if [[ -n "$OOM_DETAILS" ]]; then
                print_info "OOM killed processes:"
                echo "$OOM_DETAILS" | while IFS= read -r line; do
                    echo -e "    ${RED}$line${NC}"
                done
            fi
        fi
        if echo "$CRITICAL_DMESG" | grep -qi "Hardware Error\|MCE"; then
            add_finding "D2.9: 硬件故障 (Hardware Error/MCE) - RC-009"
        fi
        if echo "$CRITICAL_DMESG" | grep -qi "I/O error\|device not responding"; then
            add_finding "D2.9: 磁盘硬件故障 (I/O error) - RC-009"
        fi
        if echo "$CRITICAL_DMESG" | grep -qi "soft lockup"; then
            add_finding "D2.9: CPU 软锁死 (soft lockup) - RC-009"
        fi
        if echo "$CRITICAL_DMESG" | grep -qi "nf_conntrack.*table full"; then
            add_finding "D2.9: conntrack 表满 - RC-006 变种"
        fi
        if echo "$CRITICAL_DMESG" | grep -qi "EXT4-fs error\|XFS error"; then
            add_finding "D2.9: 文件系统错误 - RC-009"
        fi
    else
        print_ok "No critical kernel log entries found"
    fi
    
    # 显示最后5行日志 / Show last 5 log lines
    print_info "Last 5 dmesg entries:"
    echo "$DMESG_OUTPUT" | tail -5 | while IFS= read -r line; do
        echo "  $line"
    done
fi

# =============================================================================
# D2.10: 检查 NTP/时间同步
# Check NTP/time synchronization
# Commands: timedatectl, chronyc/ntpq, date
# =============================================================================
print_section "D2.10: Time Synchronization / 时间同步"

# 检查时间同步状态
TIMEDATECTL_OUTPUT=$(run_ssh "timedatectl status 2>/dev/null || echo 'timedatectl not available'" 10)
if [[ "$TIMEDATECTL_OUTPUT" != *"not available"* ]]; then
    echo "$TIMEDATECTL_OUTPUT" | while IFS= read -r line; do
        if echo "$line" | grep -qi "synchronized: yes"; then
            echo -e "  ${GREEN}$line${NC}"
        elif echo "$line" | grep -qi "synchronized: no"; then
            echo -e "  ${RED}$line${NC}"
        else
            echo "  $line"
        fi
    done
    
    if echo "$TIMEDATECTL_OUTPUT" | grep -qi "synchronized: no"; then
        print_error "System clock is NOT synchronized - RC-010"
        add_finding "D2.10: 时间未同步 - RC-010"
    elif echo "$TIMEDATECTL_OUTPUT" | grep -qi "synchronized: yes"; then
        print_ok "System clock is synchronized"
    fi
else
    print_warn "timedatectl not available on this node"
fi

echo ""

# 检查 chrony/ntpd 状态
NTP_OUTPUT=$(run_ssh "chronyc tracking 2>/dev/null || ntpq -p 2>/dev/null || echo 'No NTP service found'" 10)
if echo "$NTP_OUTPUT" | grep -qi "No NTP service found"; then
    print_warn "No NTP service (chrony/ntpd) found"
else
    print_info "NTP service status:"
    echo "$NTP_OUTPUT" | head -10 | while IFS= read -r line; do
        echo "  $line"
    done
fi

echo ""

# 对比节点时间与本地时间
NODE_TIME=$(run_ssh "date -u '+%Y-%m-%d %H:%M:%S'" 10)
LOCAL_TIME=$(date -u '+%Y-%m-%d %H:%M:%S')
print_info "Node time (UTC):  $NODE_TIME"
print_info "Local time (UTC): $LOCAL_TIME"

# 计算时间偏差（简单比较秒数）
NODE_EPOCH=$(run_ssh "date -u +%s" 5)
LOCAL_EPOCH=$(date -u +%s)
if [[ -n "$NODE_EPOCH" && "$NODE_EPOCH" =~ ^[0-9]+$ ]]; then
    TIME_DIFF=$(( LOCAL_EPOCH - NODE_EPOCH ))
    # 取绝对值
    if [[ $TIME_DIFF -lt 0 ]]; then
        TIME_DIFF=$(( -TIME_DIFF ))
    fi
    
    if [[ $TIME_DIFF -gt 60 ]]; then
        print_error "Time drift: ${TIME_DIFF}s (> 60s) - 严重偏差，几乎确定导致 TLS 失败 (RC-010 + RC-007)"
        add_finding "D2.10: 时间偏差 ${TIME_DIFF}s > 60s - RC-010 + RC-007"
    elif [[ $TIME_DIFF -gt 5 ]]; then
        print_warn "Time drift: ${TIME_DIFF}s (> 5s) - 可能导致证书验证失败和 Lease 续租异常 (RC-010)"
        add_finding "D2.10: 时间偏差 ${TIME_DIFF}s > 5s - RC-010"
    else
        print_ok "Time drift: ${TIME_DIFF}s (within acceptable range)"
    fi
fi

# =============================================================================
# 诊断总结 / Diagnosis Summary
# =============================================================================
print_header "Phase 2 Deep Diagnosis Summary / 深度诊断总结"

echo -e "  Node IP:   ${BOLD}${NODE_IP}${NC}"
echo -e "  Timestamp: $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo ""

# 输出发现 / Print findings
if [[ ${#FINDINGS[@]} -gt 0 ]]; then
    echo -e "  ${BOLD}Findings / 发现 (${#FINDINGS[@]}):${NC}"
    for i in "${!FINDINGS[@]}"; do
        echo -e "    $((i+1)). ${FINDINGS[$i]}"
    done
    echo ""
fi

# 输出错误 / Print errors
if [[ ${#ERRORS[@]} -gt 0 ]]; then
    echo -e "  ${RED}${BOLD}Errors (${#ERRORS[@]}):${NC}"
    for err in "${ERRORS[@]}"; do
        echo -e "    ${RED}- $err${NC}"
    done
    echo ""
fi

# 输出告警 / Print warnings
if [[ ${#WARNINGS[@]} -gt 0 ]]; then
    echo -e "  ${YELLOW}${BOLD}Warnings (${#WARNINGS[@]}):${NC}"
    for warn in "${WARNINGS[@]}"; do
        echo -e "    ${YELLOW}- $warn${NC}"
    done
    echo ""
fi

# 根因推断 / Root cause inference
echo -e "  ${BOLD}Suspected Root Causes / 可能根因:${NC}"
declare -A RC_HITS
for finding in "${FINDINGS[@]}"; do
    for rc in RC-001 RC-002 RC-003 RC-004 RC-005 RC-006 RC-007 RC-008 RC-009 RC-010 RC-011 RC-012; do
        if echo "$finding" | grep -q "$rc"; then
            RC_HITS[$rc]=$(( ${RC_HITS[$rc]:-0} + 1 ))
        fi
    done
done

if [[ ${#RC_HITS[@]} -gt 0 ]]; then
    for rc in $(echo "${!RC_HITS[@]}" | tr ' ' '\n' | sort); do
        case "$rc" in
            RC-001) DESC="kubelet 进程崩溃或未运行" ;;
            RC-002) DESC="容器运行时（containerd）异常" ;;
            RC-003) DESC="节点磁盘空间耗尽（DiskPressure）" ;;
            RC-004) DESC="节点内存耗尽（MemoryPressure）" ;;
            RC-005) DESC="节点 PID 耗尽（PIDPressure）" ;;
            RC-006) DESC="节点与 apiserver 网络不通" ;;
            RC-007) DESC="kubelet 客户端证书过期" ;;
            RC-008) DESC="PLEG 不健康" ;;
            RC-009) DESC="内核故障/硬件异常" ;;
            RC-010) DESC="NTP 时间不同步" ;;
            RC-011) DESC="CNI 插件异常" ;;
            RC-012) DESC="节点被手动 cordon/drain" ;;
            *) DESC="Unknown" ;;
        esac
        echo -e "    ${YELLOW}$rc${NC}: $DESC (evidence count: ${RC_HITS[$rc]})"
    done
else
    echo "    No specific root cause identified. Consider Phase 3 active probing."
fi

echo ""

# 建议修复操作 / Recommended remediation
echo -e "  ${BOLD}Recommended Next Steps / 建议下一步:${NC}"
echo "    1. Run resource check:     bash check-resources.sh $NODE_IP"
echo "    2. Verify after fix:       bash verify-node.sh <node-name>"
if [[ -n "${RC_HITS[RC-003]:-}" ]]; then
    echo -e "    ${YELLOW}3. Disk cleanup (RC-003):   bash cleanup-disk.sh $NODE_IP${NC}"
fi

echo ""
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════════════${NC}"
echo -e "  Phase 2 Deep Diagnosis Complete - $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════════════${NC}"
