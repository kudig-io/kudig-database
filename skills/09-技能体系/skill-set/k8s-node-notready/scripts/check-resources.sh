#!/usr/bin/env bash
# =============================================================================
# K8s Node NotReady - Resource Pressure Check
# 资源压力检查脚本 - 检查节点磁盘/内存/PID/inode 使用情况
#
# Usage: bash check-resources.sh <node-ip>
# Risk: NONE (read-only)
# Source: SKILL-NODE-001 D2.5
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

# --- 阈值定义 / Threshold Definitions ---
DISK_WARN_PCT=70      # 磁盘告警阈值 / Disk warning threshold
DISK_FAIL_PCT=85      # 磁盘错误阈值（kubelet 默认 imagefs.available 驱逐阈值为 15%）
MEMORY_FAIL_MI=100    # 内存错误阈值（kubelet 默认 memory.available 驱逐阈值）
MEMORY_WARN_MI=256    # 内存告警阈值
PID_WARN_PCT=70       # PID 告警阈值
PID_FAIL_PCT=90       # PID 错误阈值
INODE_WARN_PCT=70     # inode 告警阈值
INODE_FAIL_PCT=90     # inode 错误阈值

# --- 统计变量 / Statistics Variables ---
PASS_COUNT=0
WARN_COUNT=0
FAIL_COUNT=0
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

print_pass() {
    echo -e "  ${GREEN}[PASS]${NC} $1"
    PASS_COUNT=$((PASS_COUNT + 1))
}

print_warn_result() {
    echo -e "  ${YELLOW}[WARN]${NC} $1"
    WARN_COUNT=$((WARN_COUNT + 1))
}

print_fail() {
    echo -e "  ${RED}[FAIL]${NC} $1"
    FAIL_COUNT=$((FAIL_COUNT + 1))
}

print_info() {
    echo -e "  ${BLUE}[INFO]${NC} $1"
}

# SSH 命令封装 / SSH command wrapper
run_ssh() {
    ssh $SSH_OPTS "$NODE_IP" "$1" 2>&1 || true
}

