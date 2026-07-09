#!/usr/bin/env bash
# =============================================================================
# K8s Performance Bottleneck - Post-Remediation Verification
#
# Usage: bash verify-performance.sh [namespace] [pod-name]
# Risk: NONE (read-only)
# Source: SKILL-PERF-001 Section 7
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

NAMESPACE="${1:-}"
POD_NAME="${2:-}"

if ! command -v kubectl &>/dev/null; then
    echo -e "${RED}Error: kubectl is not installed.${NC}"
    exit 1
fi

if ! kubectl cluster-info &>/dev/null; then
    echo -e "${RED}Error: Cannot connect to cluster.${NC}"
    exit 1
fi

print_header "K8s Performance - Post-Remediation Verification"
if [[ -n "$NAMESPACE" ]]; then
    echo -e "  Namespace:  ${BOLD}${NAMESPACE}${NC}"
fi
if [[ -n "$POD_NAME" ]]; then
    echo -e "  Pod:        ${BOLD}${POD_NAME}${NC}"
fi
echo -e "  Timestamp:  $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "  Checks:     V1-V4 (${TOTAL_CHECKS} total)"

# =============================================================================
# V1: 节点资源在正常范围
# =============================================================================
print_section "V1: Node Resources"

if kubectl top nodes &>/dev/null; then
    HIGH_USAGE=$(kubectl top nodes --no-headers 2>/dev/null | awk '{print $3}' | tr -d '%' | awk '$1 > 85 {print}' | wc -l | tr -d ' ')
    if [[ "$HIGH_USAGE" -eq 0 ]]; then
        pass_pass "V1: All nodes CPU usage below 85%"
    else
        print_fail "V1: $HIGH_USAGE node(s) still have high CPU usage"
    fi
else
    print_info "V1: Metrics API unavailable"
    TOTAL_CHECKS=$((TOTAL_CHECKS - 1))
fi

# =============================================================================
# V2: Pod 无 OOMKilled
# =============================================================================
print_section "V2: Pod OOM Status"

if [[ -n "$NAMESPACE" && -n "$POD_NAME" ]]; then
    OOM_STATUS=$(kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o json 2>/dev/null | jq -r '.status.containerStatuses[]?.lastState.terminated?.reason // "N/A"')
    if [[ "$OOM_STATUS" == "OOMKilled" ]]; then
        print_fail "V2: Pod was recently OOMKilled"
    else
        print_pass "V2: Pod not recently OOMKilled"
    fi
elif [[ -n "$NAMESPACE" ]]; then
    OOM_COUNT=$(kubectl get pods -n "$NAMESPACE" -o json 2>/dev/null | jq '[.items[] | select(.status.containerStatuses[]?.lastState.terminated?.reason == "OOMKilled")] | length')
    if [[ "$OOM_COUNT" -eq 0 ]]; then
        print_pass "V2: No OOMKilled pods in namespace"
    else
        print_fail "V2: $OOM_COUNT pod(s) recently OOMKilled"
    fi
else
    print_info "V2: No namespace specified"
    TOTAL_CHECKS=$((TOTAL_CHECKS - 1))
fi

# =============================================================================
# V3: Pod 重启次数低
# =============================================================================
print_section "V3: Pod Restarts"

if [[ -n "$NAMESPACE" && -n "$POD_NAME" ]]; then
    RESTARTS=$(kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='{.status.containerStatuses[0].restartCount}' 2>/dev/null || echo "0")
    if [[ "$RESTARTS" -lt 3 ]]; then
        print_pass "V3: Pod restart count is $RESTARTS (< 3)"
    else
        print_fail "V3: Pod restart count is $RESTARTS (>= 3)"
    fi
elif [[ -n "$NAMESPACE" ]]; then
    HIGH_RESTART=$(kubectl get pods -n "$NAMESPACE" -o json 2>/dev/null | jq '[.items[] | select(.status.containerStatuses[]?.restartCount >= 5)] | length')
    if [[ "$HIGH_RESTART" -eq 0 ]]; then
        print_pass "V3: No pods with high restart count"
    else
        print_fail "V3: $HIGH_RESTART pod(s) with >=5 restarts"
    fi
else
    print_info "V3: No namespace specified"
    TOTAL_CHECKS=$((TOTAL_CHECKS - 1))
fi

# =============================================================================
# V4: 节点无压力条件
# =============================================================================
print_section "V4: Node Pressure"

PRESSURE_COUNT=$(kubectl get nodes -o json 2>/dev/null | jq '[.items[].status.conditions[] | select(.status == "True" and (.type | test("Pressure"; "i")))] | length')

if [[ "$PRESSURE_COUNT" -eq 0 ]]; then
    print_pass "V4: No node pressure conditions"
else
    print_fail "V4: $PRESSURE_COUNT node pressure condition(s) active"
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
    echo -e "  ${GREEN}${BOLD}║  Performance issues resolved.          ║${NC}"
    echo -e "  ${GREEN}${BOLD}╚══════════════════════════════════════════╝${NC}"
else
    echo -e "  ${RED}${BOLD}╔══════════════════════════════════════════╗${NC}"
    echo -e "  ${RED}${BOLD}║       ❌  SOME CHECKS FAILED            ║${NC}"
    echo -e "  ${RED}${BOLD}║  Performance NOT fully recovered.      ║${NC}"
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
