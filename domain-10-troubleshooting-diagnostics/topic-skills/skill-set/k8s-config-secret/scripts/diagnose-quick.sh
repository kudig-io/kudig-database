#!/usr/bin/env bash
# =============================================================================
# K8s ConfigMap & Secret Failure - Phase 1 Quick Diagnosis (Read-only)
#
# Usage: bash diagnose-quick.sh <namespace> <pod-name>
# Risk: NONE (read-only kubectl operations)
# Source: SKILL-CFG-001 D1.1-D1.5
# =============================================================================
set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

FINDINGS=()
WARNINGS=()
ERRORS=()

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

print_ok() { echo -e "  ${GREEN}[OK]${NC} $1"; }
print_warn() { echo -e "  ${YELLOW}[WARN]${NC} $1"; WARNINGS+=("$1"); }
print_error() { echo -e "  ${RED}[ERROR]${NC} $1"; ERRORS+=("$1"); }
print_info() { echo -e "  ${BLUE}[INFO]${NC} $1"; }
add_finding() { FINDINGS+=("$1"); }

if [[ $# -lt 2 ]]; then
    echo -e "${RED}Error: Missing required arguments.${NC}"
    echo ""
    echo "Usage: bash diagnose-quick.sh <namespace> <pod-name>"
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
    echo -e "${RED}Error: Pod '$POD_NAME' not found in '$NAMESPACE'.${NC}"
    exit 1
fi

print_header "K8s ConfigMap & Secret Failure - Phase 1 Quick Diagnosis"
echo -e "  Namespace:  ${BOLD}${NAMESPACE}${NC}"
echo -e "  Pod:        ${BOLD}${POD_NAME}${NC}"
echo -e "  Timestamp:  $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "  Risk Level: ${GREEN}NONE (read-only)${NC}"

# =============================================================================
# D1.1: Pod 状态和事件
# =============================================================================
print_section "D1.1: Pod Status & Events / Pod 状态和事件"

POD_JSON=$(kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o json 2>/dev/null || true)
POD_PHASE=$(echo "$POD_JSON" | jq -r '.status.phase')
POD_STATUS_REASON=$(echo "$POD_JSON" | jq -r '.status.reason // "N/A"')

printf "  %-20s %s\n" "Phase:" "$POD_PHASE"
printf "  %-20s %s\n" "Reason:" "$POD_STATUS_REASON"

if echo "$POD_STATUS_REASON" | grep -qi "CreateContainerConfigError"; then
    add_finding "D1.1: CreateContainerConfigError - RC-001, RC-002, or RC-005"
fi
if echo "$POD_STATUS_REASON" | grep -qi "InvalidImageName\|ImageInspectError"; then
    add_finding "D1.1: Image config error"
fi

EVENTS=$(kubectl get events -n "$NAMESPACE" --field-selector "involvedObject.name=${POD_NAME},involvedObject.kind=Pod" --no-headers 2>/dev/null | tail -15 || true)
if [[ -n "$EVENTS" && "$EVENTS" != *"No resources found"* ]]; then
    echo "$EVENTS" | while IFS= read -r line; do
        if echo "$line" | grep -qiE "configmap|secret|mount|key.*not found|base64"; then
            echo -e "  ${YELLOW}$line${NC}"
        else
            echo "  $line"
        fi
    done

    if echo "$EVENTS" | grep -qi "configmap.*not found"; then
        add_finding "D1.1: ConfigMap not found event - RC-001"
    fi
    if echo "$EVENTS" | grep -qi "secret.*not found"; then
        add_finding "D1.1: Secret not found event - RC-001"
    fi
    if echo "$EVENTS" | grep -qi "key.*not found"; then
        add_finding "D1.1: Key not found in ConfigMap/Secret - RC-001"
    fi
fi

# =============================================================================
# D1.2: 引用的 ConfigMap 检查
# =============================================================================
print_section "D1.2: ConfigMap References / ConfigMap 引用"

CM_REFS=$(echo "$POD_JSON" | jq -r '.spec.volumes[]? | select(.configMap) | "\(.name):\(.configMap.name):\(.configMap.items[0]?.key // \"all\")"')
ENV_CM_REFS=$(echo "$POD_JSON" | jq -r '.spec.containers[].envFrom[]? | select(.configMapRef) | "\(.configMapRef.name)"')
ENV_CM_KEYS=$(echo "$POD_JSON" | jq -r '.spec.containers[].env[]? | select(.valueFrom.configMapKeyRef) | "\(.valueFrom.configMapKeyRef.name):\(.valueFrom.configMapKeyRef.key)"')

if [[ -n "$CM_REFS" || -n "$ENV_CM_REFS" || -n "$ENV_CM_KEYS" ]]; then
    echo -e "  ${BOLD}Volume ConfigMaps:${NC}"
    if [[ -n "$CM_REFS" ]]; then
        echo "$CM_REFS" | while IFS=: read -r volName cmName cmKey; do
            if kubectl get configmap "$cmName" -n "$NAMESPACE" &>/dev/null; then
                CM_DATA=$(kubectl get configmap "$cmName" -n "$NAMESPACE" -o json | jq -r '.data | keys | @json')
                print_ok "ConfigMap '$cmName' exists (keys: $CM_DATA)"

                if [[ "$cmKey" != "all" && -n "$cmKey" ]]; then
                    KEY_EXISTS=$(kubectl get configmap "$cmName" -n "$NAMESPACE" -o json | jq -r --arg key "$cmKey" '.data | has($key)')
                    if [[ "$KEY_EXISTS" != "true" ]]; then
                        print_error "Key '$cmKey' not found in ConfigMap '$cmName'"
                        add_finding "D1.2: Key '$cmKey' missing in ConfigMap '$cmName' - RC-001"
                    fi
                fi
            else
                print_error "ConfigMap '$cmName' not found"
                add_finding "D1.2: ConfigMap '$cmName' missing - RC-001"
            fi
        done
    else
        echo "    (none)"
    fi

    echo -e "  ${BOLD}EnvFrom ConfigMaps:${NC}"
    if [[ -n "$ENV_CM_REFS" ]]; then
        for cm in $ENV_CM_REFS; do
            if kubectl get configmap "$cm" -n "$NAMESPACE" &>/dev/null; then
                print_ok "ConfigMap '$cm' exists"
            else
                print_error "ConfigMap '$cm' not found"
                add_finding "D1.2: ConfigMap '$cm' missing - RC-001"
            fi
        done
    else
        echo "    (none)"
    fi

    echo -e "  ${BOLD}Env ConfigMap Keys:${NC}"
    if [[ -n "$ENV_CM_KEYS" ]]; then
        echo "$ENV_CM_KEYS" | while IFS=: read -r cmName cmKey; do
            if kubectl get configmap "$cmName" -n "$NAMESPACE" &>/dev/null; then
                KEY_EXISTS=$(kubectl get configmap "$cmName" -n "$NAMESPACE" -o json | jq -r --arg key "$cmKey" '.data | has($key)')
                if [[ "$KEY_EXISTS" == "true" ]]; then
                    print_ok "Key '$cmKey' exists in ConfigMap '$cmName'"
                else
                    print_error "Key '$cmKey' not found in ConfigMap '$cmName'"
                    add_finding "D1.2: Key '$cmKey' missing in ConfigMap '$cmName' - RC-001"
                fi
            else
                print_error "ConfigMap '$cmName' not found"
                add_finding "D1.2: ConfigMap '$cmName' missing - RC-001"
            fi
        done
    else
        echo "    (none)"
    fi
else
    print_info "No ConfigMap references found"
fi

# =============================================================================
# D1.3: 引用的 Secret 检查
# =============================================================================
print_section "D1.3: Secret References / Secret 引用"

SECRET_REFS=$(echo "$POD_JSON" | jq -r '.spec.volumes[]? | select(.secret) | "\(.name):\(.secret.secretName):\(.secret.items[0]?.key // \"all\")"')
ENV_SECRET_REFS=$(echo "$POD_JSON" | jq -r '.spec.containers[].envFrom[]? | select(.secretRef) | "\(.secretRef.name)"')
ENV_SECRET_KEYS=$(echo "$POD_JSON" | jq -r '.spec.containers[].env[]? | select(.valueFrom.secretKeyRef) | "\(.valueFrom.secretKeyRef.name):\(.valueFrom.secretKeyRef.key)"')
IMAGE_PULL_SECRETS=$(echo "$POD_JSON" | jq -r '.spec.imagePullSecrets[]?.name // empty')

if [[ -n "$SECRET_REFS" || -n "$ENV_SECRET_REFS" || -n "$ENV_SECRET_KEYS" || -n "$IMAGE_PULL_SECRETS" ]]; then
    echo -e "  ${BOLD}Volume Secrets:${NC}"
    if [[ -n "$SECRET_REFS" ]]; then
        echo "$SECRET_REFS" | while IFS=: read -r volName secName secKey; do
            if kubectl get secret "$secName" -n "$NAMESPACE" &>/dev/null; then
                SECRET_TYPE=$(kubectl get secret "$secName" -n "$NAMESPACE" -o jsonpath='{.type}')
                print_ok "Secret '$secName' exists (type: $SECRET_TYPE)"

                if [[ "$secKey" != "all" && -n "$secKey" ]]; then
                    KEY_EXISTS=$(kubectl get secret "$secName" -n "$NAMESPACE" -o json | jq -r --arg key "$secKey" '.data | has($key)')
                    if [[ "$KEY_EXISTS" != "true" ]]; then
                        print_error "Key '$secKey' not found in Secret '$secName'"
                        add_finding "D1.3: Key '$secKey' missing in Secret '$secName' - RC-001"
                    fi
                fi
            else
                print_error "Secret '$secName' not found"
                add_finding "D1.3: Secret '$secName' missing - RC-001"
            fi
        done
    else
        echo "    (none)"
    fi

    echo -e "  ${BOLD}EnvFrom Secrets:${NC}"
    if [[ -n "$ENV_SECRET_REFS" ]]; then
        for sec in $ENV_SECRET_REFS; do
            if kubectl get secret "$sec" -n "$NAMESPACE" &>/dev/null; then
                print_ok "Secret '$sec' exists"
            else
                print_error "Secret '$sec' not found"
                add_finding "D1.3: Secret '$sec' missing - RC-001"
            fi
        done
    else
        echo "    (none)"
    fi

    echo -e "  ${BOLD}Env Secret Keys:${NC}"
    if [[ -n "$ENV_SECRET_KEYS" ]]; then
        echo "$ENV_SECRET_KEYS" | while IFS=: read -r secName secKey; do
            if kubectl get secret "$secName" -n "$NAMESPACE" &>/dev/null; then
                KEY_EXISTS=$(kubectl get secret "$secName" -n "$NAMESPACE" -o json | jq -r --arg key "$secKey" '.data | has($key)')
                if [[ "$KEY_EXISTS" == "true" ]]; then
                    print_ok "Key '$secKey' exists in Secret '$secName'"
                else
                    print_error "Key '$secKey' not found in Secret '$secName'"
                    add_finding "D1.3: Key '$secKey' missing in Secret '$secName' - RC-001"
                fi
            else
                print_error "Secret '$secName' not found"
                add_finding "D1.3: Secret '$secName' missing - RC-001"
            fi
        done
    else
        echo "    (none)"
    fi

    if [[ -n "$IMAGE_PULL_SECRETS" ]]; then
        echo -e "  ${BOLD}imagePullSecrets:${NC}"
        for sec in $IMAGE_PULL_SECRETS; do
            if kubectl get secret "$sec" -n "$NAMESPACE" &>/dev/null; then
                print_ok "Secret '$sec' exists"
            else
                print_error "Secret '$sec' not found"
                add_finding "D1.3: imagePullSecret '$sec' missing"
            fi
        done
    fi
else
    print_info "No Secret references found"
fi

# =============================================================================
# D1.4: ConfigMap/Secret 大小检查
# =============================================================================
print_section "D1.4: Size Check / 大小检查"

# ConfigMap 大小限制约 1MiB
ALL_CM=$(echo "$CM_REFS" "$ENV_CM_REFS" | tr ' ' '\n' | sort -u)
for cm in $ALL_CM; do
    [[ -z "$cm" ]] && continue
    CM_SIZE=$(kubectl get configmap "$cm" -n "$NAMESPACE" -o json 2>/dev/null | wc -c)
    if [[ "$CM_SIZE" -gt 1048576 ]]; then
        print_warn "ConfigMap '$cm' size (${CM_SIZE} bytes) exceeds 1MiB limit"
        add_finding "D1.4: ConfigMap '$cm' too large - RC-003"
    fi
done

# Secret 大小限制约 1MiB
ALL_SECRETS=$(echo "$SECRET_REFS" "$ENV_SECRET_REFS" | tr ' ' '\n' | sort -u)
for sec in $ALL_SECRETS; do
    [[ -z "$sec" ]] && continue
    SEC_SIZE=$(kubectl get secret "$sec" -n "$NAMESPACE" -o json 2>/dev/null | wc -c)
    if [[ "$SEC_SIZE" -gt 1048576 ]]; then
        print_warn "Secret '$sec' size (${SEC_SIZE} bytes) exceeds 1MiB limit"
        add_finding "D1.4: Secret '$sec' too large - RC-003"
    fi
done

# =============================================================================
# D1.5: immutable 检查
# =============================================================================
print_section "D1.5: Immutable Check / 不可变性检查"

for cm in $ALL_CM; do
    [[ -z "$cm" ]] && continue
    IMMUTABLE=$(kubectl get configmap "$cm" -n "$NAMESPACE" -o json 2>/dev/null | jq -r '.immutable // false')
    if [[ "$IMMUTABLE" == "true" ]]; then
        print_info "ConfigMap '$cm' is immutable"
        add_finding "D1.5: ConfigMap '$cm' is immutable - modifications require recreation (RC-004)"
    fi
done

for sec in $ALL_SECRETS; do
    [[ -z "$sec" ]] && continue
    IMMUTABLE=$(kubectl get secret "$sec" -n "$NAMESPACE" -o json 2>/dev/null | jq -r '.immutable // false')
    if [[ "$IMMUTABLE" == "true" ]]; then
        print_info "Secret '$sec' is immutable"
        add_finding "D1.5: Secret '$sec' is immutable - modifications require recreation (RC-004)"
    fi
done

# =============================================================================
# 诊断总结
# =============================================================================
print_header "Diagnosis Summary / 诊断总结"

echo -e "  Pod: ${BOLD}${NAMESPACE}/${POD_NAME}${NC}"
echo ""

if [[ ${#FINDINGS[@]} -gt 0 ]]; then
    echo -e "  ${BOLD}Findings / 发现:${NC}"
    for i in "${!FINDINGS[@]}"; do
        echo -e "    $((i+1)). ${FINDINGS[$i]}"
    done
    echo ""
fi

if [[ ${#ERRORS[@]} -gt 0 ]]; then
    echo -e "  ${RED}${BOLD}Errors (${#ERRORS[@]}):${NC}"
    for err in "${ERRORS[@]}"; do
        echo -e "    ${RED}- $err${NC}"
    done
    echo ""
fi

echo -e "  ${BOLD}Recommended Next Steps / 建议下一步:${NC}"
echo -e "    ${GREEN}参考: reference/remediation-playbook.md${NC}"

echo ""
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════════════${NC}"
echo -e "  Phase 1 Quick Diagnosis Complete - $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════════════${NC}"
