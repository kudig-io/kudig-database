#!/usr/bin/env bash
# =============================================================================
# K8s Image Pull Failure - Post-Remediation Verification
#
# Usage: bash verify-image.sh <namespace> <pod-name>
# Risk: NONE (read-only)
# Source: SKILL-IMG-001 Section 7
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
    echo "Usage: bash verify-image.sh <namespace> <pod-name>"
    exit 1
fi

NAMESPACE="$1"
POD_NAME="$2"

if ! command -v kubectl &>/dev/null; then
    echo -e "${RED}Error: kubectl is not installed.${NC}"
    exit 1
fi

if ! kubectl cluster-info &>/dev/null; then
    echo -e "${RED}Error: Cannot connect to cluster.${NC}"
    exit 1
fi

if ! kubectl get pod "$POD_NAME" -n "$NAMESPACE" &>/dev/null; then
    echo -e "${RED}Error: Pod '$POD_NAME' not found.${NC}"
    exit 1
fi

print_header "K8s Image Pull Failure - Post-Remediation Verification"
echo -e "  Namespace:  ${BOLD}${NAMESPACE}${NC}"
echo -e "  Pod:        ${BOLD}${POD_NAME}${NC}"
echo -e "  Timestamp:  $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "  Checks:     V1-V4 (${TOTAL_CHECKS} total)"

# =============================================================================
# V1: Pod 不在 ImagePullBackOff/ErrImagePull 状态
# =============================================================================
print_section "V1: Pod Status"

POD_PHASE=$(kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='{.status.phase}')
CONTAINER_STATES=$(kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o json | jq -r '.status.containerStatuses[]?.state | keys[0]')

printf "  %-15s %s\n" "Phase:" "$POD_PHASE"

BAD_STATES=$(echo "$CONTAINER_STATES" | grep -c "waiting" || echo "0")
IMAGE_ERRORS=$(kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o json | jq -r '.status.containerStatuses[]?.state.waiting.reason // empty' | grep -ciE "ImagePullBackOff|ErrImagePull" || echo "0")

if [[ "$IMAGE_ERRORS" -eq 0 ]]; then
    print_pass "V1: No ImagePullBackOff/ErrImagePull errors"
else
    print_fail "V1: $IMAGE_ERRORS container(s) still in image pull error state"
fi

# =============================================================================
# V2: 容器不在 waiting 状态（因镜像原因）
# =============================================================================
print_section "V2: Container States"

WAITING_REASONS=$(kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o json | jq -r '.status.containerStatuses[]?.state.waiting.reason // empty')

if [[ -z "$WAITING_REASONS" ]]; then
    print_pass "V2: No containers in waiting state"
else
    echo -e "  ${BOLD}Waiting reasons:${NC}"
    echo "$WAITING_REASONS" | sed 's/^/    /'
    if echo "$WAITING_REASONS" | grep -qiE "ImagePullBackOff|ErrImagePull"; then
        print_fail "V2: Containers still waiting due to image pull errors"
    else
        print_pass "V2: Containers waiting for non-image reasons (e.g., CrashLoopBackOff)"
    fi
fi

# =============================================================================
# V3: 无镜像相关失败事件
# =============================================================================
print_section "V3: Events"

EVENTS=$(kubectl get events -n "$NAMESPACE" --field-selector "involvedObject.name=${POD_NAME},involvedObject.kind=Pod" --no-headers 2>/dev/null | grep -iE "imagepull|pull.*denied|unauthorized|not found|manifest unknown" || true)

if [[ -z "$EVENTS" ]]; then
    print_pass "V3: No image pull failure events"
else
    print_fail "V3: Image pull failure events still present"
    echo "$EVENTS" | sed 's/^/    /'
fi

# =============================================================================
# V4: Pod 已进入 Running 或 Completed
# =============================================================================
print_section "V4: Pod Phase"

if [[ "$POD_PHASE" == "Running" || "$POD_PHASE" == "Succeeded" ]]; then
    print_pass "V4: Pod is in $POD_PHASE state"
else
    print_fail "V4: Pod is in $POD_PHASE state (expected Running or Succeeded)"
fi

# =============================================================================
# 验证总结
# =============================================================================
print_header "Verification Summary / 验证总结"

echo -e "  Pod:  ${BOLD}${NAMESPACE}/${POD_NAME}${NC}"
echo -e "  Time: $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo ""
echo -e "  ${BOLD}Results:${NC}"
echo -e "    ${GREEN}PASS: ${PASS_COUNT}${NC} / ${TOTAL_CHECKS}"
echo -e "    ${RED}FAIL: ${FAIL_COUNT}${NC} / ${TOTAL_CHECKS}"
echo ""

if [[ $FAIL_COUNT -eq 0 ]]; then
    echo -e "  ${GREEN}${BOLD}╔══════════════════════════════════════════╗${NC}"
    echo -e "  ${GREEN}${BOLD}║       ✅  ALL CHECKS PASSED             ║${NC}"
    echo -e "  ${GREEN}${BOLD}║  Image pull issue resolved.            ║${NC}"
    echo -e "  ${GREEN}${BOLD}╚══════════════════════════════════════════╝${NC}"
else
    echo -e "  ${RED}${BOLD}╔══════════════════════════════════════════╗${NC}"
    echo -e "  ${RED}${BOLD}║       ❌  SOME CHECKS FAILED            ║${NC}"
    echo -e "  ${RED}${BOLD}║  Image pull issue NOT resolved.        ║${NC}"
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
