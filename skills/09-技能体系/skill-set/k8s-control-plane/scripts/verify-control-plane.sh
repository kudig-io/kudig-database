#!/usr/bin/env bash
# =============================================================================
# K8s Control Plane Failure - Post-Remediation Verification
#
# Usage: bash verify-control-plane.sh [context]
# Risk: NONE (read-only)
# Source: SKILL-CTRL-001 Section 7
# =============================================================================
set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

PASS_COUNT=0
FAIL_COUNT=0
TOTAL_CHECKS=5

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

KUBECTL="kubectl"
if [[ $# -ge 1 ]]; then
    KUBECTL="kubectl --context=$1"
fi

if ! command -v kubectl &>/dev/null; then
    echo -e "${RED}Error: kubectl is not installed.${NC}"
    exit 1
fi

print_header "K8s Control Plane - Post-Remediation Verification"
echo -e "  Timestamp:  $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "  Checks:     V1-V5 (${TOTAL_CHECKS} total)"

# =============================================================================
# V1: API Server 可达
# =============================================================================
print_section "V1: API Server Reachable"

if $KUBECTL cluster-info &>/dev/null; then
    print_pass "V1: API Server is reachable"
else
    print_fail "V1: API Server is NOT reachable"
fi

# =============================================================================
# V2: 控制平面节点 Ready
# =============================================================================
print_section "V2: Control Plane Nodes Ready"

CP_NODES=$($KUBECTL get nodes -l node-role.kubernetes.io/control-plane --no-headers 2>/dev/null || \
           $KUBECTL get nodes -l node-role.kubernetes.io/master --no-headers 2>/dev/null || true)

if [[ -n "$CP_NODES" ]]; then
    CP_COUNT=$(echo "$CP_NODES" | wc -l | tr -d ' ')
    CP_READY=$(echo "$CP_NODES" | grep -c "Ready" || echo "0")
    CP_NOTREADY=$(echo "$CP_NODES" | grep -c "NotReady" || echo "0")

    printf "  %-15s %s\n" "Total:" "$CP_COUNT"
    printf "  %-15s %s\n" "Ready:" "$CP_READY"
    printf "  %-15s %s\n" "NotReady:" "$CP_NOTREADY"
    echo ""

    if [[ "$CP_NOTREADY" -eq 0 ]]; then
        print_pass "V2: All $CP_COUNT control plane nodes are Ready"
    else
        print_fail "V2: $CP_NOTREADY control plane node(s) are NotReady"
    fi
else
    print_info "V2: No control plane nodes found with standard labels"
    TOTAL_CHECKS=$((TOTAL_CHECKS - 1))
fi

# =============================================================================
# V3: kube-system 核心 Pod Running
# =============================================================================
print_section "V3: Core kube-system Pods"

if $KUBECTL get ns kube-system &>/dev/null; then
    CORE_COMPONENTS=("kube-apiserver" "etcd" "kube-scheduler" "kube-controller-manager")
    CORE_OK=0
    CORE_FAIL=0

    for comp in "${CORE_COMPONENTS[@]}"; do
        COMP_POD=$($KUBECTL get pods -n kube-system --no-headers 2>/dev/null | grep "$comp" | head -1 || true)
        if [[ -n "$COMP_POD" ]]; then
            COMP_STATUS=$(echo "$COMP_POD" | awk '{print $3}')
            if [[ "$COMP_STATUS" == "Running" || "$COMP_STATUS" == "Completed" ]]; then
                printf "  ${GREEN}%-30s${NC} %s\n" "$comp" "✓ $COMP_STATUS"
                CORE_OK=$((CORE_OK + 1))
            else
                printf "  ${RED}%-30s${NC} %s\n" "$comp" "✗ $COMP_STATUS"
                CORE_FAIL=$((CORE_FAIL + 1))
            fi
        else
            print_info "$comp pod not found (may be managed differently)"
        fi
    done

    echo ""
    if [[ "$CORE_FAIL" -eq 0 ]]; then
        print_pass "V3: All core control plane pods are healthy"
    else
        print_fail "V3: $CORE_FAIL core component(s) not healthy"
    fi
else
    print_fail "V3: Cannot access kube-system namespace"
fi

# =============================================================================
# V4: etcd 健康
# =============================================================================
print_section "V4: etcd Health"

ETCD_PODS=$($KUBECTL get pods -n kube-system --no-headers 2>/dev/null | grep etcd | awk '{print $1}' || true)

if [[ -n "$ETCD_PODS" ]]; then
    ETCD_HEALTHY=0
    ETCD_UNHEALTHY=0
    for etcd_pod in $ETCD_PODS; do
        HEALTH=$($KUBECTL exec "$etcd_pod" -n kube-system -- etcdctl endpoint health 2>/dev/null || echo "unhealthy")
        if echo "$HEALTH" | grep -qi "healthy"; then
            ETCD_HEALTHY=$((ETCD_HEALTHY + 1))
        else
            ETCD_UNHEALTHY=$((ETCD_UNHEALTHY + 1))
        fi
    done

    if [[ "$ETCD_UNHEALTHY" -eq 0 ]]; then
        print_pass "V4: All etcd endpoints healthy ($ETCD_HEALTHY)"
    else
        print_fail "V4: $ETCD_UNHEALTHY etcd endpoint(s) unhealthy"
    fi
else
    print_info "V4: No etcd pods in kube-system (external etcd)"
    TOTAL_CHECKS=$((TOTAL_CHECKS - 1))
fi

# =============================================================================
# V5: 节点列表可正常获取
# =============================================================================
print_section "V5: Node List Accessible"

NODES=$($KUBECTL get nodes --no-headers 2>/dev/null || true)
if [[ -n "$NODES" ]]; then
    NODE_COUNT=$(echo "$NODES" | wc -l | tr -d ' ')
    print_pass "V5: Can list nodes ($NODE_COUNT total)"
else
    print_fail "V5: Cannot list nodes"
fi

# =============================================================================
# 验证总结
# =============================================================================
print_header "Verification Summary / 验证总结"

echo -e "  Time: $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo ""
echo -e "  ${BOLD}Results:${NC}"
echo -e "    ${GREEN}PASS: ${PASS_COUNT}${NC} / ${TOTAL_CHECKS}"
echo -e "    ${RED}FAIL: ${FAIL_COUNT}${NC} / ${TOTAL_CHECKS}"
echo ""

if [[ $FAIL_COUNT -eq 0 ]]; then
    echo -e "  ${GREEN}${BOLD}╔══════════════════════════════════════════╗${NC}"
    echo -e "  ${GREEN}${BOLD}║       ✅  ALL CHECKS PASSED             ║${NC}"
    echo -e "  ${GREEN}${BOLD}║  Control plane is healthy.             ║${NC}"
    echo -e "  ${GREEN}${BOLD}╚══════════════════════════════════════════╝${NC}"
else
    echo -e "  ${RED}${BOLD}╔══════════════════════════════════════════╗${NC}"
    echo -e "  ${RED}${BOLD}║       ❌  SOME CHECKS FAILED            ║${NC}"
    echo -e "  ${RED}${BOLD}║  Control plane NOT fully recovered.    ║${NC}"
    echo -e "  ${RED}${BOLD}╚══════════════════════════════════════════╝${NC}"
fi

echo ""
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════════════${NC}"
echo -e "  Verification Complete - $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════════════${NC}"

if [[ $FAIL_COUNT -gt 0 ]]; then
    exit 1
fi
exit 0
