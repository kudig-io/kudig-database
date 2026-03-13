#!/usr/bin/env bash
# =============================================================================
# K8s Node NotReady - Disk Space Cleanup (REM-002)
# 磁盘空间清理脚本 - 清理未使用的容器镜像和旧日志
#
# Usage: bash cleanup-disk.sh <node-ip> [--yes]
# Risk: LOW (removes unused images and old logs)
# Applicable Root Cause: RC-003 (DiskPressure)
# Source: SKILL-NODE-001 REM-002
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

SSH_OPTS="-o ConnectTimeout=10 -o StrictHostKeyChecking=no -o BatchMode=yes"
AUTO_CONFIRM=false

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
}

print_error() {
    echo -e "  ${RED}[ERROR]${NC} $1"
}

print_info() {
    echo -e "  ${BLUE}[INFO]${NC} $1"
}

# SSH 命令封装 / SSH command wrapper
run_ssh() {
    ssh $SSH_OPTS "$NODE_IP" "$1" 2>&1 || true
}

# --- 参数解析 / Argument Parsing ---
if [[ $# -lt 1 ]]; then
    echo -e "${RED}Error: Missing required argument.${NC}"
    echo ""
    echo "Usage: bash cleanup-disk.sh <node-ip> [--yes]"
    echo ""
    echo "  <node-ip>  IP address of the node to clean (SSH must be accessible)"
    echo "  --yes      Skip confirmation prompt (use with caution)"
    echo ""
    echo "Examples:"
    echo "  bash cleanup-disk.sh 10.0.1.100"
    echo "  bash cleanup-disk.sh 10.0.1.100 --yes"
    echo ""
    echo "What this script cleans:"
    echo "  1. Unused container images (crictl rmi --prune)"
    echo "  2. Old compressed log files (*.gz older than 7 days)"
    echo "  3. Old rotated log files (*.old older than 3 days)"
    echo "  4. Old journal logs (older than 2 days)"
    echo ""
    echo -e "${YELLOW}Risk: LOW - Only removes unused images and old logs.${NC}"
    echo -e "${YELLOW}Running containers and active logs are NOT affected.${NC}"
    exit 1
fi

NODE_IP="$1"
shift

# 解析可选参数 / Parse optional arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --yes|-y)
            AUTO_CONFIRM=true
            shift
            ;;
        *)
            echo -e "${RED}Error: Unknown argument: $1${NC}"
            exit 1
            ;;
    esac
done

# --- 检查 SSH 可达性 / Check SSH connectivity ---
if ! ssh $SSH_OPTS "$NODE_IP" "echo 'ok'" &>/dev/null; then
    echo -e "${RED}Error: Cannot establish SSH connection to $NODE_IP${NC}"
    exit 1
fi

print_header "K8s Node NotReady - Disk Space Cleanup (REM-002)"
echo -e "  Target Node IP: ${BOLD}${NODE_IP}${NC}"
echo -e "  Timestamp:      $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "  Risk Level:     ${YELLOW}LOW (removes unused images and old logs)${NC}"
echo -e "  Root Cause:     RC-003 (DiskPressure)"

# =============================================================================
# 清理前：显示当前磁盘使用情况
# BEFORE cleanup: Show current disk usage
# =============================================================================
print_section "BEFORE Cleanup / 清理前磁盘状态"

print_info "Current disk usage on critical partitions:"
DISK_BEFORE=$(run_ssh "df -h / /var/lib/kubelet /var/lib/containerd /var/log 2>/dev/null || df -h /")
echo "$DISK_BEFORE" | while IFS= read -r line; do
    if echo "$line" | grep -q "Filesystem"; then
        echo "  $line"
    else
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

# 记录清理前的总已用空间（根分区）/ Record pre-cleanup usage
USED_BEFORE_KB=$(run_ssh "df -k / | awk 'NR==2{print \$3}'")

echo ""
print_info "Disk usage summary for cleanup targets:"

# 容器镜像大小估算 / Estimate container images size
IMAGES_SIZE=$(run_ssh "crictl images -o json 2>/dev/null | python3 -c 'import sys,json; d=json.load(sys.stdin); print(sum(int(i.get(\"size\",\"0\")) for i in d.get(\"images\",[])))' 2>/dev/null || echo '0'")
if [[ -n "$IMAGES_SIZE" && "$IMAGES_SIZE" =~ ^[0-9]+$ && "$IMAGES_SIZE" -gt 0 ]]; then
    IMAGES_SIZE_MB=$((IMAGES_SIZE / 1024 / 1024))
    print_info "Total container images: ~${IMAGES_SIZE_MB}Mi"
fi

# 日志大小估算 / Estimate log sizes
GZ_SIZE=$(run_ssh "find /var/log -name '*.gz' -mtime +7 -exec du -cb {} + 2>/dev/null | tail -1 | awk '{print \$1}'" || echo "0")
OLD_SIZE=$(run_ssh "find /var/log -name '*.old' -mtime +3 -exec du -cb {} + 2>/dev/null | tail -1 | awk '{print \$1}'" || echo "0")
GZ_SIZE_MB=$(( ${GZ_SIZE:-0} / 1024 / 1024 ))
OLD_SIZE_MB=$(( ${OLD_SIZE:-0} / 1024 / 1024 ))
print_info "Old compressed logs (*.gz, >7 days): ~${GZ_SIZE_MB}Mi"
print_info "Old rotated logs (*.old, >3 days): ~${OLD_SIZE_MB}Mi"

JOURNAL_SIZE=$(run_ssh "journalctl --disk-usage 2>/dev/null | grep -oP '[0-9.]+[GMK]' || echo 'unknown'")
print_info "Journal log size: $JOURNAL_SIZE"

# =============================================================================
# 确认清理操作 / Confirm cleanup operation
# =============================================================================
if [[ "$AUTO_CONFIRM" != "true" ]]; then
    echo ""
    echo -e "  ${YELLOW}${BOLD}⚠️  Cleanup Operations to be Performed / 将要执行的清理操作:${NC}"
    echo -e "    ${YELLOW}1. Remove unused container images (crictl rmi --prune)${NC}"
    echo -e "    ${YELLOW}2. Remove old compressed logs *.gz (older than 7 days) from /var/log${NC}"
    echo -e "    ${YELLOW}3. Remove old rotated logs *.old (older than 3 days) from /var/log${NC}"
    echo -e "    ${YELLOW}4. Vacuum journal logs older than 2 days${NC}"
    echo ""
    echo -e "  ${GREEN}NOTE: Running containers and active logs will NOT be affected.${NC}"
    echo -e "  ${GREEN}NOTE: Removed images will be auto-pulled when pods are rescheduled.${NC}"
    echo ""
    
    read -rp "  Proceed with cleanup? [y/N]: " CONFIRM
    if [[ ! "$CONFIRM" =~ ^[yY]([eE][sS])?$ ]]; then
        echo -e "  ${YELLOW}Cleanup cancelled by user.${NC}"
        exit 0
    fi
fi

# =============================================================================
# Step 1: 清理未使用的容器镜像
# Remove unused container images
# Command: crictl rmi --prune
# =============================================================================
print_section "Step 1/4: Remove Unused Container Images / 清理未使用容器镜像"
print_info "Running: crictl rmi --prune"
print_info "This removes images not referenced by any running container..."

STEP1_OUTPUT=$(run_ssh "crictl rmi --prune 2>&1")
if [[ -n "$STEP1_OUTPUT" ]]; then
    REMOVED_IMAGES=$(echo "$STEP1_OUTPUT" | grep -c "Deleted" || echo "0")
    if [[ "$REMOVED_IMAGES" -gt 0 ]]; then
        print_ok "Removed $REMOVED_IMAGES unused image(s)"
    else
        print_info "No unused images to remove"
    fi
    # 显示详细输出（最多10行） / Show detailed output (max 10 lines)
    echo "$STEP1_OUTPUT" | tail -10 | while IFS= read -r line; do
        echo "    $line"
    done
else
    print_info "No output from crictl rmi --prune (no unused images)"
fi

# =============================================================================
# Step 2: 清理旧的压缩日志文件
# Remove old compressed log files (*.gz older than 7 days)
# Command: find /var/log -name '*.gz' -mtime +7 -delete
# =============================================================================
print_section "Step 2/4: Remove Old Compressed Logs / 清理旧压缩日志 (*.gz > 7d)"
print_info "Running: find /var/log -name '*.gz' -mtime +7 -delete"

GZ_COUNT=$(run_ssh "find /var/log -name '*.gz' -mtime +7 2>/dev/null | wc -l")
if [[ -n "$GZ_COUNT" && "$GZ_COUNT" -gt 0 ]]; then
    STEP2_OUTPUT=$(run_ssh "find /var/log -name '*.gz' -mtime +7 -delete 2>&1 && echo 'CLEANUP_OK'")
    if echo "$STEP2_OUTPUT" | grep -q "CLEANUP_OK"; then
        print_ok "Removed $GZ_COUNT compressed log file(s) (~${GZ_SIZE_MB}Mi)"
    else
        print_warn "Some files could not be removed (permission denied?)"
        echo "    $STEP2_OUTPUT" | head -5
    fi
else
    print_info "No old compressed logs found (*.gz > 7 days)"
fi

# =============================================================================
# Step 3: 清理旧的轮转日志文件
# Remove old rotated log files (*.old older than 3 days)
# Command: find /var/log -name '*.old' -mtime +3 -delete
# =============================================================================
print_section "Step 3/4: Remove Old Rotated Logs / 清理旧轮转日志 (*.old > 3d)"
print_info "Running: find /var/log -name '*.old' -mtime +3 -delete"

OLD_COUNT=$(run_ssh "find /var/log -name '*.old' -mtime +3 2>/dev/null | wc -l")
if [[ -n "$OLD_COUNT" && "$OLD_COUNT" -gt 0 ]]; then
    STEP3_OUTPUT=$(run_ssh "find /var/log -name '*.old' -mtime +3 -delete 2>&1 && echo 'CLEANUP_OK'")
    if echo "$STEP3_OUTPUT" | grep -q "CLEANUP_OK"; then
        print_ok "Removed $OLD_COUNT rotated log file(s) (~${OLD_SIZE_MB}Mi)"
    else
        print_warn "Some files could not be removed (permission denied?)"
        echo "    $STEP3_OUTPUT" | head -5
    fi
else
    print_info "No old rotated logs found (*.old > 3 days)"
fi

# =============================================================================
# Step 4: 清理 journal 日志
# Vacuum journal logs (keep last 2 days)
# Command: journalctl --vacuum-time=2d
# =============================================================================
print_section "Step 4/4: Vacuum Journal Logs / 清理 journal 日志 (保留最近 2d)"
print_info "Running: journalctl --vacuum-time=2d"

STEP4_OUTPUT=$(run_ssh "journalctl --vacuum-time=2d 2>&1")
if [[ -n "$STEP4_OUTPUT" ]]; then
    echo "$STEP4_OUTPUT" | while IFS= read -r line; do
        if echo "$line" | grep -qi "freed\|vacuuming\|deleted"; then
            echo -e "    ${GREEN}$line${NC}"
        else
            echo "    $line"
        fi
    done
    
    FREED_JOURNAL=$(echo "$STEP4_OUTPUT" | grep -oP 'freed [0-9.]+[GMK]' || echo "")
    if [[ -n "$FREED_JOURNAL" ]]; then
        print_ok "Journal cleanup: $FREED_JOURNAL"
    else
        print_info "Journal vacuum completed"
    fi
else
    print_info "No journal output (may need root permissions)"
fi

# =============================================================================
# 清理后：显示磁盘使用情况
# AFTER cleanup: Show disk usage
# =============================================================================
print_section "AFTER Cleanup / 清理后磁盘状态"

print_info "Current disk usage on critical partitions:"
DISK_AFTER=$(run_ssh "df -h / /var/lib/kubelet /var/lib/containerd /var/log 2>/dev/null || df -h /")
echo "$DISK_AFTER" | while IFS= read -r line; do
    if echo "$line" | grep -q "Filesystem"; then
        echo "  $line"
    else
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

# =============================================================================
# 清理总结 / Cleanup Summary
# =============================================================================
print_header "Cleanup Summary / 清理总结"

# 计算释放的空间 / Calculate freed space
USED_AFTER_KB=$(run_ssh "df -k / | awk 'NR==2{print \$3}'")
if [[ -n "$USED_BEFORE_KB" && -n "$USED_AFTER_KB" && "$USED_BEFORE_KB" =~ ^[0-9]+$ && "$USED_AFTER_KB" =~ ^[0-9]+$ ]]; then
    FREED_KB=$((USED_BEFORE_KB - USED_AFTER_KB))
    if [[ $FREED_KB -gt 0 ]]; then
        if [[ $FREED_KB -gt 1048576 ]]; then
            FREED_DISPLAY="$((FREED_KB / 1048576))Gi"
        elif [[ $FREED_KB -gt 1024 ]]; then
            FREED_DISPLAY="$((FREED_KB / 1024))Mi"
        else
            FREED_DISPLAY="${FREED_KB}Ki"
        fi
        echo -e "  ${GREEN}${BOLD}Total Space Recovered: ~${FREED_DISPLAY}${NC}"
    else
        echo -e "  ${YELLOW}Space recovered: minimal (root partition usage unchanged)${NC}"
        echo -e "  ${YELLOW}Note: cleaned data may be on different partitions${NC}"
    fi
else
    echo -e "  ${YELLOW}Could not calculate space recovered${NC}"
fi

echo ""
echo -e "  ${BOLD}Cleanup Actions Performed:${NC}"
echo -e "    1. ${GREEN}✓${NC} Unused container images pruned (crictl rmi --prune)"
echo -e "    2. ${GREEN}✓${NC} Old compressed logs removed (*.gz > 7 days)"
echo -e "    3. ${GREEN}✓${NC} Old rotated logs removed (*.old > 3 days)"
echo -e "    4. ${GREEN}✓${NC} Journal logs vacuumed (kept last 2 days)"

echo ""
echo -e "  ${BOLD}Next Steps / 后续步骤:${NC}"
echo "    1. Wait 1-2 minutes for kubelet to re-evaluate disk conditions"
echo "    2. Verify node recovery: bash verify-node.sh <node-name>"
echo "    3. Check DiskPressure condition:"
echo "       kubectl get node <node-name> -o jsonpath='{.status.conditions[?(@.type==\"DiskPressure\")].status}'"
echo ""
echo -e "  ${YELLOW}NOTE: Removed images will be auto-pulled when Pods are rescheduled.${NC}"
echo -e "  ${YELLOW}NOTE: This is a non-reversible operation, but only cache/logs were removed.${NC}"

echo ""
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════════════${NC}"
echo -e "  Disk Cleanup Complete - $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════════════${NC}"
