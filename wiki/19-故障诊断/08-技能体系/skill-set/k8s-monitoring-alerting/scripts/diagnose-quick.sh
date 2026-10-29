#!/usr/bin/env bash
# =============================================================================
# K8s Monitoring & Alerting Failure - Phase 1 Quick Diagnosis (Read-only)
#
# Usage: bash diagnose-quick.sh [monitoring-namespace]
#   Default namespace: monitoring
# Risk: NONE (read-only kubectl operations)
# Source: SKILL-MON-001 D1.1-D1.5
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

MON_NS="${1:-monitoring}"

if ! command -v kubectl &>/dev/null; then
    echo -e "${RED}Error: kubectl is not installed.${NC}"
    exit 1
fi

if ! kubectl cluster-info &>/dev/null; then
    echo -e "${RED}Error: Cannot connect to cluster.${NC}"
    exit 1
fi

print_header "K8s Monitoring & Alerting Failure - Phase 1 Quick Diagnosis"
echo -e "  Namespace:  ${BOLD}${MON_NS}${NC}"
echo -e "  Timestamp:  $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "  Risk Level: ${GREEN}NONE (read-only)${NC}"

# =============================================================================
# D1.1: 核心组件 Pod 状态
# =============================================================================
print_section "D1.1: Core Components / 核心组件状态"

COMPONENTS=("prometheus" "grafana" "alertmanager")
for comp in "${COMPONENTS[@]}"; do
    COMP_POD=$(kubectl get pods -n "$MON_NS" --no-headers 2>/dev/null | grep "$comp" | head -1 || true)
    if [[ -n "$COMP_POD" ]]; then
        COMP_STATUS=$(echo "$COMP_POD" | awk '{print $3}')
        COMP_READY=$(echo "$COMP_POD" | awk '{print $2}')
        COMP_NAME=$(echo "$COMP_POD" | awk '{print $1}')

        printf "  ${BOLD}%-20s${NC} %-10s %s\n" "$comp:" "$COMP_STATUS" "($COMP_NAME)"

        if [[ "$COMP_STATUS" != "Running" ]]; then
            print_error "$comp pod is $COMP_STATUS"
            add_finding "D1.1: $comp pod $COMP_STATUS - may be RC-001 (OOM/resource)"
        elif [[ "$COMP_READY" != "1/1" && "$COMP_READY" != "2/2" && "$COMP_READY" != "3/3" ]]; then
            print_warn "$comp pod not fully ready ($COMP_READY)"
            add_finding "D1.1: $comp pod not fully ready"
        else
            print_ok "$comp is Running and Ready"
        fi
    else
        print_warn "$comp pod not found in $MON_NS"
    fi
done

# =============================================================================
# D1.2: Prometheus 存储检查
# =============================================================================
print_section "D1.2: Prometheus Storage / 存储检查"

PROM_POD=$(kubectl get pods -n "$MON_NS" --no-headers 2>/dev/null | grep prometheus | grep -v operator | head -1 | awk '{print $1}' || true)

if [[ -n "$PROM_POD" ]]; then
    # 检查 PVC 使用率
    PROM_PVC=$(kubectl get pvc -n "$MON_NS" --no-headers 2>/dev/null | grep prometheus | head -1 || true)
    if [[ -n "$PROM_PVC" ]]; then
        PVC_NAME=$(echo "$PROM_PVC" | awk '{print $1}')
        PVC_STATUS=$(echo "$PROM_PVC" | awk '{print $2}')
        PVC_CAPACITY=$(echo "$PROM_PVC" | awk '{print $3}')
        printf "  ${BOLD}PVC:${NC} %s (%s, %s)\n" "$PVC_NAME" "$PVC_STATUS" "$PVC_CAPACITY"

        if [[ "$PVC_STATUS" != "Bound" ]]; then
            print_error "Prometheus PVC is $PVC_STATUS"
            add_finding "D1.2: Prometheus PVC $PVC_STATUS - RC-001"
        fi
    fi

    # 检查 Prometheus 容器内存储
    DISK_USAGE=$(kubectl exec "$PROM_POD" -n "$MON_NS" -c prometheus -- df -h /prometheus 2>/dev/null || echo "")
    if [[ -n "$DISK_USAGE" ]]; then
        USAGE_PCT=$(echo "$DISK_USAGE" | tail -1 | awk '{print $5}' | tr -d '%')
        printf "  ${BOLD}Disk Usage:${NC} %s%%\n" "$USAGE_PCT"
        if [[ "$USAGE_PCT" -gt 85 ]]; then
            print_warn "Prometheus disk usage is ${USAGE_PCT}%"
            add_finding "D1.2: Prometheus disk nearly full - RC-001"
        fi
    fi
