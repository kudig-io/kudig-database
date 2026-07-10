#!/usr/bin/env bash
# =============================================================================
# K8s Autoscaling Failure - Phase 1 Quick Diagnosis (Read-only)
#
# Usage: bash diagnose-quick.sh [namespace] [hpa-name]
# Risk: NONE (read-only kubectl operations)
# Source: SKILL-AUTO-001 D1.1-D1.5
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

NAMESPACE="${1:-all}"
HPA_NAME="${2:-}"

if ! command -v kubectl &>/dev/null; then
    echo -e "${RED}Error: kubectl is not installed.${NC}"
    exit 1
fi

if ! kubectl cluster-info &>/dev/null; then
    echo -e "${RED}Error: Cannot connect to cluster.${NC}"
    exit 1
fi

print_header "K8s Autoscaling Failure - Phase 1 Quick Diagnosis"
echo -e "  Namespace:  ${BOLD}${NAMESPACE}${NC}"
if [[ -n "$HPA_NAME" ]]; then
    echo -e "  HPA:        ${BOLD}${HPA_NAME}${NC}"
fi
echo -e "  Timestamp:  $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "  Risk Level: ${GREEN}NONE (read-only)${NC}"

# =============================================================================
# D1.1: metrics-server 状态
# =============================================================================
print_section "D1.1: Metrics Server / 指标服务状态"

MS_POD=$(kubectl get pods -n kube-system --no-headers 2>/dev/null | grep "metrics-server" | head -1 || true)

if [[ -z "$MS_POD" ]]; then
    print_error "metrics-server pod not found in kube-system"
    add_finding "D1.1: metrics-server not installed - RC-001"
else
    MS_STATUS=$(echo "$MS_POD" | awk '{print $3}')
    MS_READY=$(echo "$MS_POD" | awk '{print $2}')
    printf "  %-20s %s\n" "Pod:" "$(echo "$MS_POD" | awk '{print $1}')"
    printf "  %-20s %s\n" "Status:" "$MS_STATUS"
    printf "  %-20s %s\n" "Ready:" "$MS_READY"

    if [[ "$MS_STATUS" != "Running" ]]; then
        print_error "metrics-server is not Running"
        add_finding "D1.1: metrics-server status=$MS_STATUS - RC-001"
    else
        print_ok "metrics-server is Running"
    fi

    # 测试 metrics API
    if kubectl top nodes &>/dev/null; then
        print_ok "Metrics API is responding"
    else
        print_warn "Metrics API is not responding"
        add_finding "D1.1: Metrics API not responding - RC-001"
    fi
fi

# =============================================================================
# D1.2: HPA 状态检查
# =============================================================================
print_section "D1.2: HPA Status / 水平自动扩缩容状态"

if [[ -n "$HPA_NAME" && "$NAMESPACE" != "all" ]]; then
    HPA_LIST=$(kubectl get hpa "$HPA_NAME" -n "$NAMESPACE" --no-headers 2>/dev/null || true)
else
    if [[ "$NAMESPACE" == "all" ]]; then
        HPA_LIST=$(kubectl get hpa --all-namespaces --no-headers 2>/dev/null || true)
    else
        HPA_LIST=$(kubectl get hpa -n "$NAMESPACE" --no-headers 2>/dev/null || true)
    fi
fi

if [[ -z "$HPA_LIST" ]]; then
    print_info "No HPA resources found"
else
    echo -e "  ${BOLD}NAMESPACE NAME           REFERENCE       TARGETS   MIN MAX REPLICAS${NC}"
    echo "  ────────────────────────────────────────────────────────────"
    echo "$HPA_LIST" | while IFS= read -r line; do
        if echo "$line" | grep -q "<unknown>"; then
            echo -e "  ${RED}$line${NC}"
            HPA_N=$(echo "$line" | awk '{print $2}')
            add_finding "D1.2: HPA $HPA_N shows <unknown> metrics - RC-001 or RC-002"
        else
            echo "  $line"
        fi
    done
fi

# =============================================================================
# D1.3: VPA 状态检查
# =============================================================================
print_section "D1.3: VPA Status / 垂直自动扩缩容状态"

if [[ "$NAMESPACE" == "all" ]]; then
    VPA_LIST=$(kubectl get vpa --all-namespaces --no-headers 2>/dev/null || true)
else
    VPA_LIST=$(kubectl get vpa -n "$NAMESPACE" --no-headers 2>/dev/null || true)
fi

if [[ -z "$VPA_LIST" ]]; then
    print_info "No VPA resources found"
else
    echo -e "  ${BOLD}NAMESPACE NAME           MODE   TARGET       STATUS${NC}"
    echo "  ────────────────────────────────────────────────────────────"
    echo "$VPA_LIST" | while IFS= read -r line; do
        echo "  $line"
    done
fi

# =============================================================================
# D1.4: Cluster Autoscaler 状态
# =============================================================================
print_section "D1.4: Cluster Autoscaler / 集群自动扩缩容状态"

CA_POD=$(kubectl get pods -n kube-system --no-headers 2>/dev/null | grep "cluster-autoscaler" | head -1 || true)

if [[ -z "$CA_POD" ]]; then
    print_info "Cluster Autoscaler not found (may not be installed)"
else
    CA_STATUS=$(echo "$CA_POD" | awk '{print $3}')
    printf "  %-20s %s\n" "Pod:" "$(echo "$CA_POD" | awk '{print $1}')"
    printf "  %-20s %s\n" "Status:" "$CA_STATUS"

    if [[ "$CA_STATUS" != "Running" ]]; then
        print_error "Cluster Autoscaler is not Running"
        add_finding "D1.4: Cluster Autoscaler status=$CA_STATUS - RC-004"
    else
        print_ok "Cluster Autoscaler is Running"
    fi

    # 检查 CA 日志中的错误
    CA_POD_NAME=$(echo "$CA_POD" | awk '{print $1}')
    CA_ERRORS=$(kubectl logs "$CA_POD_NAME" -n kube-system --tail=30 2>/dev/null | grep -iE "error|fail|unable|denied" || true)
    if [[ -n "$CA_ERRORS" ]]; then
        print_warn "Cluster Autoscaler has recent errors"
        echo "$CA_ERRORS" | head -5 | sed 's/^/    /'
        add_finding "D1.4: Cluster Autoscaler errors - RC-004"
    fi
fi

# =============================================================================
# D1.5: Prometheus Adapter（如果使用自定义指标）
# =============================================================================
print_section "D1.5: Prometheus Adapter / 自定义指标适配器"

PA_POD=$(kubectl get pods -n kube-system --no-headers 2>/dev/null | grep "prometheus-adapter" | head -1 || true)

if [[ -n "$PA_POD" ]]; then
    PA_STATUS=$(echo "$PA_POD" | awk '{print $3}')
    printf "  %-20s %s\n" "Pod:" "$(echo "$PA_POD" | awk '{print $1}')"
    printf "  %-20s %s\n" "Status:" "$PA_STATUS"

    if [[ "$PA_STATUS" != "Running" ]]; then
        print_warn "Prometheus Adapter is not Running"
        add_finding "D1.5: Prometheus Adapter status=$PA_STATUS"
    else
        print_ok "Prometheus Adapter is Running"
    fi
else
    print_info "Prometheus Adapter not found (may not be needed)"
fi

# =============================================================================
# 诊断总结
# =============================================================================
print_header "Diagnosis Summary / 诊断总结"

echo -e "  Namespace: ${BOLD}${NAMESPACE}${NC}"
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
