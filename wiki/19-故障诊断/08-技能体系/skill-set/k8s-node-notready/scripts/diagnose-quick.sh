#!/usr/bin/env bash
# =============================================================================
# K8s Node NotReady - Phase 1 Quick Diagnosis (Read-only)
# 快速诊断脚本 - 通过 kubectl 远程收集节点状态信息，无需 SSH
#
# Usage: bash diagnose-quick.sh <node-name>
# Risk: NONE (read-only kubectl operations)
# Source: SKILL-NODE-001 D1.1-D1.5
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

# --- 参数验证 / Argument Validation ---
if [[ $# -lt 1 ]]; then
    echo -e "${RED}Error: Missing required argument.${NC}"
    echo ""
    echo "Usage: bash diagnose-quick.sh <node-name>"
    echo ""
    echo "  <node-name>  Name of the Kubernetes node to diagnose"
    echo ""
    echo "Examples:"
    echo "  bash diagnose-quick.sh worker-node-01"
    echo "  bash diagnose-quick.sh ip-10-0-1-100.ec2.internal"
    exit 1
fi

NODE_NAME="$1"

# --- 检查 kubectl 可用性和版本 / Check kubectl availability and version ---
if ! command -v kubectl &>/dev/null; then
    echo -e "${RED}Error: kubectl is not installed or not in PATH.${NC}"
    exit 1
fi

# 尝试 jq 优先解析，回退到 sed (更健壮，不受 JSON 空格格式影响)
KUBECTL_VERSION=$(kubectl version --client -o json 2>/dev/null | jq -r '.gitVersion' 2>/dev/null | sed 's/^v//' || \
                  kubectl version --client -o json 2>/dev/null | sed -n 's/.*"gitVersion":[[:space:]]*"v\([^"]*\)".*/\1/p' | head -1 || \
                  echo "")
if [[ -n "$KUBECTL_VERSION" ]]; then
    print_info "kubectl version: v${KUBECTL_VERSION}"
fi

# --- 检查 kubectl 连接 / Check kubectl connectivity ---
if ! kubectl cluster-info &>/dev/null; then
    echo -e "${RED}Error: Cannot connect to Kubernetes cluster. Check your kubeconfig.${NC}"
    exit 1
fi

# --- 验证节点存在 / Validate node exists ---
if ! kubectl get node "$NODE_NAME" &>/dev/null; then
    echo -e "${RED}Error: Node '$NODE_NAME' not found in the cluster.${NC}"
    echo ""
    echo "Available nodes:"
    kubectl get nodes --no-headers | awk '{print "  " $1 " (" $2 ")"}'
    exit 1
fi

print_header "K8s Node NotReady - Phase 1 Quick Diagnosis"
echo -e "  Target Node: ${BOLD}${NODE_NAME}${NC}"
echo -e "  Timestamp:   $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "  Risk Level:  ${GREEN}NONE (read-only)${NC}"

# =============================================================================
# D1.1: 获取节点全局状态概览
# Get node global status overview
# Command: kubectl get nodes -o wide
# =============================================================================
print_section "D1.1: Node Status Overview / 节点状态概览"

NODE_STATUS_OUTPUT=$(kubectl get nodes -o wide 2>&1)
echo "$NODE_STATUS_OUTPUT" | while IFS= read -r line; do
    if echo "$line" | grep -q "$NODE_NAME"; then
        # 高亮目标节点行 / Highlight target node line
        if echo "$line" | grep -q "NotReady"; then
            echo -e "  ${RED}${BOLD}>>> $line${NC}"
        elif echo "$line" | grep -q "Ready"; then
            echo -e "  ${GREEN}>>> $line${NC}"
        else
            echo -e "  ${YELLOW}>>> $line${NC}"
        fi
    else
        echo "  $line"
    fi
done

# 提取目标节点状态 / Extract target node status
TARGET_STATUS=$(kubectl get node "$NODE_NAME" --no-headers | awk '{print $2}')
if echo "$TARGET_STATUS" | grep -q "NotReady"; then
    print_error "Node $NODE_NAME is in NotReady state"
    add_finding "D1.1: Node status is NotReady"
elif echo "$TARGET_STATUS" | grep -q "Ready,SchedulingDisabled"; then
    print_warn "Node $NODE_NAME is Ready but SchedulingDisabled (cordoned)"
    add_finding "D1.1: Node is cordoned (SchedulingDisabled) - possibly RC-012"
elif echo "$TARGET_STATUS" | grep -q "Ready"; then
    print_ok "Node $NODE_NAME is in Ready state"
else
    print_warn "Node $NODE_NAME status: $TARGET_STATUS"
    add_finding "D1.1: Unexpected node status: $TARGET_STATUS"
fi

# =============================================================================
# D1.2: 获取节点详细状态和 Conditions
# Get node detailed status and Conditions
# Command: kubectl describe node <node-name>
# =============================================================================
print_section "D1.2: Node Conditions / 节点状态条件"

# 提取 Conditions 信息 / Extract Conditions information
CONDITIONS_JSON=$(kubectl get node "$NODE_NAME" -o jsonpath='{range .status.conditions[*]}{.type}{"\t"}{.status}{"\t"}{.reason}{"\t"}{.message}{"\n"}{end}' 2>&1)

echo -e "  ${BOLD}Type                 Status    Reason${NC}"
echo "  ────────────────────────────────────────────────────────────"

while IFS=$'\t' read -r ctype cstatus creason cmessage; do
    [[ -z "$ctype" ]] && continue
    
    case "$ctype" in
        Ready)
            if [[ "$cstatus" == "True" ]]; then
                echo -e "  ${GREEN}$ctype${NC}\t\t     $cstatus\t   $creason"
            elif [[ "$cstatus" == "False" ]]; then
                echo -e "  ${RED}$ctype${NC}\t\t     $cstatus\t   $creason"
                print_error "Ready=False, Reason: $creason"
                add_finding "D1.2: Ready=False ($creason) - kubelet 无法正常工作，可能 RC-001"
                if echo "$cmessage" | grep -qi "container runtime"; then
                    add_finding "D1.2: Message indicates container runtime issue - RC-002"
                fi
                if echo "$cmessage" | grep -qi "PLEG"; then
                    add_finding "D1.2: Message indicates PLEG issue - RC-008"
                fi
                if echo "$cmessage" | grep -qi "certificate\|x509"; then
                    add_finding "D1.2: Message indicates certificate issue - RC-007"
                fi
            else
                echo -e "  ${YELLOW}$ctype${NC}\t\t     $cstatus\t   $creason"
                print_warn "Ready=Unknown - apiserver 长时间未收到心跳"
                add_finding "D1.2: Ready=Unknown - 可能网络问题 RC-006 或 kubelet 停止 RC-001"
            fi
            ;;
        MemoryPressure)
            if [[ "$cstatus" == "True" ]]; then
                echo -e "  ${RED}$ctype${NC}\t     $cstatus\t   $creason"
                print_error "MemoryPressure=True"
                add_finding "D1.2: MemoryPressure=True - 可能根因 RC-004"
            else
                echo -e "  ${GREEN}$ctype${NC}\t     $cstatus\t   $creason"
            fi
            ;;
        DiskPressure)
            if [[ "$cstatus" == "True" ]]; then
                echo -e "  ${RED}$ctype${NC}\t     $cstatus\t   $creason"
                print_error "DiskPressure=True"
                add_finding "D1.2: DiskPressure=True - 可能根因 RC-003"
            else
                echo -e "  ${GREEN}$ctype${NC}\t     $cstatus\t   $creason"
            fi
            ;;
        PIDPressure)
            if [[ "$cstatus" == "True" ]]; then
                echo -e "  ${RED}$ctype${NC}\t     $cstatus\t   $creason"
                print_error "PIDPressure=True"
                add_finding "D1.2: PIDPressure=True - 可能根因 RC-005"
            else
                echo -e "  ${GREEN}$ctype${NC}\t     $cstatus\t   $creason"
            fi
            ;;
        NetworkUnavailable)
            if [[ "$cstatus" == "True" ]]; then
                echo -e "  ${RED}$ctype${NC}\t     $cstatus\t   $creason"
                print_error "NetworkUnavailable=True"
                add_finding "D1.2: NetworkUnavailable=True - 可能 CNI 问题 RC-011"
            else
                echo -e "  ${GREEN}$ctype${NC}\t     $cstatus\t   $creason"
            fi
            ;;
        *)
            echo -e "  $ctype\t     $cstatus\t   $creason"
            ;;
    esac
