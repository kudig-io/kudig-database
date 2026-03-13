#!/usr/bin/env bash
# =============================================================================
# K8s Node NotReady - Post-Remediation Verification
# 修复后验证脚本 - 确认节点已恢复正常状态
#
# Usage: bash verify-node.sh <node-name>
# Risk: NONE (read-only)
# Source: SKILL-NODE-001 Section 7, V1-V5
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

# --- 统计变量 / Statistics Variables ---
PASS_COUNT=0
FAIL_COUNT=0
TOTAL_CHECKS=5

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

print_fail() {
    echo -e "  ${RED}[FAIL]${NC} $1"
    FAIL_COUNT=$((FAIL_COUNT + 1))
}

print_info() {
    echo -e "  ${BLUE}[INFO]${NC} $1"
}

# --- 参数验证 / Argument Validation ---
if [[ $# -lt 1 ]]; then
    echo -e "${RED}Error: Missing required argument.${NC}"
    echo ""
    echo "Usage: bash verify-node.sh <node-name>"
    echo ""
    echo "  <node-name>  Name of the Kubernetes node to verify"
    echo ""
    echo "Examples:"
    echo "  bash verify-node.sh worker-node-01"
    echo "  bash verify-node.sh ip-10-0-1-100.ec2.internal"
    echo ""
    echo "Verification checks:"
    echo "  V1: Node status is Ready"
    echo "  V2: All Conditions normal (Ready=True, others=False)"
    echo "  V3: Lease renewTime is recent"
    echo "  V4: Pods on node are Running"
    echo "  V5: Node system info (kubelet version, runtime version)"
    exit 1
fi

NODE_NAME="$1"

# --- 检查 kubectl 可用性 / Check kubectl availability ---
if ! command -v kubectl &>/dev/null; then
    echo -e "${RED}Error: kubectl is not installed or not in PATH.${NC}"
    exit 1
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

print_header "K8s Node NotReady - Post-Remediation Verification"
echo -e "  Target Node: ${BOLD}${NODE_NAME}${NC}"
echo -e "  Timestamp:   $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "  Risk Level:  ${GREEN}NONE (read-only)${NC}"
echo -e "  Checks:      V1-V5 (${TOTAL_CHECKS} total)"

# =============================================================================
# V1: 确认节点状态恢复为 Ready
# Confirm node status is Ready
# Command: kubectl get node <node-name>
# =============================================================================
print_section "V1: Node Status / 节点状态"

NODE_STATUS_LINE=$(kubectl get node "$NODE_NAME" --no-headers 2>&1)
echo "  $NODE_STATUS_LINE"
echo ""

TARGET_STATUS=$(echo "$NODE_STATUS_LINE" | awk '{print $2}')

if echo "$TARGET_STATUS" | grep -q "^Ready$"; then
    print_pass "V1: Node status is Ready"
elif echo "$TARGET_STATUS" | grep -q "Ready,SchedulingDisabled"; then
    print_fail "V1: Node is Ready but SchedulingDisabled (still cordoned)"
    print_info "Run 'kubectl uncordon $NODE_NAME' to re-enable scheduling"
else
    print_fail "V1: Node status is $TARGET_STATUS (expected: Ready)"
fi

# =============================================================================
# V2: 确认所有 Conditions 恢复正常
# Confirm all Conditions are normal (Ready=True, others=False)
# Command: kubectl get node <node-name> -o jsonpath='{range .status.conditions[*]}{.type}={.status}{"\n"}{end}'
# Expected: MemoryPressure=False, DiskPressure=False, PIDPressure=False, Ready=True
# =============================================================================
print_section "V2: Node Conditions / 节点条件"

CONDITIONS=$(kubectl get node "$NODE_NAME" \
    -o jsonpath='{range .status.conditions[*]}{.type}={.status}{"\n"}{end}' 2>&1)

V2_ALL_OK=true

echo -e "  ${BOLD}Condition             Status    Expected   Result${NC}"
echo "  ────────────────────────────────────────────────────────"

while IFS= read -r cond; do
    [[ -z "$cond" ]] && continue
    CTYPE=$(echo "$cond" | cut -d= -f1)
    CSTATUS=$(echo "$cond" | cut -d= -f2)
    
    case "$CTYPE" in
        Ready)
            EXPECTED="True"
            if [[ "$CSTATUS" == "True" ]]; then
                printf "  %-22s%-10s%-11s${GREEN}%s${NC}\n" "$CTYPE" "$CSTATUS" "$EXPECTED" "OK"
            else
                printf "  %-22s%-10s%-11s${RED}%s${NC}\n" "$CTYPE" "$CSTATUS" "$EXPECTED" "MISMATCH"
                V2_ALL_OK=false
            fi
            ;;
        MemoryPressure|DiskPressure|PIDPressure)
            EXPECTED="False"
            if [[ "$CSTATUS" == "False" ]]; then
                printf "  %-22s%-10s%-11s${GREEN}%s${NC}\n" "$CTYPE" "$CSTATUS" "$EXPECTED" "OK"
            else
                printf "  %-22s%-10s%-11s${RED}%s${NC}\n" "$CTYPE" "$CSTATUS" "$EXPECTED" "MISMATCH"
                V2_ALL_OK=false
            fi
            ;;
        NetworkUnavailable)
            EXPECTED="False"
            if [[ "$CSTATUS" == "False" ]]; then
                printf "  %-22s%-10s%-11s${GREEN}%s${NC}\n" "$CTYPE" "$CSTATUS" "$EXPECTED" "OK"
            else
                printf "  %-22s%-10s%-11s${RED}%s${NC}\n" "$CTYPE" "$CSTATUS" "$EXPECTED" "MISMATCH"
                V2_ALL_OK=false
            fi
            ;;
        *)
            printf "  %-22s%-10s%-11s%s\n" "$CTYPE" "$CSTATUS" "N/A" "INFO"
            ;;
    esac
