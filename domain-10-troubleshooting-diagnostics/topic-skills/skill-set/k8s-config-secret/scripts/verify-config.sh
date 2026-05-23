#!/usr/bin/env bash
# =============================================================================
# K8s ConfigMap & Secret Failure - Post-Remediation Verification
#
# Usage: bash verify-config.sh <namespace> <pod-name>
# Risk: NONE (read-only)
# Source: SKILL-CFG-001 Section 7
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
    echo "Usage: bash verify-config.sh <namespace> <pod-name>"
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

print_header "K8s ConfigMap & Secret - Post-Remediation Verification"
echo -e "  Namespace:  ${BOLD}${NAMESPACE}${NC}"
echo -e "  Pod:        ${BOLD}${POD_NAME}${NC}"
echo -e "  Timestamp:  $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "  Checks:     V1-V4 (${TOTAL_CHECKS} total)"

# =============================================================================
# V1: Pod 不在 CreateContainerConfigError
# =============================================================================
print_section "V1: Pod Status"

POD_REASON=$(kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='{.status.reason}' 2>/dev/null || echo "")
POD_PHASE=$(kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='{.status.phase}' 2>/dev/null || echo "Unknown")

printf "  %-15s %s\n" "Phase:" "$POD_PHASE"
printf "  %-15s %s\n" "Reason:" "${POD_REASON:-N/A}"

if echo "$POD_REASON" | grep -qi "CreateContainerConfigError"; then
    print_fail "V1: Pod still in CreateContainerConfigError"
else
    print_pass "V1: Pod not in CreateContainerConfigError"
fi

# =============================================================================
# V2: 所有引用的 ConfigMap 存在
# =============================================================================
print_section "V2: ConfigMap Existence"

POD_JSON=$(kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o json)
CM_NAMES=$(echo "$POD_JSON" | jq -r '[.spec.volumes[]? | select(.configMap) | .configMap.name] + [.spec.containers[].envFrom[]? | select(.configMapRef) | .configMapRef.name] + [.spec.containers[].env[]? | select(.valueFrom.configMapKeyRef) | .valueFrom.configMapKeyRef.name] | unique | .[]')

if [[ -n "$CM_NAMES" ]]; then
    ALL_EXIST=true
    for cm in $CM_NAMES; do
        if ! kubectl get configmap "$cm" -n "$NAMESPACE" &>/dev/null; then
            ALL_EXIST=false
        fi
    done

    if [[ "$ALL_EXIST" == "true" ]]; then
        print_pass "V2: All referenced ConfigMaps exist"
    else
        print_fail "V2: Some referenced ConfigMaps missing"
    fi
else
    print_pass "V2: No ConfigMap references (OK)"
fi

# =============================================================================
# V3: 所有引用的 Secret 存在
# =============================================================================
print_section "V3: Secret Existence"

SECRET_NAMES=$(echo "$POD_JSON" | jq -r '[.spec.volumes[]? | select(.secret) | .secret.secretName] + [.spec.containers[].envFrom[]? | select(.secretRef) | .secretRef.name] + [.spec.containers[].env[]? | select(.valueFrom.secretKeyRef) | .valueFrom.secretKeyRef.name] + [.spec.imagePullSecrets[]? | .name] | unique | .[]')

if [[ -n "$SECRET_NAMES" ]]; then
    ALL_EXIST=true
    for sec in $SECRET_NAMES; do
        if ! kubectl get secret "$sec" -n "$NAMESPACE" &>/dev/null; then
            ALL_EXIST=false
        fi
    done

    if [[ "$ALL_EXIST" == "true" ]]; then
        pass_pass "V3: All referenced Secrets exist"
    else
        print_fail "V3: Some referenced Secrets missing"
    fi
else
    print_pass "V3: No Secret references (OK)"
fi

# =============================================================================
# V4: 所有需要的 key 存在
# =============================================================================
print_section "V4: Key Existence"

ALL_KEYS_OK=true

# Check ConfigMap keys
ENV_CM_KEYS=$(echo "$POD_JSON" | jq -r '.spec.containers[].env[]? | select(.valueFrom.configMapKeyRef) | "\(.valueFrom.configMapKeyRef.name):\(.valueFrom.configMapKeyRef.key)"')
if [[ -n "$ENV_CM_KEYS" ]]; then
    echo "$ENV_CM_KEYS" | while IFS=: read -r cmName cmKey; do
        if kubectl get configmap "$cmName" -n "$NAMESPACE" &>/dev/null; then
            KEY_EXISTS=$(kubectl get configmap "$cmName" -n "$NAMESPACE" -o json | jq -r --arg key "$cmKey" '.data | has($key)')
            if [[ "$KEY_EXISTS" != "true" ]]; then
                ALL_KEYS_OK=false
            fi
        else
            ALL_KEYS_OK=false
        fi
    done
fi

# Check Secret keys
ENV_SECRET_KEYS=$(echo "$POD_JSON" | jq -r '.spec.containers[].env[]? | select(.valueFrom.secretKeyRef) | "\(.valueFrom.secretKeyRef.name):\(.valueFrom.secretKeyRef.key)"')
if [[ -n "$ENV_SECRET_KEYS" ]]; then
    echo "$ENV_SECRET_KEYS" | while IFS=: read -r secName secKey; do
        if kubectl get secret "$secName" -n "$NAMESPACE" &>/dev/null; then
            KEY_EXISTS=$(kubectl get secret "$secName" -n "$NAMESPACE" -o json | jq -r --arg key "$secKey" '.data | has($key)')
            if [[ "$KEY_EXISTS" != "true" ]]; then
                ALL_KEYS_OK=false
            fi
        else
            ALL_KEYS_OK=false
        fi
    done
fi

if [[ "$ALL_KEYS_OK" == "true" ]]; then
    print_pass "V4: All required keys exist"
else
    print_fail "V4: Some required keys missing"
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
    echo -e "  ${GREEN}${BOLD}║  ConfigMap & Secret are healthy.       ║${NC}"
    echo -e "  ${GREEN}${BOLD}╚══════════════════════════════════════════╝${NC}"
else
    echo -e "  ${RED}${BOLD}╔══════════════════════════════════════════╗${NC}"
    echo -e "  ${RED}${BOLD}║       ❌  SOME CHECKS FAILED            ║${NC}"
    echo -e "  ${RED}${BOLD}║  Config/Secret NOT fully recovered.    ║${NC}"
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