else
    print_info "Prometheus pod not found"
fi

# =============================================================================
# D1.3: ServiceMonitor / PodMonitor 检查
# =============================================================================
print_section "D1.3: ServiceMonitors / 服务监控器"

SM_LIST=$(kubectl get servicemonitor --all-namespaces --no-headers 2>/dev/null | head -10 || true)

if [[ -z "$SM_LIST" ]]; then
    print_info "No ServiceMonitors found"
else
    echo -e "  ${BOLD}NAMESPACE NAME           ENDPOINTS AGE${NC}"
    echo "  ────────────────────────────────────────────────────────────"
    echo "$SM_LIST" | while IFS= read -r line; do
        echo "  $line"
    done

    # 检查 Prometheus 是否能发现 target
    if [[ -n "$PROM_POD" ]]; then
        ACTIVE_TARGETS=$(kubectl exec "$PROM_POD" -n "$MON_NS" -c prometheus -- wget -qO- http://localhost:9090/api/v1/targets 2>/dev/null | jq '.data.activeTargets | length' || echo "0")
        DROPPED_TARGETS=$(kubectl exec "$PROM_POD" -n "$MON_NS" -c prometheus -- wget -qO- http://localhost:9090/api/v1/targets 2>/dev/null | jq '.data.droppedTargets | length' || echo "0")
        printf "  ${BOLD}Active Targets:${NC} %s\n" "$ACTIVE_TARGETS"
        printf "  ${BOLD}Dropped Targets:${NC} %s\n" "$DROPPED_TARGETS"

        if [[ "$ACTIVE_TARGETS" == "0" ]]; then
            add_finding "D1.3: Prometheus has 0 active targets - RC-002"
        fi
    fi
fi

# =============================================================================
# D1.4: 告警规则检查
# =============================================================================
print_section "D1.4: Alert Rules / 告警规则"

PROMRULE_LIST=$(kubectl get prometheusrules --all-namespaces --no-headers 2>/dev/null | head -10 || true)

if [[ -z "$PROMRULE_LIST" ]]; then
    print_info "No PrometheusRule CRDs found"
else
    echo -e "  ${BOLD}NAMESPACE NAME           AGE${NC}"
    echo "  ────────────────────────────────────────────────────────────"
    echo "$PROMRULE_LIST" | while IFS= read -r line; do
        echo "  $line"
    done
fi

# 检查规则评估错误
if [[ -n "$PROM_POD" ]]; then
    RULE_ERRORS=$(kubectl logs "$PROM_POD" -n "$MON_NS" -c prometheus --tail=50 2>/dev/null | grep -i "rule evaluation" | tail -5 || true)
    if [[ -n "$RULE_ERRORS" ]]; then
        print_warn "Prometheus has rule evaluation errors"
        echo "$RULE_ERRORS" | sed 's/^/    /'
        add_finding "D1.4: Prometheus rule evaluation errors - RC-005"
    fi
fi

# =============================================================================
# D1.5: Alertmanager 配置检查
# =============================================================================
print_section "D1.5: Alertmanager Config / 告警管理器配置"

AM_POD=$(kubectl get pods -n "$MON_NS" --no-headers 2>/dev/null | grep alertmanager | head -1 | awk '{print $1}' || true)

if [[ -n "$AM_POD" ]]; then
    # 检查 Alertmanager 状态
    AM_STATUS=$(kubectl exec "$AM_POD" -n "$MON_NS" -- wget -qO- http://localhost:9093/api/v2/status 2>/dev/null | jq -r '.cluster.status // "unknown"' || echo "unknown")
    printf "  ${BOLD}Cluster Status:${NC} %s\n" "$AM_STATUS"

    # 检查接收器配置
    RECEIVERS=$(kubectl exec "$AM_POD" -n "$MON_NS" -- wget -qO- http://localhost:9093/api/v2/status 2>/dev/null | jq -r '.config.original' | grep -c "receiver" || echo "0")
    printf "  ${BOLD}Receivers:${NC} %s\n" "$RECEIVERS"

    if [[ "$AM_STATUS" != "ready" && "$AM_STATUS" != "unknown" ]]; then
        add_finding "D1.5: Alertmanager cluster status is $AM_STATUS - RC-004"
    fi
else
    print_info "Alertmanager pod not found"
fi

# =============================================================================
# 诊断总结
# =============================================================================
print_header "Diagnosis Summary / 诊断总结"

echo -e "  Namespace: ${BOLD}${MON_NS}${NC}"
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