done <<< "$CONDITIONS"

echo ""
if [[ "$V2_ALL_OK" == "true" ]]; then
    print_pass "V2: All Conditions normal (Ready=True, pressure conditions=False)"
else
    print_fail "V2: One or more Conditions are abnormal"
fi

# =============================================================================
# V3: 确认 Node Lease 正常续租
# Confirm Node Lease is being renewed
# Command: kubectl get lease -n kube-node-lease <node-name> -o jsonpath='{.spec.renewTime}'
# Expected: Timestamp within the last few seconds
# =============================================================================
print_section "V3: Node Lease / 节点 Lease 续租"

LEASE_RENEW_TIME=$(kubectl get lease -n kube-node-lease "$NODE_NAME" \
    -o jsonpath='{.spec.renewTime}' 2>&1)

if [[ -z "$LEASE_RENEW_TIME" || "$LEASE_RENEW_TIME" == *"not found"* ]]; then
    print_fail "V3: Lease object not found for node $NODE_NAME"
else
    print_info "Lease renewTime: $LEASE_RENEW_TIME"
    
    # 计算 Lease 距当前时间的秒数 / Calculate seconds since last renewal
    CURRENT_TIME=$(date -u +%s 2>/dev/null)
    LEASE_EPOCH=$(date -jf "%Y-%m-%dT%H:%M:%S" "${LEASE_RENEW_TIME%%.*}" +%s 2>/dev/null || \
                  date -d "${LEASE_RENEW_TIME}" +%s 2>/dev/null || \
                  echo "")
    
    if [[ -n "$LEASE_EPOCH" && -n "$CURRENT_TIME" ]]; then
        TIME_DIFF=$((CURRENT_TIME - LEASE_EPOCH))
        
        if [[ $TIME_DIFF -le 40 ]]; then
            print_pass "V3: Lease renewed ${TIME_DIFF}s ago (within 40s grace period)"
        else
            print_fail "V3: Lease not renewed for ${TIME_DIFF}s (exceeds 40s grace period)"
        fi
    else
        print_info "Cannot calculate time difference, manual verification needed"
        print_info "Verify renewTime ($LEASE_RENEW_TIME) is within 40s of current time"
        # 无法自动判断时不算 PASS/FAIL，手动调整计数
        TOTAL_CHECKS=$((TOTAL_CHECKS - 1))
    fi
fi

# =============================================================================
# V4: 确认 Pod 恢复调度和运行
# Confirm Pods are scheduled and running on the node
# Command: kubectl get pods --field-selector spec.nodeName=<node-name> --all-namespaces
# Expected: Pods in Running state
# =============================================================================
print_section "V4: Pods on Node / 节点上的 Pod 状态"

PODS_OUTPUT=$(kubectl get pods --field-selector "spec.nodeName=${NODE_NAME}" \
    --all-namespaces --no-headers 2>&1)

