#!/usr/bin/env bash
# =============================================================================
# K8s Security Incident Response - Phase 1 Quick Audit (Read-only)
#
# Usage: bash diagnose-quick.sh [namespace]
# Risk: NONE (read-only kubectl operations)
# Source: SKILL-SEC-002 D1.1-D1.5
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

NAMESPACE="${1:-}"

if ! command -v kubectl &>/dev/null; then
    echo -e "${RED}Error: kubectl is not installed.${NC}"
    exit 1
fi

if ! kubectl cluster-info &>/dev/null; then
    echo -e "${RED}Error: Cannot connect to cluster.${NC}"
    exit 1
fi

print_header "K8s Security Incident Response - Phase 1 Quick Audit"
if [[ -n "$NAMESPACE" ]]; then
    echo -e "  Namespace:  ${BOLD}${NAMESPACE}${NC}"
fi
echo -e "  Timestamp:  $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "  Risk Level: ${GREEN}NONE (read-only audit)${NC}"
echo -e "  ${YELLOW}⚠️  This script performs read-only operations only${NC}"

# =============================================================================
# D1.1: 特权容器检查
# =============================================================================
print_section "D1.1: Privileged Containers / 特权容器"

if [[ -n "$NAMESPACE" ]]; then
    PRIV_PODS=$(kubectl get pods -n "$NAMESPACE" -o json 2>/dev/null | jq -r '.items[] | select(.spec.containers[]?.securityContext?.privileged == true) | "\(.metadata.name)"' | sort -u)
else
    PRIV_PODS=$(kubectl get pods --all-namespaces -o json 2>/dev/null | jq -r '.items[] | select(.spec.containers[]?.securityContext?.privileged == true) | "\(.metadata.namespace)/\(.metadata.name)"' | sort -u)
fi

if [[ -n "$PRIV_PODS" ]]; then
    echo -e "  ${BOLD}Privileged Pods:${NC}"
    echo "$PRIV_PODS" | while IFS= read -r line; do
        echo -e "    ${RED}$line${NC}"
    done
    PRIV_COUNT=$(echo "$PRIV_PODS" | wc -l | tr -d ' ')
    add_finding "D1.1: $PRIV_COUNT privileged pod(s) found - review for RC-001"
else
    print_ok "No privileged containers found"
fi

# =============================================================================
# D1.2: 共享主机命名空间/网络
# =============================================================================
print_section "D1.2: Host Namespace Sharing / 主机命名空间共享"

if [[ -n "$NAMESPACE" ]]; then
    HOST_NS_PODS=$(kubectl get pods -n "$NAMESPACE" -o json 2>/dev/null | jq -r '.items[] | select(.spec.hostNetwork == true or .spec.hostPID == true or .spec.hostIPC == true) | "\(.metadata.name):hostNetwork=\(.spec.hostNetwork):hostPID=\(.spec.hostPID):hostIPC=\(.spec.hostIPC)"')
else
    HOST_NS_PODS=$(kubectl get pods --all-namespaces -o json 2>/dev/null | jq -r '.items[] | select(.spec.hostNetwork == true or .spec.hostPID == true or .spec.hostIPC == true) | "\(.metadata.namespace)/\(.metadata.name):hostNetwork=\(.spec.hostNetwork):hostPID=\(.spec.hostPID):hostIPC=\(.spec.hostIPC)"')
fi

if [[ -n "$HOST_NS_PODS" ]]; then
    echo -e "  ${BOLD}Host Namespace Pods:${NC}"
    echo "$HOST_NS_PODS" | while IFS= read -r line; do
        echo -e "    ${YELLOW}$line${NC}"
    done
    add_finding "D1.2: Pods sharing host namespace - potential RC-001"
else
    print_ok "No pods sharing host namespaces"
fi

# =============================================================================
# D1.3: ClusterRole/cluster-admin 检查
# =============================================================================
print_section "D1.3: Cluster-Admin Bindings / 集群管理员绑定"

ADMIN_BINDINGS=$(kubectl get clusterrolebinding -o json 2>/dev/null | jq -r '.items[] | select(.roleRef.name == "cluster-admin") | "\(.metadata.name):\(.subjects[]? | "\(.kind):\(.name)")"')

if [[ -n "$ADMIN_BINDINGS" ]]; then
    echo -e "  ${BOLD}Cluster-Admin Bindings:${NC}"
    echo "$ADMIN_BINDINGS" | while IFS= read -r line; do
        echo "    $line"
    done
    add_finding "D1.3: cluster-admin bindings exist - review for RC-002"
else
    print_ok "No cluster-admin bindings found"
fi

# =============================================================================
# D1.4: 可疑镜像检查
# =============================================================================
print_section "D1.4: Suspicious Images / 可疑镜像"

if [[ -n "$NAMESPACE" ]]; then
    IMAGES=$(kubectl get pods -n "$NAMESPACE" -o json 2>/dev/null | jq -r '.items[].spec.containers[].image' | sort -u)
else
    IMAGES=$(kubectl get pods --all-namespaces -o json 2>/dev/null | jq -r '.items[].spec.containers[].image' | sort -u)
fi

if [[ -n "$IMAGES" ]]; then
    echo -e "  ${BOLD}Images in use:${NC}"
    echo "$IMAGES" | while IFS= read -r img; do
        if echo "$img" | grep -qE ":latest$|/busybox$|/alpine$|bash$|sh$"; then
            echo -e "    ${YELLOW}$img${NC}"
        else
            echo "    $img"
        fi
    done

    # 检查是否使用 latest 标签
    LATEST_COUNT=$(echo "$IMAGES" | grep -c ":latest$" || echo "0")
    if [[ "$LATEST_COUNT" -gt 0 ]]; then
        add_finding "D1.4: $LATEST_COUNT image(s) using 'latest' tag - supply chain risk"
    fi
else
    print_info "No images found"
fi

# =============================================================================
# D1.5: 近期事件审计
# =============================================================================
print_section "D1.5: Recent Events / 近期事件"

if [[ -n "$NAMESPACE" ]]; then
    RECENT_EVENTS=$(kubectl get events -n "$NAMESPACE" --sort-by=.lastTimestamp --no-headers 2>/dev/null | tail -20 || true)
else
    RECENT_EVENTS=$(kubectl get events --all-namespaces --sort-by=.lastTimestamp --no-headers 2>/dev/null | tail -20 || true)
fi

if [[ -n "$RECENT_EVENTS" ]]; then
    echo "$RECENT_EVENTS" | while IFS= read -r line; do
        if echo "$line" | grep -qiE "created.*secret|created.*serviceaccount|modified.*role|exec.*container"; then
            echo -e "  ${YELLOW}$line${NC}"
        else
            echo "  $line"
        fi
    done
fi

# =============================================================================
# 诊断总结
# =============================================================================
print_header "Audit Summary / 审计总结"

if [[ -n "$NAMESPACE" ]]; then
    echo -e "  Namespace: ${BOLD}${NAMESPACE}${NC}"
fi
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
echo -e "    ${YELLOW}⚠️ All remediation actions require security team approval${NC}"
echo -e "    ${GREEN}参考: reference/remediation-playbook.md${NC}"

echo ""
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════════════${NC}"
echo -e "  Phase 1 Quick Audit Complete - $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════════════${NC}"
