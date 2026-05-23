#!/usr/bin/env bash
# =============================================================================
# K8s Autoscaling Failure - Post-Remediation Verification
#
# Usage: bash verify-autoscaling.sh [namespace] [hpa-name]
# Risk: NONE (read-only)
# Source: SKILL-AUTO-001 Section 7
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
TOTAL_CHECKS=4

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

NAMESPACE="${1:-all}"
HPA_NAME="${2:-}"

if ! command -v kubectl &>/dev/null; then
    echo -e "${RED}Error: kubectl is not installed.${NC}"
    exit 1
fi

if ! kubectl cluster-info &>/dev/null; then
    echo -e "${RED}Error: Cannot connect to cluster.${NC}"
    exit 1
fi

print_header "K8s Autoscaling - Post-Remediation Verification"
echo -e "  Namespace:  ${BOLD}${NAMESPACE}${NC}"
echo -e "  Timestamp:  $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "  Checks:     V1-V4 (${TOTAL_CHECKS} total)"

# =============================================================================
# V1: metrics-server 运行且 API 可用
# =============================================================================
print_section "V1: Metrics Server"

MS_POD=$(kubectl get pods -n kube-system --no-headers 2>/dev/null | grep "metrics-server" | head -1 || true)

if [[ -z "$MS_POD" ]]; then
    print_fail "V1: metrics-server not found"
else
    MS_STATUS=$(echo "$MS_POD" | awk '{print $3}')
    if [[ "$MS_STATUS" == "Running" ]]; then
        if kubectl top nodes &>/dev/null; then
            print_pass "V1: metrics-server Running and Metrics API responding"
        else
            print_fail "V1: metrics-server Running but Metrics API not responding"
        fi
    else
        print_fail "V1: metrics-server status=$MS_STATUS"
    fi
fi

# =============================================================================
# V2: HPA 无 <unknown>
# =============================================================================
print_section "V2: HPA Metrics"

if [[ -n "$HPA_NAME" && "$NAMESPACE" != "all" ]]; then
    HPA_OUT=$(kubectl get hpa "$HPA_NAME" -n "$NAMESPACE" --no-headers 2>/dev/null || true)
else
    if [[ "$NAMESPACE" == "all" ]]; then
        HPA_OUT=$(kubectl get hpa --all-namespaces --no-headers 2>/dev/null || true)
    else
        HPA_OUT=$(kubectl get hpa -n "$NAMESPACE" --no-headers 2>/dev/null || true)
    fi
fi

if [[ -z "$HPA_OUT" ]]; then
    print_info "V2: No HPA resources found"
    TOTAL_CHECKS=$((TOTAL_CHECKS - 1))
else
    UNKNOWN_COUNT=$(echo "$HPA_OUT" | grep -c "<unknown>" || echo "0")
    if [[ "$UNKNOWN_COUNT" -eq 0 ]]; then
        print_pass "V2: All HPA resources have known metrics"
    else
        print_fail "V2: $UNKNOWN_COUNT HPA(s) still show <unknown>"
    fi
fi

# =============================================================================
# V3: Cluster Autoscaler Running
# =============================================================================
print_section "V3: Cluster Autoscaler"

CA_POD=$(kubectl get pods -n kube-system --no-headers 2>/dev/null | grep "cluster-autoscaler" | head -1 || true)

if [[ -z "$CA_POD" ]]; then
    print_info "V3: Cluster Autoscaler not installed"
    TOTAL_CHECKS=$((TOTAL_CHECKS - 1))
else
    CA_STATUS=$(echo "$CA_POD" | awk '{print $3}')
    if [[ "$CA_STATUS" == "Running" ]]; then
        print_pass "V3: Cluster Autoscaler is Running"
    else
        print_fail "V3: Cluster Autoscaler status=$CA_STATUS"
    fi
fi

# =============================================================================
# V4: 节点资源可获取
# =============================================================================
print_section "V4: Node Metrics"

if kubectl top nodes &>/dev/null; then
    print_pass "V4: Node metrics available via Metrics API"
else
    print_fail "V4: Node metrics unavailable"
fi

# =============================================================================
# 验证总结
# =============================================================================
print_header "Verification Summary / 验证总结"

echo -e "  Namespace: ${BOLD}${NAMESPACE}${NC}"
echo -e "  Time:      $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo ""
echo -e "  ${BOLD}Results:${NC}"
echo -e "    ${GREEN}PASS: ${PASS_COUNT}${NC} / ${TOTAL_CHECKS}"
echo -e "    ${RED}FAIL: ${FAIL_COUNT}${NC} / ${TOTAL_CHECKS}"
echo ""

if [[ $FAIL_COUNT -eq 0 ]]; then
    echo -e "  ${GREEN}${BOLD}╔══════════════════════════════════════════╗${NC}"
    echo -e "  ${GREEN}${BOLD}║       ✅  ALL CHECKS PASSED             ║${NC}"
    echo -e "  ${GREEN}${BOLD}║  Autoscaling is healthy.               ║${NC}"
    echo -e "  ${GREEN}${BOLD}╚══════════════════════════════════════════╝${NC}"
else
    echo -e "  ${RED}${BOLD}╔══════════════════════════════════════════╗${NC}"
    echo -e "  ${RED}${BOLD}║       ❌  SOME CHECKS FAILED            ║${NC}"
    echo -e "  ${RED}${BOLD}║  Autoscaling NOT fully recovered.      ║${NC}"
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