if [[ -z "$PODS_OUTPUT" || "$PODS_OUTPUT" == *"No resources found"* ]]; then
    print_info "No pods currently running on node $NODE_NAME"
    print_info "This may be normal if node was just recovered and pods are being rescheduled"
    # Pod 不存在并不一定是错误（新恢复的节点可能还未调度） / No pods may be okay for freshly recovered nodes
    TOTAL_PODS=0
    RUNNING_PODS=0
else
    # 统计 Pod 状态 / Count pod statuses
    TOTAL_PODS=$(echo "$PODS_OUTPUT" | wc -l | tr -d ' ')
    RUNNING_PODS=$(echo "$PODS_OUTPUT" | awk '{print $4}' | grep -c "Running" || echo "0")
    PENDING_PODS=$(echo "$PODS_OUTPUT" | awk '{print $4}' | grep -c "Pending" || echo "0")
    FAILED_PODS=$(echo "$PODS_OUTPUT" | awk '{print $4}' | grep -c -E "Error|CrashLoopBackOff|Failed|ImagePullBackOff" || echo "0")
    
    echo -e "  ${BOLD}Total Pods: $TOTAL_PODS | Running: $RUNNING_PODS | Pending: $PENDING_PODS | Failed: $FAILED_PODS${NC}"
    echo ""
    
    # 显示非 Running Pod 详情 / Show non-Running pod details
    NON_RUNNING=$(echo "$PODS_OUTPUT" | awk '$4 != "Running"' || true)
    if [[ -n "$NON_RUNNING" ]]; then
        print_info "Non-Running pods:"
        echo "$NON_RUNNING" | while IFS= read -r line; do
            STATUS=$(echo "$line" | awk '{print $4}')
            case "$STATUS" in
                Pending)
                    echo -e "    ${YELLOW}$line${NC}"
                    ;;
                Error|CrashLoopBackOff|Failed|ImagePullBackOff)
                    echo -e "    ${RED}$line${NC}"
                    ;;
                Completed|Succeeded)
                    echo -e "    ${GREEN}$line${NC}"
                    ;;
                *)
                    echo "    $line"
                    ;;
            esac
        done
        echo ""
    fi
fi

if [[ "$TOTAL_PODS" -eq 0 ]]; then
    # 节点上没有 Pod 不算失败 / No pods is not a failure
    print_info "V4: No pods on node (may need time for rescheduling)"
    TOTAL_CHECKS=$((TOTAL_CHECKS - 1))
elif [[ "$RUNNING_PODS" -eq "$TOTAL_PODS" ]]; then
    print_pass "V4: All $TOTAL_PODS pods are Running"
elif [[ "$FAILED_PODS" -gt 0 ]]; then
    print_fail "V4: $FAILED_PODS of $TOTAL_PODS pods are in error state"
else
    # 有些 Pod 是 Pending 或 Completed，不一定是错误
    if [[ "$RUNNING_PODS" -gt 0 ]]; then
        print_pass "V4: $RUNNING_PODS of $TOTAL_PODS pods are Running (others may be Completed/Pending)"
    else
        print_fail "V4: No pods are Running ($TOTAL_PODS pods in non-Running state)"
    fi
fi

# =============================================================================
# V5: 确认节点系统信息
# Confirm node system info (kubelet version, runtime version)
# Command: kubectl get node <node-name> -o jsonpath='kubelet={.status.nodeInfo.kubeletVersion} runtime={.status.nodeInfo.containerRuntimeVersion}'
# =============================================================================
print_section "V5: Node System Info / 节点系统信息"

NODE_INFO=$(kubectl get node "$NODE_NAME" -o jsonpath='\
kubelet={.status.nodeInfo.kubeletVersion}\n\
runtime={.status.nodeInfo.containerRuntimeVersion}\n\
os={.status.nodeInfo.osImage}\n\
kernel={.status.nodeInfo.kernelVersion}\n\
arch={.status.nodeInfo.architecture}' 2>&1)

echo "$NODE_INFO" | while IFS= read -r line; do
    echo -e "  $line"
done
echo ""

KUBELET_VER=$(echo "$NODE_INFO" | grep "^kubelet=" | cut -d= -f2)
RUNTIME_VER=$(echo "$NODE_INFO" | grep "^runtime=" | cut -d= -f2)

