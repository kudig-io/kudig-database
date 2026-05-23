#!/usr/bin/env bash
# =============================================================================
# K8s RBAC & Quota Failure - Phase 1 Quick Diagnosis (Read-only)
#
# Usage: bash diagnose-quick.sh <namespace> <resource-type> <resource-name>
#   resource-type: pod, deployment, serviceaccount, etc.
# Risk: NONE (read-only kubectl operations)
# Source: SKILL-RBAC-001 D1.1-D1.5
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

if [[ $# -lt 3 ]]; then
    echo -e "${RED}Error: Missing required arguments.${NC}"
    echo ""
    echo "Usage: bash diagnose-quick.sh <namespace> <resource-type> <resource-name>"
    echo ""
    echo "Examples:"
    echo "  bash diagnose-quick.sh default pod my-app-xxx"
    echo "  bash diagnose-quick.sh production deployment api-gateway"
    exit 1
fi

NAMESPACE="$1"
RES_TYPE="$2"
RES_NAME="$3"

if ! command -v kubectl &>/dev/null; then
    echo -e "${RED}Error: kubectl is not installed.${NC}"
    exit 1
fi

if ! kubectl cluster-info &>/dev/null; then
    echo -e "${RED}Error: Cannot connect to cluster.${NC}"
    exit 1
fi

if ! kubectl get ns "$NAMESPACE" &>/dev/null; then
    echo -e "${RED}Error: Namespace '$NAMESPACE' not found.${NC}"
    exit 1
fi

print_header "K8s RBAC & Quota Failure - Phase 1 Quick Diagnosis"
echo -e "  Namespace:  ${BOLD}${NAMESPACE}${NC}"
echo -e "  Resource:   ${BOLD}${RES_TYPE}/${RES_NAME}${NC}"
echo -e "  Timestamp:  $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "  Risk Level: ${GREEN}NONE (read-only)${NC}"

# =============================================================================
# D1.1: 目标资源事件检查
# =============================================================================
print_section "D1.1: Resource Events / 资源事件"

EVENTS=$(kubectl get events -n "$NAMESPACE" --field-selector "involvedObject.name=${RES_NAME},involvedObject.kind=$(echo "$RES_TYPE" | awk '{print toupper(substr($0,1,1)) substr($0,2)}')" --sort-by=.lastTimestamp --no-headers 2>&1 | tail -20 || true)

if [[ -z "$EVENTS" || "$EVENTS" == *"No resources found"* ]]; then
    print_info "No recent events for this resource"
else
    echo "$EVENTS" | while IFS= read -r line; do
        if echo "$line" | grep -qiE "forbidden|unauthorized|denied|exceeded|failed"; then
            echo -e "  ${RED}$line${NC}"
        else
            echo "  $line"
        fi
    done

    if echo "$EVENTS" | grep -qi "forbidden"; then
        add_finding "D1.1: 'forbidden' event detected - RC-001 (RBAC)"
    fi
    if echo "$EVENTS" | grep -qi "exceeded"; then
        add_finding "D1.1: Quota exceeded event detected - RC-002"
    fi
    if echo "$EVENTS" | grep -qi "serviceaccount.*not found"; then
        add_finding "D1.1: ServiceAccount not found - RC-003"
    fi
fi

# =============================================================================
# D1.2: ResourceQuota 状态
# =============================================================================
print_section "D1.2: ResourceQuota Status / 资源配额状态"

QUOTAS=$(kubectl get resourcequota -n "$NAMESPACE" --no-headers 2>/dev/null || true)

if [[ -z "$QUOTAS" ]]; then
    print_info "No ResourceQuota in namespace $NAMESPACE"
else
    echo -e "  ${BOLD}NAME       AGE${NC}"
    echo "$QUOTAS" | while IFS= read -r line; do
        echo "  $line"
    done

    echo ""
    print_info "Quota usage details:"
    kubectl describe resourcequota -n "$NAMESPACE" 2>/dev/null | grep -A 20 "Resource Quotas" | sed 's/^/  /' || true

    # 检查是否已用尽
    QUOTA_DETAIL=$(kubectl describe resourcequota -n "$NAMESPACE" 2>/dev/null || true)
    if echo "$QUOTA_DETAIL" | grep -qiE "used\s*hard"; then
        if echo "$QUOTA_DETAIL" | awk '/used.*hard/ {for(i=1;i<=NF;i++) if ($i ~ /^[0-9]+$/ && $(i+1) ~ /^[0-9]+$/ && $i >= $(i+1)) print "EXCEEDED"}' | grep -q "EXCEEDED"; then
            print_error "ResourceQuota appears exhausted"
            add_finding "D1.2: ResourceQuota exhausted - RC-002"
        fi
    fi
fi

# =============================================================================
# D1.3: ServiceAccount 检查
# =============================================================================
print_section "D1.3: ServiceAccount / 服务账号检查"

if [[ "$RES_TYPE" == "pod" ]]; then
    SA=$(kubectl get pod "$RES_NAME" -n "$NAMESPACE" -o jsonpath='{.spec.serviceAccountName}' 2>/dev/null || echo "default")
else
    SA=$(kubectl get "$RES_TYPE" "$RES_NAME" -n "$NAMESPACE" -o jsonpath='{.spec.template.spec.serviceAccountName}' 2>/dev/null || echo "default")
fi

printf "  %-20s %s\n" "ServiceAccount:" "$SA"

if ! kubectl get sa "$SA" -n "$NAMESPACE" &>/dev/null; then
    print_error "ServiceAccount '$SA' not found in namespace '$NAMESPACE'"
    add_finding "D1.3: ServiceAccount missing - RC-003"
else
    print_ok "ServiceAccount '$SA' exists"

    # 检查 SA 的 secrets
    SA_SECRETS=$(kubectl get sa "$SA" -n "$NAMESPACE" -o jsonpath='{.secrets}' 2>/dev/null || echo "[]")
    if [[ "$SA_SECRETS" == "[]" || -z "$SA_SECRETS" ]]; then
        print_warn "ServiceAccount has no secrets (may be using TokenRequest API)"
    fi
fi

# =============================================================================
# D1.4: RoleBinding 和 ClusterRoleBinding 检查
# =============================================================================
print_section "D1.4: RBAC Bindings / 权限绑定检查"

if [[ "$SA" != "default" ]]; then
    BINDINGS=$(kubectl get rolebinding -n "$NAMESPACE" --no-headers 2>/dev/null | awk '{print $1}' || true)
    CRBINDINGS=$(kubectl get clusterrolebinding --no-headers 2>/dev/null | awk '{print $1}' || true)

    SA_MATCH_COUNT=0
    for rb in $BINDINGS; do
        if kubectl get rolebinding "$rb" -n "$NAMESPACE" -o json 2>/dev/null | jq -e ".subjects[]? | select(.name == \"$SA\" and .namespace == \"$NAMESPACE\")" &>/dev/null; then
            ROLE_REF=$(kubectl get rolebinding "$rb" -n "$NAMESPACE" -o jsonpath='{.roleRef.name}')
            print_ok "RoleBinding '$rb' -> Role '$ROLE_REF'"
            SA_MATCH_COUNT=$((SA_MATCH_COUNT + 1))
        fi
    done

    for crb in $CRBINDINGS; do
        if kubectl get clusterrolebinding "$crb" -o json 2>/dev/null | jq -e ".subjects[]? | select(.name == \"$SA\" and .namespace == \"$NAMESPACE\")" &>/dev/null; then
            ROLE_REF=$(kubectl get clusterrolebinding "$crb" -o jsonpath='{.roleRef.name}')
            print_ok "ClusterRoleBinding '$crb' -> ClusterRole '$ROLE_REF'"
            SA_MATCH_COUNT=$((SA_MATCH_COUNT + 1))
        fi
    done

    if [[ "$SA_MATCH_COUNT" -eq 0 ]]; then
        print_warn "No RoleBinding or ClusterRoleBinding found for ServiceAccount '$SA'"
        add_finding "D1.4: No RBAC bindings for SA '$SA' - RC-004"
    fi
fi

# =============================================================================
# D1.5: 权限自查（auth can-i）
# =============================================================================
print_section "D1.5: Auth Can-I / 权限自查"

if [[ "$SA" != "default" ]]; then
    COMMON_CHECKS=(
        "create pods"
        "create deployments"
        "create services"
        "get secrets"
        "create configmaps"
        "list pods"
    )

    for check in "${COMMON_CHECKS[@]}"; do
        ACTION=$(echo "$check" | awk '{print $1}')
        RESOURCE=$(echo "$check" | awk '{print $2}')
        RESULT=$(kubectl auth can-i "$ACTION" "$RESOURCE" --as="system:serviceaccount:${NAMESPACE}:${SA}" -n "$NAMESPACE" 2>/dev/null || echo "no")
        if [[ "$RESULT" == "yes" ]]; then
            printf "  ${GREEN}%-30s${NC} %s\n" "$check" "$RESULT"
        else
            printf "  ${YELLOW}%-30s${NC} %s\n" "$check" "$RESULT"
        fi
    done
else
    print_info "Using default ServiceAccount - check if namespace has default restrictions"
fi

# =============================================================================
# 诊断总结
# =============================================================================
print_header "Diagnosis Summary / 诊断总结"

echo -e "  Resource: ${BOLD}${NAMESPACE}/${RES_TYPE}/${RES_NAME}${NC}"
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

if [[ ${#WARNINGS[@]} -gt 0 ]]; then
    echo -e "  ${YELLOW}${BOLD}Warnings (${#WARNINGS[@]}):${NC}"
    for warn in "${WARNINGS[@]}"; do
        echo -e "    ${YELLOW}- $warn${NC}"
    done
    echo ""
fi

echo -e "  ${BOLD}Recommended Next Steps / 建议下一步:${NC}"
echo -e "    ${GREEN}参考: reference/remediation-playbook.md${NC}"

echo ""
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════════════${NC}"
echo -e "  Phase 1 Quick Diagnosis Complete - $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════════════${NC}"