# --- 参数验证 / Argument Validation ---
if [[ $# -lt 1 ]]; then
    echo -e "${RED}Error: Missing required argument.${NC}"
    echo ""
    echo "Usage: bash check-resources.sh <node-ip>"
    echo ""
    echo "  <node-ip>  IP address of the node to check (SSH must be accessible)"
    echo ""
    echo "Examples:"
    echo "  bash check-resources.sh 10.0.1.100"
    echo "  bash check-resources.sh 192.168.1.50"
    echo ""
    echo "Thresholds:"
    echo "  Disk:   WARN >= ${DISK_WARN_PCT}%, FAIL >= ${DISK_FAIL_PCT}%"
    echo "  Memory: WARN < ${MEMORY_WARN_MI}Mi, FAIL < ${MEMORY_FAIL_MI}Mi"
    echo "  PID:    WARN >= ${PID_WARN_PCT}%, FAIL >= ${PID_FAIL_PCT}%"
    echo "  Inode:  WARN >= ${INODE_WARN_PCT}%, FAIL >= ${INODE_FAIL_PCT}%"
    exit 1
fi

NODE_IP="$1"

# --- 检查 SSH 可达性 / Check SSH connectivity ---
if ! ssh $SSH_OPTS "$NODE_IP" "echo 'ok'" &>/dev/null; then
    echo -e "${RED}Error: Cannot establish SSH connection to $NODE_IP${NC}"
    exit 1
fi

print_header "K8s Node NotReady - Resource Pressure Check"
echo -e "  Target Node IP: ${BOLD}${NODE_IP}${NC}"
echo -e "  Timestamp:      $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "  Risk Level:     ${GREEN}NONE (read-only)${NC}"

# =============================================================================
# 1. 磁盘使用检查 / Disk Usage Check
# 检查关键路径: /, /var/lib/kubelet, /var/lib/containerd, /var/log
# Thresholds: > 85% FAIL (kubelet imagefs.available default 15%), > 70% WARN
# =============================================================================
print_section "1. Disk Usage / 磁盘使用 (df -h)"
echo -e "  Thresholds: ${RED}FAIL >= ${DISK_FAIL_PCT}%${NC} | ${YELLOW}WARN >= ${DISK_WARN_PCT}%${NC} | ${GREEN}PASS < ${DISK_WARN_PCT}%${NC}"
echo ""

DISK_OUTPUT=$(run_ssh "df -h / /var/lib/kubelet /var/lib/containerd /var/log 2>/dev/null || df -h /")
DISK_HAS_ISSUE=false

# 打印表头 / Print header
echo -e "  ${BOLD}Filesystem                      Size  Used  Avail Use%  Mounted on        Status${NC}"
echo "  ──────────────────────────────────────────────────────────────────────────────────"

# 解析每行数据 / Parse each line
echo "$DISK_OUTPUT" | awk 'NR>1' | sort -u | while IFS= read -r line; do
    [[ -z "$line" ]] && continue
    USE_PCT=$(echo "$line" | awk '{print $5}' | tr -d '%')
    MOUNT=$(echo "$line" | awk '{print $6}')
    
    if [[ -n "$USE_PCT" && "$USE_PCT" =~ ^[0-9]+$ ]]; then
        if [[ $USE_PCT -ge $DISK_FAIL_PCT ]]; then
            printf "  ${RED}%-40s %s${NC}\n" "$line" "[FAIL]"
        elif [[ $USE_PCT -ge $DISK_WARN_PCT ]]; then
            printf "  ${YELLOW}%-40s %s${NC}\n" "$line" "[WARN]"
        else
            printf "  ${GREEN}%-40s %s${NC}\n" "$line" "[PASS]"
        fi
    else
        echo "  $line"
    fi
done

# 统计磁盘结果 / Summarize disk results
DISK_FAIL_MOUNTS=$(run_ssh "df -h / /var/lib/kubelet /var/lib/containerd /var/log 2>/dev/null | awk 'NR>1{gsub(/%/,\"\",\$5); if(\$5>=$DISK_FAIL_PCT) print \$6\"(\"100-\$5\"%free)\"}'" || true)
DISK_WARN_MOUNTS=$(run_ssh "df -h / /var/lib/kubelet /var/lib/containerd /var/log 2>/dev/null | awk 'NR>1{gsub(/%/,\"\",\$5); if(\$5>=$DISK_WARN_PCT && \$5<$DISK_FAIL_PCT) print \$6\"(\"100-\$5\"%free)\"}'" || true)

echo ""
if [[ -n "$DISK_FAIL_MOUNTS" ]]; then
    print_fail "Disk usage critical (>= ${DISK_FAIL_PCT}%): $DISK_FAIL_MOUNTS"
elif [[ -n "$DISK_WARN_MOUNTS" ]]; then
    print_warn_result "Disk usage elevated (>= ${DISK_WARN_PCT}%): $DISK_WARN_MOUNTS"
else
    print_pass "All disk partitions within safe levels"
fi

# 检查大文件（常见误诊：日志文件未轮转）
print_info "Top 5 largest directories under /var/log/pods/ (potential log accumulation):"
LARGE_DIRS=$(run_ssh "du -sh /var/log/pods/* 2>/dev/null | sort -rh | head -5" || true)
if [[ -n "$LARGE_DIRS" ]]; then
    echo "$LARGE_DIRS" | while IFS= read -r line; do
        SIZE=$(echo "$line" | awk '{print $1}')
        echo "    $line"
    done
else
    echo "    (No pod logs found or path does not exist)"
fi

# =============================================================================
# 2. 内存使用检查 / Memory Usage Check
# Thresholds: < 100Mi FAIL (kubelet default memory.available), < 256Mi WARN
# =============================================================================
print_section "2. Memory Usage / 内存使用 (free -m)"
echo -e "  Thresholds: ${RED}FAIL < ${MEMORY_FAIL_MI}Mi available${NC} | ${YELLOW}WARN < ${MEMORY_WARN_MI}Mi${NC} | ${GREEN}PASS >= ${MEMORY_WARN_MI}Mi${NC}"
echo ""

MEMORY_OUTPUT=$(run_ssh "free -m")
echo "$MEMORY_OUTPUT" | while IFS= read -r line; do
    echo "  $line"
done

# 解析内存数据 / Parse memory data
TOTAL_MEM=$(echo "$MEMORY_OUTPUT" | awk '/Mem:/{print $2}')
USED_MEM=$(echo "$MEMORY_OUTPUT" | awk '/Mem:/{print $3}')
FREE_MEM=$(echo "$MEMORY_OUTPUT" | awk '/Mem:/{print $4}')
AVAIL_MEM=$(echo "$MEMORY_OUTPUT" | awk '/Mem:/{print $7}')
SWAP_TOTAL=$(echo "$MEMORY_OUTPUT" | awk '/Swap:/{print $2}')
SWAP_USED=$(echo "$MEMORY_OUTPUT" | awk '/Swap:/{print $3}')

echo ""
if [[ -n "$AVAIL_MEM" && "$AVAIL_MEM" =~ ^[0-9]+$ ]]; then
    MEM_USED_PCT=$((USED_MEM * 100 / TOTAL_MEM))
    
    echo -e "  ${BOLD}Memory Summary:${NC}"
    echo -e "    Total:     ${TOTAL_MEM}Mi"
    echo -e "    Used:      ${USED_MEM}Mi (${MEM_USED_PCT}%)"
    echo -e "    Available: ${AVAIL_MEM}Mi"
    echo ""
    
    if [[ $AVAIL_MEM -lt $MEMORY_FAIL_MI ]]; then
        print_fail "Available memory: ${AVAIL_MEM}Mi (< ${MEMORY_FAIL_MI}Mi threshold) - RC-004"
    elif [[ $AVAIL_MEM -lt $MEMORY_WARN_MI ]]; then
        print_warn_result "Available memory: ${AVAIL_MEM}Mi (< ${MEMORY_WARN_MI}Mi, approaching threshold)"
    else
        print_pass "Available memory: ${AVAIL_MEM}Mi (healthy)"
    fi
    
    # 检查 swap 使用 / Check swap usage
    if [[ -n "$SWAP_TOTAL" && "$SWAP_TOTAL" =~ ^[0-9]+$ && "$SWAP_TOTAL" -gt 0 ]]; then
        SWAP_USED_PCT=$((SWAP_USED * 100 / SWAP_TOTAL))
        if [[ $SWAP_USED_PCT -gt 50 ]]; then
            print_warn_result "Swap usage: ${SWAP_USED}Mi / ${SWAP_TOTAL}Mi (${SWAP_USED_PCT}%) - 高 swap 使用可能是内存压力信号"
        else
            print_info "Swap usage: ${SWAP_USED}Mi / ${SWAP_TOTAL}Mi (${SWAP_USED_PCT}%)"
        fi
        print_info "Note: v1.30+ with NodeSwap feature gate enabled, swap usage may be expected"
    fi
else
    print_warn_result "Could not parse memory information"
fi

# =============================================================================
# 3. PID 使用检查 / PID Usage Check
# Thresholds: >= 90% FAIL, >= 70% WARN
# =============================================================================
print_section "3. PID Usage / PID 使用"
echo -e "  Thresholds: ${RED}FAIL >= ${PID_FAIL_PCT}%${NC} | ${YELLOW}WARN >= ${PID_WARN_PCT}%${NC} | ${GREEN}PASS < ${PID_WARN_PCT}%${NC}"
echo ""

CURRENT_PIDS=$(run_ssh "ps aux --no-heading | wc -l")
MAX_PIDS=$(run_ssh "cat /proc/sys/kernel/pid_max")

echo -e "  Current PIDs: ${BOLD}${CURRENT_PIDS}${NC}"
echo -e "  Max PIDs:     ${BOLD}${MAX_PIDS}${NC}"

if [[ -n "$CURRENT_PIDS" && -n "$MAX_PIDS" && "$CURRENT_PIDS" =~ ^[0-9]+$ && "$MAX_PIDS" =~ ^[0-9]+$ ]]; then
    PID_USAGE_PCT=$((CURRENT_PIDS * 100 / MAX_PIDS))
    echo -e "  Usage:        ${BOLD}${PID_USAGE_PCT}%${NC}"
    echo ""
    
    if [[ $PID_USAGE_PCT -ge $PID_FAIL_PCT ]]; then
        print_fail "PID usage: ${PID_USAGE_PCT}% ($CURRENT_PIDS / $MAX_PIDS) - RC-005"
    elif [[ $PID_USAGE_PCT -ge $PID_WARN_PCT ]]; then
        print_warn_result "PID usage: ${PID_USAGE_PCT}% ($CURRENT_PIDS / $MAX_PIDS) - approaching limit"
    else
        print_pass "PID usage: ${PID_USAGE_PCT}% ($CURRENT_PIDS / $MAX_PIDS)"
    fi
    
    # 显示占用 PID 最多的进程 / Show top PID consumers
    if [[ $PID_USAGE_PCT -ge $PID_WARN_PCT ]]; then
        print_info "Top 5 processes by thread count:"
        TOP_PROCS=$(run_ssh "ps -eo nlwp,pid,user,comm --sort=-nlwp --no-headers | head -5")
        echo "$TOP_PROCS" | while IFS= read -r line; do
            echo "    $line"
        done
    fi
else
    print_warn_result "Could not parse PID information"
fi

# =============================================================================
# 4. Inode 使用检查 / Inode Usage Check
# Thresholds: >= 90% FAIL, >= 70% WARN
# =============================================================================
print_section "4. Inode Usage / inode 使用 (df -i)"
echo -e "  Thresholds: ${RED}FAIL >= ${INODE_FAIL_PCT}%${NC} | ${YELLOW}WARN >= ${INODE_WARN_PCT}%${NC} | ${GREEN}PASS < ${INODE_WARN_PCT}%${NC}"
echo ""

INODE_OUTPUT=$(run_ssh "df -i / /var/lib/kubelet /var/lib/containerd 2>/dev/null || df -i /")

echo -e "  ${BOLD}Filesystem                      Inodes    IUsed     IFree  IUse%  Mounted on     Status${NC}"
echo "  ──────────────────────────────────────────────────────────────────────────────────"

echo "$INODE_OUTPUT" | awk 'NR>1' | sort -u | while IFS= read -r line; do
    [[ -z "$line" ]] && continue
    USE_PCT=$(echo "$line" | awk '{print $5}' | tr -d '%')
    
    if [[ -n "$USE_PCT" && "$USE_PCT" =~ ^[0-9]+$ ]]; then
        if [[ $USE_PCT -ge $INODE_FAIL_PCT ]]; then
            printf "  ${RED}%-40s %s${NC}\n" "$line" "[FAIL]"
        elif [[ $USE_PCT -ge $INODE_WARN_PCT ]]; then
            printf "  ${YELLOW}%-40s %s${NC}\n" "$line" "[WARN]"
        else
            printf "  ${GREEN}%-40s %s${NC}\n" "$line" "[PASS]"
        fi
    else
        echo "  $line"
    fi
done

# 统计 inode 结果 / Summarize inode results
INODE_FAIL_MOUNTS=$(run_ssh "df -i / /var/lib/kubelet /var/lib/containerd 2>/dev/null | awk 'NR>1{gsub(/%/,\"\",\$5); if(\$5>=$INODE_FAIL_PCT) print \$6\"(\"\$5\"%used)\"}'" || true)
INODE_WARN_MOUNTS=$(run_ssh "df -i / /var/lib/kubelet /var/lib/containerd 2>/dev/null | awk 'NR>1{gsub(/%/,\"\",\$5); if(\$5>=$INODE_WARN_PCT && \$5<$INODE_FAIL_PCT) print \$6\"(\"\$5\"%used)\"}'" || true)

echo ""
if [[ -n "$INODE_FAIL_MOUNTS" ]]; then
    print_fail "Inode usage critical (>= ${INODE_FAIL_PCT}%): $INODE_FAIL_MOUNTS"
elif [[ -n "$INODE_WARN_MOUNTS" ]]; then
    print_warn_result "Inode usage elevated (>= ${INODE_WARN_PCT}%): $INODE_WARN_MOUNTS"
else
    print_pass "All inode usage within safe levels"
fi

# =============================================================================
# 结果总结 / Results Summary
# =============================================================================
print_header "Resource Check Summary / 资源检查总结"

echo -e "  Node IP: ${BOLD}${NODE_IP}${NC}"
echo ""
echo -e "  ${BOLD}Results:${NC}"
echo -e "    ${GREEN}PASS: ${PASS_COUNT}${NC}"
echo -e "    ${YELLOW}WARN: ${WARN_COUNT}${NC}"
echo -e "    ${RED}FAIL: ${FAIL_COUNT}${NC}"
echo ""

TOTAL_CHECKS=$((PASS_COUNT + WARN_COUNT + FAIL_COUNT))

if [[ $FAIL_COUNT -gt 0 ]]; then
    echo -e "  ${RED}${BOLD}Overall: FAIL${NC}"
    echo -e "  ${RED}One or more resource checks exceeded critical thresholds.${NC}"
    echo ""
    echo -e "  ${BOLD}Recommended Actions / 建议操作:${NC}"
    echo "    - For disk pressure (RC-003): bash cleanup-disk.sh $NODE_IP"
    echo "    - For memory pressure (RC-004): Identify memory-hungry pods"
    echo "    - For PID pressure (RC-005): Identify processes creating excess PIDs"
    echo "    - For inode pressure (RC-003): Clean up small files (containers, tmp)"
elif [[ $WARN_COUNT -gt 0 ]]; then
    echo -e "  ${YELLOW}${BOLD}Overall: WARN${NC}"
    echo -e "  ${YELLOW}Some resource checks are approaching critical thresholds.${NC}"
    echo -e "  ${YELLOW}Proactive cleanup is recommended to prevent NotReady state.${NC}"
else
    echo -e "  ${GREEN}${BOLD}Overall: PASS${NC}"
    echo -e "  ${GREEN}All resource checks within healthy levels.${NC}"
    echo -e "  ${GREEN}Resource pressure is unlikely to be the root cause of NotReady.${NC}"
fi

echo ""
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════════════${NC}"
echo -e "  Resource Check Complete - $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════════════${NC}"
