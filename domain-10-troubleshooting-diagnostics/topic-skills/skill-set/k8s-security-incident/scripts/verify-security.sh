#!/usr/bin/env bash
# =============================================================================
# K8s Security Incident Response - Post-Remediation Verification
#
# Usage: bash verify-security.sh [namespace]
# Risk: NONE (read-only)
# Source: SKILL-SEC-002 Section 7
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

NAMESPACE="${1:-}"

if ! command -v kubectl &>/dev/null; then
    echo -e "${RED}Error: kubectl is not installed.${NC}"
    exit 1
fi

if ! kubectl cluster-info &>/dev/null; then
    echo -e "${RED}Error: Cannot connect to cluster.${NC}"
    exit 1
fi

print_header "K8s Security - Post-Remediation Verification"
if [[ -n "$NAMESPACE" ]]; then
    echo -e "  Namespace:  ${BOLD}${NAMESPACE}${NC}"
fi
echo -e "  Timestamp:  $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "  Checks:     V1-V5 (${TOTAL_CHECKS} total)"

# =============================================================================
# V1: 无特权容器
# =============================================================================
print_section "V1: Privileged Containers"

if [[ -n "$NAMESPACE" ]]; then
    PRIV_COUNT=$(kubectl get pods -n "$NAMESPACE" -o json 2>/dev/null | jq '[.items[] | select(.spec.containers[]?.securityContext?.privileged == true)] | length')
else
    PRIV_COUNT=$(kubectl get pods --all-namespaces -o json 2>/dev/null | jq '[.items[] | select(.spec.containers[]?.securityContext?.privileged == true)] | length')
fi

if [[ "$PRIV_COUNT" -eq 0 ]]; then
    print_pass "V1: No privileged containers found"
else
    print_fail "V1: $PRIV_COUNT privileged container(s) still present"
fi

# =============================================================================
# V2: 无主机命名空间共享
# =============================================================================
print_section "V2: Host Namespace Sharing"

if [[ -n "$NAMESPACE" ]]; then
    HOST_NS_COUNT=$(kubectl get pods -n "$NAMESPACE" -o json 2>/dev/null | jq '[.items[] | select(.spec.hostNetwork == true or .spec.hostPID == true or .spec.hostIPC == true)] | length')
else
    HOST_NS_COUNT=$(kubectl get pods --all-namespaces -o json 2>/dev/null | jq '[.items[] | select(.spec.hostNetwork == true or .spec.hostPID == true or .spec.hostIPC == true)] | length')
fi

if [[ "$HOST_NS_COUNT" -eq 0 ]]; then
    print_pass "V2: No host namespace sharing"
else
    print_fail "V2: $HOST_NS_COUNT pod(s) still sharing host namespaces"
fi

# =============================================================================
# V3: cluster-admin 绑定已审查
# =============================================================================
print_section "V3: Cluster-Admin Bindings"

ADMIN_COUNT=$(kubectl get clusterrolebinding -o json 2>/dev/null | jq '[.items[] | select(.roleRef.name == "cluster-admin")] | length')

if [[ "$ADMIN_COUNT" -eq 0 ]]; then
    print_pass "V3: No cluster-admin bindings"
else
    print_fail "V3: $ADMIN_COUNT cluster-admin binding(s) still exist"
fi

# =============================================================================
# V4: 无可疑镜像
# =============================================================================
print_section "V4: Suspicious Images"

if [[ -n "$NAMESPACE" ]]; then
    LATEST_COUNT=$(kubectl get pods -n "$NAMESPACE" -o json 2>/dev/null | jq -r '.items[].spec.containers[].image' | grep -c ":latest$" || echo "0")
else
    LATEST_COUNT=$(kubectl get pods --all-namespaces -o json 2>/dev/null | jq -r '.items[].spec.containers[].image' | grep -c ":latest$" || echo "0")
fi

if [[ "$LATEST_COUNT" -eq 0 ]]; then
    print_pass "V4: No images using 'latest' tag"
else
    print_fail "V4: $LATEST_COUNT image(s) still using 'latest' tag"
fi

# =============================================================================
# V5: ServiceAccount 默认令牌已审查
# =============================================================================
print_section "V5: Default ServiceAccount"

if [[ -n "$NAMESPACE" ]]; then
    AUTO_MOUNT=$(kubectl get sa default -n "$NAMESPACE" -o json 2>/dev/null | jq -r '.automountServiceAccountToken // true')
else
    AUTO_MOUNT=$(kubectl get sa default --all-namespaces -o json 2>/dev/null | jq -r '.items[] | select(.automountServiceAccountToken != false) | .metadata.name' | wc -l | tr -d ' ')
fi

if [[ "$AUTO_MOUNT" == "false" || "$AUTO_MOUNT" -eq 0 ]]; then
    print_pass "V5: default SA automount disabled"
else
    print_fail "V5: default SA still allows automount"
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
    echo -e "  ${GREEN}${BOLD}║  Security posture improved.            ║${NC}"
    echo -e "  ${GREEN}${BOLD}╚══════════════════════════════════════════╝${NC}"
else
    echo -e "  ${RED}${BOLD}╔══════════════════════════════════════════╗${NC}"
    echo -e "  ${RED}${BOLD}║       ❌  SOME CHECKS FAILED            ║${NC}"
    echo -e "  ${RED}${BOLD}║  Security issues remain.               ║${NC}"
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
