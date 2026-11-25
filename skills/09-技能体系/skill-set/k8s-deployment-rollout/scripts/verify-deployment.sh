#!/usr/bin/env bash
# =============================================================================
# K8s Deployment Rollout Failure - Post-Remediation Verification
# 修复后验证脚本 - 确认 Deployment 已恢复正常发布状态
#
# Usage: bash verify-deployment.sh <namespace> <deployment-name>
# Risk: NONE (read-only)
# Source: SKILL-DEPLOY-001 Section 7
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

# --- 参数验证 ---
if [[ $# -lt 2 ]]; then
    echo -e "${RED}Error: Missing required arguments.${NC}"
    echo ""
    echo "Usage: bash verify-deployment.sh <namespace> <deployment-name>"
    exit 1
fi

NAMESPACE="$1"
DEPLOY_NAME="$2"

if ! command -v kubectl &>/dev/null; then
    echo -e "${RED}Error: kubectl is not installed.${NC}"
    exit 1
fi

if ! kubectl cluster-info &>/dev/null; then
    echo -e "${RED}Error: Cannot connect to cluster.${NC}"
    exit 1
fi

if ! kubectl get deployment "$DEPLOY_NAME" -n "$NAMESPACE" &>/dev/null; then
    echo -e "${RED}Error: Deployment '$DEPLOY_NAME' not found in '$NAMESPACE'.${NC}"
    exit 1
fi

print_header "K8s Deployment Rollout - Post-Remediation Verification"
echo -e "  Namespace:  ${BOLD}${NAMESPACE}${NC}"
echo -e "  Deployment: ${BOLD}${DEPLOY_NAME}${NC}"
echo -e "  Timestamp:  $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "  Checks:     V1-V5 (${TOTAL_CHECKS} total)"

# =============================================================================
# V1: Deployment 状态 Ready >= Desired
# =============================================================================
print_section "V1: Deployment Replicas / 副本数检查"

DEPLOY_JSON=$(kubectl get deployment "$DEPLOY_NAME" -n "$NAMESPACE" -o json 2>&1)
DESIRED=$(echo "$DEPLOY_JSON" | jq -r '.spec.replicas // 0')
READY=$(echo "$DEPLOY_JSON" | jq -r '.status.readyReplicas // 0')
UPDATED=$(echo "$DEPLOY_JSON" | jq -r '.status.updatedReplicas // 0')
AVAILABLE=$(echo "$DEPLOY_JSON" | jq -r '.status.availableReplicas // 0')
UNAVAILABLE=$(echo "$DEPLOY_JSON" | jq -r '.status.unavailableReplicas // 0')

printf "  %-15s %s\n" "Desired:" "$DESIRED"
printf "  %-15s %s\n" "Ready:" "$READY"
printf "  %-15s %s\n" "Updated:" "$UPDATED"
printf "  %-15s %s\n" "Available:" "$AVAILABLE"
printf "  %-15s %s\n" "Unavailable:" "$UNAVAILABLE"
echo ""

if [[ "$READY" -ge "$DESIRED" && "$AVAILABLE" -ge "$DESIRED" && "$UNAVAILABLE" -eq 0 ]]; then
    print_pass "V1: All replicas ready and available ($READY/$DESIRED)"
else
    print_fail "V1: Replicas not fully ready (Ready=$READY, Available=$AVAILABLE, Unavailable=$UNAVAILABLE)"
fi

# =============================================================================
# V2: Generation 匹配（observed == spec）
# =============================================================================
print_section "V2: Generation Match / 版本一致性"

OBSERVED_GEN=$(echo "$DEPLOY_JSON" | jq -r '.status.observedGeneration // 0')
SPEC_GEN=$(echo "$DEPLOY_JSON" | jq -r '.metadata.generation // 0')

printf "  %-15s %s\n" "Observed:" "$OBSERVED_GEN"
printf "  %-15s %s\n" "Spec:" "$SPEC_GEN"
echo ""

if [[ "$OBSERVED_GEN" -eq "$SPEC_GEN" ]]; then
    print_pass "V2: Observed generation matches spec generation"
else
    print_fail "V2: Generation mismatch ($OBSERVED_GEN != $SPEC_GEN) - rollout may still be in progress"
fi

# =============================================================================
# V3: 没有旧 ReplicaSet 残留
# =============================================================================
print_section "V3: ReplicaSet Cleanup / ReplicaSet 清理"

RS_JSON=$(kubectl get rs -n "$NAMESPACE" -o json 2>/dev/null | jq -r '[.items[] | select(.metadata.ownerReferences[0].name == "'"$DEPLOY_NAME"'")]')
RS_COUNT=$(echo "$RS_JSON" | jq 'length')
OLD_RS_WITH_PODS=$(echo "$RS_JSON" | jq '[.[] | select((.status.replicas // 0) > 0 and (.status.readyReplicas // 0) == 0)] | length')

printf "  %-15s %s\n" "Total RS:" "$RS_COUNT"
printf "  %-15s %s\n" "Old RS (pods>0):" "$OLD_RS_WITH_PODS"
echo ""

if [[ "$OLD_RS_WITH_PODS" -eq 0 ]]; then
    print_pass "V3: No old ReplicaSets with remaining pods"
else
    print_fail "V3: $OLD_RS_WITH_PODS old ReplicaSet(s) still have pods"
fi

# =============================================================================
# V4: 所有 Pod Running
# =============================================================================
print_section "V4: Pod Health / Pod 健康状态"

PODS_JSON=$(kubectl get pods -n "$NAMESPACE" -l "app=$(echo "$DEPLOY_JSON" | jq -r '.spec.selector.matchLabels.app // empty')" -o json 2>/dev/null || echo '{"items":[]}')
TOTAL_PODS=$(echo "$PODS_JSON" | jq '.items | length')
RUNNING_PODS=$(echo "$PODS_JSON" | jq '[.items[] | select(.status.phase == "Running")] | length')
BAD_PODS=$(echo "$PODS_JSON" | jq '[.items[] | select(.status.phase | test("Failed|Error|Unknown|Pending"; "i"))] | length')

printf "  %-15s %s\n" "Total:" "$TOTAL_PODS"
printf "  %-15s %s\n" "Running:" "$RUNNING_PODS"
printf "  %-15s %s\n" "Bad/Problem:" "$BAD_PODS"
echo ""

if [[ "$BAD_PODS" -eq 0 && "$RUNNING_PODS" -ge "$DESIRED" ]]; then
    print_pass "V4: All pods are Running ($RUNNING_PODS/$TOTAL_PODS)"
else
    print_fail "V4: $BAD_PODS pod(s) in bad state"
fi

# =============================================================================
# V5: 无 ProgressDeadlineExceeded 事件
# =============================================================================
print_section "V5: Events / 事件检查"

EVENTS=$(kubectl get events -n "$NAMESPACE" --field-selector "involvedObject.name=${DEPLOY_NAME},involvedObject.kind=Deployment" --no-headers 2>/dev/null | grep -i "ProgressDeadlineExceeded\|FailedCreate\|FailedScheduling" || true)

if [[ -z "$EVENTS" ]]; then
    print_pass "V5: No rollout failure events found"
else
    print_fail "V5: Found failure events:"
    echo "$EVENTS" | sed 's/^/    /'
fi

# =============================================================================
# 验证总结
# =============================================================================
print_header "Verification Summary / 验证总结"

echo -e "  Deployment: ${BOLD}${NAMESPACE}/${DEPLOY_NAME}${NC}"
echo -e "  Time:       $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo ""
echo -e "  ${BOLD}Results:${NC}"
echo -e "    ${GREEN}PASS: ${PASS_COUNT}${NC} / ${TOTAL_CHECKS}"
echo -e "    ${RED}FAIL: ${FAIL_COUNT}${NC} / ${TOTAL_CHECKS}"
echo ""

if [[ $FAIL_COUNT -eq 0 ]]; then
    echo -e "  ${GREEN}${BOLD}╔══════════════════════════════════════════╗${NC}"
    echo -e "  ${GREEN}${BOLD}║       ✅  ALL CHECKS PASSED             ║${NC}"
    echo -e "  ${GREEN}${BOLD}║  Deployment rollout is healthy.        ║${NC}"
    echo -e "  ${GREEN}${BOLD}╚══════════════════════════════════════════╝${NC}"
else
    echo -e "  ${RED}${BOLD}╔══════════════════════════════════════════╗${NC}"
    echo -e "  ${RED}${BOLD}║       ❌  SOME CHECKS FAILED            ║${NC}"
    echo -e "  ${RED}${BOLD}║  Deployment has NOT fully recovered.   ║${NC}"
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
