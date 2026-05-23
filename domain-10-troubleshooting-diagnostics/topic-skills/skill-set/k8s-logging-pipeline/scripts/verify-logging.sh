#!/usr/bin/env bash
# =============================================================================
# K8s Logging Pipeline Failure - Post-Remediation Verification
#
# Usage: bash verify-logging.sh [logging-namespace]
# Risk: NONE (read-only)
# Source: SKILL-LOG-001 Section 7
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

LOG_NS="${1:-logging}"

if ! command -v kubectl &>/dev/null; then
    echo -e "${RED}Error: kubectl is not installed.${NC}"
    exit 1
fi

if ! kubectl cluster-info &>/dev/null; then
    echo -e "${RED}Error: Cannot connect to cluster.${NC}"
    exit 1
fi

print_header "K8s Logging Pipeline - Post-Remediation Verification"
echo -e "  Namespace:  ${BOLD}${LOG_NS}${NC}"
echo -e "  Timestamp:  $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "  Checks:     V1-V4 (${TOTAL_CHECKS} total)"

# =============================================================================
# V1: 日志代理 Running
# =============================================================================
print_section "V1: Log Agents"

AGENT_PATTERNS=("fluentd" "fluent-bit" "filebeat" "vector" "promtail")
AGENT_RUNNING=false

for pattern in "${AGENT_PATTERNS[@]}"; do
    AGENT_POD=$(kubectl get pods -n "$LOG_NS" --no-headers 2>/dev/null | grep "$pattern" | grep "Running" | head -1 || true)
    if [[ -n "$AGENT_POD" ]]; then
        AGENT_RUNNING=true
        break
    fi
done

if [[ "$AGENT_RUNNING" == "true" ]]; then
    print_pass "V1: Log agent(s) are Running"
else
    print_fail "V1: No Running log agents found"
fi

# =============================================================================
# V2: 日志后端 Running
# =============================================================================
print_section "V2: Log Backend"

ES_POD=$(kubectl get pods -n "$LOG_NS" --no-headers 2>/dev/null | grep elasticsearch | grep "Running" | head -1 || true)
LOKI_POD=$(kubectl get pods -n "$LOG_NS" --no-headers 2>/dev/null | grep loki | grep "Running" | head -1 || true)

if [[ -n "$ES_POD" || -n "$LOKI_POD" ]]; then
    print_pass "V2: Log backend is Running"
else
    print_fail "V2: No Running log backend found"
fi

# =============================================================================
# V3: DaemonSet 完全调度
# =============================================================================
print_section "V3: DaemonSet Coverage"

DS_FOUND=false
DS_OK=false
for pattern in "fluentd" "fluent-bit" "filebeat" "promtail"; do
    DS=$(kubectl get ds -n "$LOG_NS" --no-headers 2>/dev/null | grep "$pattern" | head -1 || true)
    if [[ -n "$DS" ]]; then
        DS_FOUND=true
        DS_DESIRED=$(echo "$DS" | awk '{print $2}')
        DS_READY=$(echo "$DS" | awk '{print $4}')
        if [[ "$DS_DESIRED" == "$DS_READY" && "$DS_DESIRED" -gt 0 ]]; then
            DS_OK=true
        fi
        break
    fi
done

if [[ "$DS_FOUND" == "false" ]]; then
    print_info "V3: No log agent DaemonSet found"
    TOTAL_CHECKS=$((TOTAL_CHECKS - 1))
elif [[ "$DS_OK" == "true" ]]; then
    print_pass "V3: Log agent DaemonSet fully scheduled"
else
    print_fail "V3: Log agent DaemonSet not fully scheduled"
fi

# =============================================================================
# V4: 无严重错误日志
# =============================================================================
print_section "V4: Agent Log Errors"

ERROR_FOUND=false
for pattern in "${AGENT_PATTERNS[@]}"; do
    AGENT_POD=$(kubectl get pods -n "$LOG_NS" --no-headers 2>/dev/null | grep "$pattern" | grep "Running" | head -1 | awk '{print $1}' || true)
    if [[ -n "$AGENT_POD" ]]; then
        ERRORS=$(kubectl logs "$AGENT_POD" -n "$LOG_NS" --tail=20 2>/dev/null | grep -ciE "error|fail|reject" || echo "0")
        if [[ "$ERRORS" -gt 0 ]]; then
            ERROR_FOUND=true
        fi
        break
    fi
done

if [[ "$ERROR_FOUND" == "true" ]]; then
    print_fail "V4: Log agent has recent errors"
else
    print_pass "V4: Log agent logs look clean"
fi

# =============================================================================
# 验证总结
# =============================================================================
print_header "Verification Summary / 验证总结"

echo -e "  Namespace: ${BOLD}${LOG_NS}${NC}"
echo -e "  Time:      $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo ""
echo -e "  ${BOLD}Results:${NC}"
echo -e "    ${GREEN}PASS: ${PASS_COUNT}${NC} / ${TOTAL_CHECKS}"
echo -e "    ${RED}FAIL: ${FAIL_COUNT}${NC} / ${TOTAL_CHECKS}"
echo ""

if [[ $FAIL_COUNT -eq 0 ]]; then
    echo -e "  ${GREEN}${BOLD}╔══════════════════════════════════════════╗${NC}"
    echo -e "  ${GREEN}${BOLD}║       ✅  ALL CHECKS PASSED             ║${NC}"
    echo -e "  ${GREEN}${BOLD}║  Logging pipeline is healthy.          ║${NC}"
    echo -e "  ${GREEN}${BOLD}╚══════════════════════════════════════════╝${NC}"
else
    echo -e "  ${RED}${BOLD}╔══════════════════════════════════════════╗${NC}"
    echo -e "  ${RED}${BOLD}║       ❌  SOME CHECKS FAILED            ║${NC}"
    echo -e "  ${RED}${BOLD}║  Logging pipeline NOT fully recovered. ║${NC}"
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