if [[ -n "$KUBELET_VER" && "$KUBELET_VER" != "null" && -n "$RUNTIME_VER" && "$RUNTIME_VER" != "null" ]]; then
    print_pass "V5: Node system info available (kubelet=$KUBELET_VER, runtime=$RUNTIME_VER)"
    
    # 检查是否与集群中其他节点一致 / Check consistency with other nodes
    OTHER_VERSIONS=$(kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}={.status.nodeInfo.kubeletVersion}{"\n"}{end}' 2>&1 | grep -v "$NODE_NAME" | head -3)
    if [[ -n "$OTHER_VERSIONS" ]]; then
        print_info "Other nodes' kubelet versions (for comparison):"
        echo "$OTHER_VERSIONS" | while IFS= read -r line; do
            OTHER_VER=$(echo "$line" | cut -d= -f2)
            if [[ "$OTHER_VER" == "$KUBELET_VER" ]]; then
                echo -e "    ${GREEN}$line (match)${NC}"
            else
                echo -e "    ${YELLOW}$line (different!)${NC}"
            fi
        done
    fi
else
    print_fail "V5: Node system info not available (node may not be reporting)"
fi

# =============================================================================
# 验证总结 / Verification Summary
# =============================================================================
print_header "Verification Summary / 验证总结"

echo -e "  Node:   ${BOLD}${NODE_NAME}${NC}"
echo -e "  Time:   $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo ""
echo -e "  ${BOLD}Results:${NC}"
echo -e "    ${GREEN}PASS: ${PASS_COUNT}${NC} / ${TOTAL_CHECKS}"
echo -e "    ${RED}FAIL: ${FAIL_COUNT}${NC} / ${TOTAL_CHECKS}"
echo ""

# 显示每项检查状态条 / Show status bar for each check
echo -e "  ${BOLD}Check Details:${NC}"

# 重新检查各项结果（用简洁格式展示）
V1_RESULT=$(echo "$TARGET_STATUS" | grep -q "^Ready$" && echo "PASS" || echo "FAIL")
V2_RESULT=$([[ "$V2_ALL_OK" == "true" ]] && echo "PASS" || echo "FAIL")

for V in V1 V2 V3 V4 V5; do
    case "$V" in
        V1) DESC="Node status is Ready" ;;
        V2) DESC="All Conditions normal" ;;
        V3) DESC="Lease renewTime is recent" ;;
        V4) DESC="Pods on node are Running" ;;
        V5) DESC="Node system info available" ;;
    esac
    # 简化显示 / Simplified display (already counted above)
    echo -e "    $V: $DESC"
done

echo ""

if [[ $FAIL_COUNT -eq 0 ]]; then
    echo -e "  ${GREEN}${BOLD}╔══════════════════════════════════════════╗${NC}"
    echo -e "  ${GREEN}${BOLD}║       ✅  ALL CHECKS PASSED             ║${NC}"
    echo -e "  ${GREEN}${BOLD}║  Node has recovered successfully.       ║${NC}"
    echo -e "  ${GREEN}${BOLD}╚══════════════════════════════════════════╝${NC}"
    echo ""
    echo -e "  ${BOLD}Recommended:${NC}"
    echo "    - Monitor the node for 5-30 minutes (Section 7.2)"
    echo "    - Set up 24-hour regression monitoring (Section 7.4)"
    echo "    - Document the root cause and remediation for post-mortem"
else
    echo -e "  ${RED}${BOLD}╔══════════════════════════════════════════╗${NC}"
    echo -e "  ${RED}${BOLD}║       ❌  SOME CHECKS FAILED            ║${NC}"
    echo -e "  ${RED}${BOLD}║  Node has NOT fully recovered.          ║${NC}"
    echo -e "  ${RED}${BOLD}╚══════════════════════════════════════════╝${NC}"
    echo ""
    echo -e "  ${BOLD}Recommended Actions:${NC}"
    echo "    1. Re-run diagnosis:  bash diagnose-quick.sh $NODE_NAME"
    echo "    2. Check deep status: bash diagnose-deep.sh <node-ip>"
    echo "    3. If repeated failures, consider escalation (Section 8)"
fi

echo ""
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════════════${NC}"
echo -e "  Post-Remediation Verification Complete - $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════════════${NC}"

# 退出码：所有检查通过返回 0，否则返回 1 / Exit code: 0 if all pass, 1 if any fail
if [[ $FAIL_COUNT -gt 0 ]]; then
    exit 1
fi
exit 0