done <<< "$CONDITIONS_JSON"

# =============================================================================
# D1.3: 检查节点事件
# Check node events
# Command: kubectl get events --field-selector involvedObject.kind=Node,involvedObject.name=<node-name>
# =============================================================================
print_section "D1.3: Node Events (Last 30) / 节点事件（最近30条）"

EVENTS_OUTPUT=$(kubectl get events \
    --field-selector "involvedObject.kind=Node,involvedObject.name=${NODE_NAME}" \
    --sort-by=.lastTimestamp --no-headers 2>&1 | tail -30)

if [[ -z "$EVENTS_OUTPUT" || "$EVENTS_OUTPUT" == *"No resources found"* ]]; then
    print_warn "No recent events found for node $NODE_NAME"
    add_finding "D1.3: 无近期事件 - 可能是网络分区，apiserver 未收到任何更新 (RC-006)"
else
    # 显示事件并高亮 Warning 类型 / Display events and highlight Warnings
    echo "$EVENTS_OUTPUT" | while IFS= read -r line; do
        if echo "$line" | grep -qi "warning\|NodeNotReady\|NotReady"; then
            echo -e "  ${RED}$line${NC}"
        elif echo "$line" | grep -qi "DiskPressure\|MemoryPressure\|PIDPressure"; then
            echo -e "  ${YELLOW}$line${NC}"
        else
            echo -e "  $line"
        fi
    done

    # 分析事件关键词 / Analyze event keywords
    if echo "$EVENTS_OUTPUT" | grep -qi "NodeNotReady"; then
        add_finding "D1.3: NodeNotReady event detected"
    fi
    if echo "$EVENTS_OUTPUT" | grep -qi "NodeHasDiskPressure\|DiskPressure"; then
        add_finding "D1.3: DiskPressure event - RC-003"
    fi
    if echo "$EVENTS_OUTPUT" | grep -qi "NodeHasMemoryPressure\|MemoryPressure\|InsufficientMemory"; then
        add_finding "D1.3: MemoryPressure event - RC-004"
    fi
    if echo "$EVENTS_OUTPUT" | grep -qi "NodeHasPIDPressure\|PIDPressure"; then
        add_finding "D1.3: PIDPressure event - RC-005"
    fi
    if echo "$EVENTS_OUTPUT" | grep -qi "Rebooted"; then
        add_finding "D1.3: Node reboot detected - 关注 RC-009 内核/硬件问题"
    fi
    if echo "$EVENTS_OUTPUT" | grep -qi "Starting"; then
        add_finding "D1.3: kubelet Starting event - RC-001 的恢复迹象"
    fi
