#!/usr/bin/env bash
# =============================================================================
# K8s Monitoring & Alerting Failure - Post-Remediation Verification
#
# Usage: bash verify-monitoring.sh [monitoring-namespace]
# Risk: NONE (read-only)
# Source: SKILL-MON-001 Section 7
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

MON_NS="${1:-monitoring}"

if ! command -v kubectl &>/dev/null; then
    echo -e "${RED}Error: kubectl is not installed.${NC}"
    exit 1
fi

if ! kubectl cluster-info &>/dev/null; then
    echo -e "${RED}Error: Cannot connect to cluster.${NC}"
    exit 1
fi

print_header "K8s Monitoring & Alerting - Post-Remediation Verification"
echo -e "  Namespace:  ${BOLD}${MON_NS}${NC}"
echo -e "  Timestamp:  $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "  Checks:     V1-V5 (${TOTAL_CHECKS} total)"

# =============================================================================
# V1: Prometheus Running
# =============================================================================
print_section "V1: Prometheus"

PROM_POD=$(kubectl get pods -n "$MON_NS" --no-headers 2>/dev/null | grep prometheus | grep -v operator | head -1 || true)
if [[ -n "$PROM_POD" ]]; then
    PROM_STATUS=$(echo "$PROM_POD" | awk '{print $3}')
    if [[ "$PROM_STATUS" == "Running" ]]; then
        print_pass "V1: Prometheus is Running"
    else
        print_fail "V1: Prometheus status=$PROM_STATUS"
    fi
else
    print_fail "V1: Prometheus pod not found"
fi

# =============================================================================
# V2: Grafana Running
# =============================================================================
print_section "V2: Grafana"

GRAFANA_POD=$(kubectl get pods -n "$MON_NS" --no-headers 2>/dev/null | grep grafana | head -1 || true)
if [[ -n "$GRAFANA_POD" ]]; then
    GRAFANA_STATUS=$(echo "$GRAFANA_POD" | awk '{print $3}')
    if [[ "$GRAFANA_STATUS" == "Running" ]]; then
        print_pass "V2: Grafana is Running"
    else
        print_fail "V2: Grafana status=$GRAFANA_STATUS"
    fi
else
    print_fail "V2: Grafana pod not found"
fi

# =============================================================================
# V3: Alertmanager Running
# =============================================================================
print_section "V3: Alertmanager"

AM_POD=$(kubectl get pods -n "$MON_NS" --no-headers 2>/dev/null | grep alertmanager | head -1 || true)
if [[ -n "$AM_POD" ]]; then
    AM_STATUS=$(echo "$AM_POD" | awk '{print $3}')
    if [[ "$AM_STATUS" == "Running" ]]; then
        print_pass "V3: Alertmanager is Running"
    else
        print_fail "V3: Alertmanager status=$AM_STATUS"
    fi
else
    print_fail "V3: Alertmanager pod not found"
fi

# =============================================================================
# V4: Prometheus 有 Active Targets
# =============================================================================
print_section "V4: Active Targets"

if [[ -n "$PROM_POD" ]]; then
    PROM_POD_NAME=$(echo "$PROM_POD" | awk '{print $1}')
    ACTIVE_TARGETS=$(kubectl exec "$PROM_POD_NAME" -n "$MON_NS" -c prometheus -- wget -qO- http://localhost:9090/api/v1/targets 2>/dev/null | jq '.data.activeTargets | length' || echo "0")
    if [[ "$ACTIVE_TARGETS" -gt 0 ]]; then
        print_pass "V4: Prometheus has $ACTIVE_TARGETS active targets"
    else
        print_fail "V4: Prometheus has 0 active targets"
    fi
else
    print_info "V4: Cannot check targets (Prometheus not found)"
    TOTAL_CHECKS=$((TOTAL_CHECKS - 1))
fi

# =============================================================================
# V5: 无规则评估错误
# =============================================================================
print_section "V5: Rule Evaluation"

if [[ -n "$PROM_POD" ]]; then
    PROM_POD_NAME=$(echo "$PROM_POD" | awk '{print $1}')
    RULE_ERRORS=$(kubectl logs "$PROM_POD_NAME" -n "$MON_NS" -c prometheus --tail=50 2>/dev/null | grep -ci "rule evaluation" || echo "0")
    if [[ "$RULE_ERRORS" -eq 0 ]]; then
        print_pass "V5: No rule evaluation errors in recent logs"
    else
        print_fail "V5: $RULE_ERRORS rule evaluation error(s) found"
    fi
else
    print_info "V5: Cannot check rules (Prometheus not found)"
    TOTAL_CHECKS=$((TOTAL_CHECKS - 1))
fi

# =============================================================================
# 验证总结
# =============================================================================
print_header "Verification Summary / 验证总结"

echo -e "  Namespace: ${BOLD}${MON_NS}${NC}"
echo -e "  Time:      $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo ""
echo -e "  ${BOLD}Results:${NC}"
echo -e "    ${GREEN}PASS: ${PASS_COUNT}${NC} / ${TOTAL_CHECKS}"
echo -e "    ${RED}FAIL: ${FAIL_COUNT}${NC} / ${TOTAL_CHECKS}"
echo ""

if [[ $FAIL_COUNT -eq 0 ]]; then
    echo -e "  ${GREEN}${BOLD}╔══════════════════════════════════════════╗${NC}"
    echo -e "  ${GREEN}${BOLD}║       ✅  ALL CHECKS PASSED             ║${NC}"
    echo -e "  ${GREEN}${BOLD}║  Monitoring is healthy.                ║${NC}"
    echo -e "  ${GREEN}${BOLD}╚══════════════════════════════════════════╝${NC}"
else
    echo -e "  ${RED}${BOLD}╔══════════════════════════════════════════╗${NC}"
    echo -e "  ${RED}${BOLD}║       ❌  SOME CHECKS FAILED            ║${NC}"
    echo -e "  ${RED}${BOLD}║  Monitoring NOT fully recovered.       ║${NC}"
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
