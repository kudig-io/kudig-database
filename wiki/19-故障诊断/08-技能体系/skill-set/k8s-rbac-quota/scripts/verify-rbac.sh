#!/usr/bin/env bash
# =============================================================================
# K8s RBAC & Quota Failure - Post-Remediation Verification
#
# Usage: bash verify-rbac.sh <namespace> <service-account-name>
# Risk: NONE (read-only)
# Source: SKILL-RBAC-001 Section 7
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

if [[ $# -lt 2 ]]; then
    echo -e "${RED}Error: Missing required arguments.${NC}"
    echo ""
    echo "Usage: bash verify-rbac.sh <namespace> <service-account-name>"
    exit 1
fi

NAMESPACE="$1"
SA_NAME="$2"

if ! command -v kubectl &>/dev/null; then
    echo -e "${RED}Error: kubectl is not installed.${NC}"
    exit 1
fi

if ! kubectl cluster-info &>/dev/null; then
    echo -e "${RED}Error: Cannot connect to cluster.${NC}"
    exit 1
fi

print_header "K8s RBAC & Quota - Post-Remediation Verification"
echo -e "  Namespace:  ${BOLD}${NAMESPACE}${NC}"
echo -e "  SA:         ${BOLD}${SA_NAME}${NC}"
echo -e "  Timestamp:  $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "  Checks:     V1-V4 (${TOTAL_CHECKS} total)"

# =============================================================================
# V1: ServiceAccount 存在
# =============================================================================
print_section "V1: ServiceAccount Existence"

if kubectl get sa "$SA_NAME" -n "$NAMESPACE" &>/dev/null; then
    print_pass "V1: ServiceAccount '$SA_NAME' exists in '$NAMESPACE'"
else
    print_fail "V1: ServiceAccount '$SA_NAME' not found in '$NAMESPACE'"
fi

# =============================================================================
# V2: 至少一个 RoleBinding 或 ClusterRoleBinding
# =============================================================================
print_section "V2: RBAC Bindings"

BINDING_COUNT=0

RB_LIST=$(kubectl get rolebinding -n "$NAMESPACE" --no-headers 2>/dev/null | awk '{print $1}' || true)
for rb in $RB_LIST; do
    if kubectl get rolebinding "$rb" -n "$NAMESPACE" -o json 2>/dev/null | jq -e ".subjects[]? | select(.name == \"$SA_NAME\" and .namespace == \"$NAMESPACE\")" &>/dev/null; then
        BINDING_COUNT=$((BINDING_COUNT + 1))
    fi
done

CRB_LIST=$(kubectl get clusterrolebinding --no-headers 2>/dev/null | awk '{print $1}' || true)
for crb in $CRB_LIST; do
    if kubectl get clusterrolebinding "$crb" -o json 2>/dev/null | jq -e ".subjects[]? | select(.name == \"$SA_NAME\" and .namespace == \"$NAMESPACE\")" &>/dev/null; then
        BINDING_COUNT=$((BINDING_COUNT + 1))
    fi
done

if [[ "$BINDING_COUNT" -gt 0 ]]; then
    print_pass "V2: Found $BINDING_COUNT binding(s) for SA '$SA_NAME'"
else
    print_fail "V2: No RoleBinding or ClusterRoleBinding found for SA '$SA_NAME'"
fi

# =============================================================================
# V3: ResourceQuota 未超限
# =============================================================================
print_section "V3: ResourceQuota"

QUOTAS=$(kubectl get resourcequota -n "$NAMESPACE" --no-headers 2>/dev/null || true)

if [[ -z "$QUOTAS" ]]; then
    print_pass "V3: No ResourceQuota in namespace (no quota restrictions)"
else
    QUOTA_EXCEEDED=false
    QUOTA_DETAIL=$(kubectl describe resourcequota -n "$NAMESPACE" 2>/dev/null || true)
    if echo "$QUOTA_DETAIL" | grep -qiE "used\s*hard"; then
        # 简单检查：查找 used >= hard 的情况
        while IFS= read -r line; do
            if echo "$line" | grep -qE "^\s*[a-zA-Z]+\s+[0-9]+\s+[0-9]+"; then
                USED=$(echo "$line" | awk '{print $(NF-1)}')
                HARD=$(echo "$line" | awk '{print $NF}')
                if [[ "$USED" =~ ^[0-9]+$ && "$HARD" =~ ^[0-9]+$ && "$USED" -ge "$HARD" ]]; then
                    QUOTA_EXCEEDED=true
                fi
            fi
        done <<< "$(echo "$QUOTA_DETAIL" | grep -A 20 "Resource Quotas")"
    fi

    if [[ "$QUOTA_EXCEEDED" == "true" ]]; then
        print_fail "V3: ResourceQuota appears exceeded"
    else
        print_pass "V3: ResourceQuota within limits"
    fi
fi

# =============================================================================
# V4: 关键权限 can-i 通过
# =============================================================================
print_section "V4: Key Permissions (can-i)"

REQUIRED_CHECKS=(
    "list pods"
    "get pods"
)

ALL_PASS=true
for check in "${REQUIRED_CHECKS[@]}"; do
    ACTION=$(echo "$check" | awk '{print $1}')
    RESOURCE=$(echo "$check" | awk '{print $2}')
    RESULT=$(kubectl auth can-i "$ACTION" "$RESOURCE" --as="system:serviceaccount:${NAMESPACE}:${SA_NAME}" -n "$NAMESPACE" 2>/dev/null || echo "no")
    if [[ "$RESULT" == "yes" ]]; then
        printf "  ${GREEN}%-20s${NC} %s\n" "$check" "✓"
    else
        printf "  ${YELLOW}%-20s${NC} %s\n" "$check" "✗"
        ALL_PASS=false
    fi
done

if [[ "$ALL_PASS" == "true" ]]; then
    print_pass "V4: All key permissions granted"
else
    print_fail "V4: Some key permissions missing"
fi

# =============================================================================
# 验证总结
# =============================================================================
print_header "Verification Summary / 验证总结"

echo -e "  SA:   ${BOLD}${NAMESPACE}/${SA_NAME}${NC}"
echo -e "  Time: $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo ""
echo -e "  ${BOLD}Results:${NC}"
echo -e "    ${GREEN}PASS: ${PASS_COUNT}${NC} / ${TOTAL_CHECKS}"
echo -e "    ${RED}FAIL: ${FAIL_COUNT}${NC} / ${TOTAL_CHECKS}"
echo ""

if [[ $FAIL_COUNT -eq 0 ]]; then
    echo -e "  ${GREEN}${BOLD}╔══════════════════════════════════════════╗${NC}"
    echo -e "  ${GREEN}${BOLD}║       ✅  ALL CHECKS PASSED             ║${NC}"
    echo -e "  ${GREEN}${BOLD}║  RBAC & Quota are healthy.             ║${NC}"
    echo -e "  ${GREEN}${BOLD}╚══════════════════════════════════════════╝${NC}"
else
    echo -e "  ${RED}${BOLD}╔══════════════════════════════════════════╗${NC}"
    echo -e "  ${RED}${BOLD}║       ❌  SOME CHECKS FAILED            ║${NC}"
    echo -e "  ${RED}${BOLD}║  RBAC/Quota has NOT fully recovered.   ║${NC}"
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