fi

# =============================================================================
# D1.4: 检查节点 Taints
# Check node Taints
# Command: kubectl get node <node-name> -o jsonpath='{range .spec.taints[*]}...'
# =============================================================================
print_section "D1.4: Node Taints / 节点污点"

TAINTS_OUTPUT=$(kubectl get node "$NODE_NAME" \
    -o jsonpath='{range .spec.taints[*]}{.key}={.value}:{.effect}{"\n"}{end}' 2>&1)

if [[ -z "$TAINTS_OUTPUT" ]]; then
    print_ok "No taints found on node"
else
    echo -e "  ${BOLD}Taint                                                   Effect${NC}"
    echo "  ────────────────────────────────────────────────────────────"
    
    while IFS= read -r taint; do
        [[ -z "$taint" ]] && continue
        case "$taint" in
            *"node.kubernetes.io/not-ready"*"NoExecute"*)
                echo -e "  ${RED}$taint${NC}"
                print_error "NoExecute taint present - Pod eviction triggered"
                add_finding "D1.4: not-ready:NoExecute - Pod 驱逐已触发"
                ;;
            *"node.kubernetes.io/not-ready"*"NoSchedule"*)
                echo -e "  ${YELLOW}$taint${NC}"
                print_warn "NoSchedule taint present - new Pods cannot be scheduled"
                add_finding "D1.4: not-ready:NoSchedule - 确认 NotReady 状态"
                ;;
            *"node.kubernetes.io/unreachable"*"NoExecute"*)
                echo -e "  ${RED}$taint${NC}"
                print_error "Node unreachable taint - node is unreachable"
                add_finding "D1.4: unreachable:NoExecute - 节点不可达"
                ;;
            *"node.kubernetes.io/unschedulable"*)
                echo -e "  ${YELLOW}$taint${NC}"
                print_warn "Node is cordoned (unschedulable)"
                add_finding "D1.4: unschedulable - 节点已被 cordon (RC-012)"
                ;;
            *"node.kubernetes.io/disk-pressure"*)
                echo -e "  ${RED}$taint${NC}"
                print_error "DiskPressure taint present"
                add_finding "D1.4: disk-pressure taint - RC-003"
                ;;
            *"node.kubernetes.io/memory-pressure"*)
                echo -e "  ${RED}$taint${NC}"
                print_error "MemoryPressure taint present"
                add_finding "D1.4: memory-pressure taint - RC-004"
                ;;
            *"node.kubernetes.io/pid-pressure"*)
                echo -e "  ${RED}$taint${NC}"
                print_error "PIDPressure taint present"
                add_finding "D1.4: pid-pressure taint - RC-005"
                ;;
            *)
                echo -e "  $taint"
                ;;
        esac
    done <<< "$TAINTS_OUTPUT"
fi

# =============================================================================
# D1.5: 检查节点 Lease 对象
# Check node Lease object
# Command: kubectl get lease -n kube-node-lease <node-name> -o jsonpath='{.spec.renewTime}'
# =============================================================================
print_section "D1.5: Node Lease / 节点 Lease 续租状态"

LEASE_RENEW_TIME=$(kubectl get lease -n kube-node-lease "$NODE_NAME" \
    -o jsonpath='{.spec.renewTime}' 2>&1)

if [[ -z "$LEASE_RENEW_TIME" || "$LEASE_RENEW_TIME" == *"not found"* ]]; then
    print_error "Lease object not found for node $NODE_NAME"
    add_finding "D1.5: Lease 对象未找到"
else
    print_info "Lease renewTime: $LEASE_RENEW_TIME"
    
    # 计算 Lease 距当前时间的秒数 / Calculate seconds since last renewal
    CURRENT_TIME=$(date -u +%s 2>/dev/null || echo "")
    # 尝试解析 Lease 时间 / Try to parse lease time (cross-platform: macOS BSD date vs GNU date)
    # Strip fractional seconds and timezone suffix for consistent parsing
    LEASE_TIME_CLEAN="${LEASE_RENEW_TIME%%.*}"
    LEASE_TIME_CLEAN="${LEASE_TIME_CLEAN//Z/}"
    LEASE_EPOCH=$(date -u -jf "%Y-%m-%dT%H:%M:%S" "$LEASE_TIME_CLEAN" +%s 2>/dev/null || \
                  date -u -d "$LEASE_TIME_CLEAN" +%s 2>/dev/null || \
                  echo "")
    
    if [[ -n "$LEASE_EPOCH" && -n "$CURRENT_TIME" ]]; then
        TIME_DIFF=$((CURRENT_TIME - LEASE_EPOCH))
        
        if [[ $TIME_DIFF -gt 40 ]]; then
            print_error "Lease not renewed for ${TIME_DIFF}s (threshold: 40s)"
            print_info "kubelet 未能续租，可能 kubelet 停止 (RC-001) 或网络不通 (RC-006)"
            add_finding "D1.5: Lease 超过 ${TIME_DIFF}s 未续租 - 可能 RC-001 或 RC-006"
        elif [[ $TIME_DIFF -gt 20 ]]; then
            print_warn "Lease renewed ${TIME_DIFF}s ago (approaching threshold)"
            add_finding "D1.5: Lease 续租延迟 ${TIME_DIFF}s - 需关注"
        else
            print_ok "Lease renewed ${TIME_DIFF}s ago (within normal range)"
        fi
    else
        print_info "Cannot calculate time difference. Lease renewTime: $LEASE_RENEW_TIME"
        print_info "Please manually verify the renewTime is recent (within 40s)"
    fi
fi

# =============================================================================
# 诊断总结 / Diagnosis Summary
# =============================================================================
print_header "Diagnosis Summary / 诊断总结"

echo -e "  Node:      ${BOLD}${NODE_NAME}${NC}"
echo -e "  Status:    ${BOLD}${TARGET_STATUS}${NC}"
echo ""

# 输出发现 / Print findings
if [[ ${#FINDINGS[@]} -gt 0 ]]; then
    echo -e "  ${BOLD}Findings / 发现:${NC}"
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

# 建议下一步 / Next steps recommendation
echo -e "  ${BOLD}Recommended Next Steps / 建议下一步:${NC}"
if [[ ${#ERRORS[@]} -gt 0 ]]; then
    echo -e "    ${YELLOW}1. Run Phase 2 deep diagnosis (requires SSH access):${NC}"
    echo -e "       bash diagnose-deep.sh <node-ip>"
    echo -e "    ${YELLOW}2. Check resources on the node:${NC}"
    echo -e "       bash check-resources.sh <node-ip>"
else
    echo -e "    ${GREEN}No critical issues found in Phase 1.${NC}"
    echo -e "    If node appears unhealthy, proceed with Phase 2 deep diagnosis."
fi

echo ""
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════════════${NC}"
echo -e "  Phase 1 Quick Diagnosis Complete - $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════════════${NC}"
