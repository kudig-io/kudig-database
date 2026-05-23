#!/usr/bin/env bash
# =============================================================================
# K8s Performance Bottleneck - Phase 1 Quick Diagnosis (Read-only)
#
# Usage: bash diagnose-quick.sh [namespace] [pod-name]
# Risk: NONE (read-only kubectl operations)
# Source: SKILL-PERF-001 D1.1-D1.6
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
POD_NAME="${2:-}"

if ! command -v kubectl &>/dev/null; then
    echo -e "${RED}Error: kubectl is not installed.${NC}"
    exit 1
fi

if ! kubectl cluster-info &>/dev/null; then
    echo -e "${RED}Error: Cannot connect to cluster.${NC}"
    exit 1
fi

print_header "K8s Performance Bottleneck - Phase 1 Quick Diagnosis"
if [[ -n "$NAMESPACE" ]]; then
    echo -e "  Namespace:  ${BOLD}${NAMESPACE}${NC}"
fi
if [[ -n "$POD_NAME" ]]; then
    echo -e "  Pod:        ${BOLD}${POD_NAME}${NC}"
fi
echo -e "  Timestamp:  $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "  Risk Level: ${GREEN}NONE (read-only)${NC}"

# =============================================================================
# D1.1: 节点资源概览
# =============================================================================
print_section "D1.1: Node Resources / 节点资源"

if kubectl top nodes &>/dev/null; then
    NODE_TOP=$(kubectl top nodes --no-headers 2>/dev/null || true)
    echo -e "  ${BOLD}NAME                    CPU(cores)   CPU%   MEMORY(bytes)   MEMORY%${NC}"
    echo "  ────────────────────────────────────────────────────────────"
    echo "$NODE_TOP" | while IFS= read -r line; do
        CPU_PCT=$(echo "$line" | awk '{print $3}')
        MEM_PCT=$(echo "$line" | awk '{print $5}')
        CPU_NUM=${CPU_PCT%%%}
        MEM_NUM=${MEM_PCT%%%}

        if [[ "$CPU_NUM" -gt 90 || "$MEM_NUM" -gt 90 ]]; then
            echo -e "  ${RED}$line${NC}"
            NODE_NAME=$(echo "$line" | awk '{print $1}')
            if [[ "$CPU_NUM" -gt 90 ]]; then
                add_finding "D1.1: Node $NODE_NAME CPU ${CPU_PCT} - RC-003"
            fi
            if [[ "$MEM_NUM" -gt 90 ]]; then
                add_finding "D1.1: Node $NODE_NAME Memory ${MEM_PCT} - RC-003"
            fi
        elif [[ "$CPU_NUM" -gt 70 || "$MEM_NUM" -gt 70 ]]; then
            echo -e "  ${YELLOW}$line${NC}"
        else
            echo "  $line"
        fi
    done
else
    print_warn "Metrics API not available - cannot get node resource usage"
fi

# =============================================================================
# D1.2: Pod 资源使用
# =============================================================================
print_section "D1.2: Pod Resources / Pod 资源使用"

if [[ -n "$NAMESPACE" && -n "$POD_NAME" ]]; then
    if kubectl top pod "$POD_NAME" -n "$NAMESPACE" &>/dev/null; then
        POD_TOP=$(kubectl top pod "$POD_NAME" -n "$NAMESPACE" --no-headers 2>/dev/null || true)
        echo "  $POD_TOP"

        # 获取资源 limit
        POD_LIMITS=$(kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o json | jq -r '.spec.containers[0].resources.limits')
        echo -e "  ${BOLD}Limits:${NC}"
        echo "$POD_LIMITS" | sed 's/^/    /'

        # 检查是否接近 limit
        CPU_USAGE=$(echo "$POD_TOP" | awk '{print $2}')
        MEM_USAGE=$(echo "$POD_TOP" | awk '{print $3}')
        CPU_LIMIT=$(echo "$POD_LIMITS" | jq -r '.cpu // "unknown"')
        MEM_LIMIT=$(echo "$POD_LIMITS" | jq -r '.memory // "unknown"')

        if [[ "$CPU_LIMIT" != "unknown" && "$CPU_LIMIT" != "null" ]]; then
            # 简单比较（不含单位转换）
            print_info "CPU: $CPU_USAGE / $CPU_LIMIT"
        fi
        if [[ "$MEM_LIMIT" != "unknown" && "$MEM_LIMIT" != "null" ]]; then
            print_info "Memory: $MEM_USAGE / $MEM_LIMIT"
        fi
    else
        print_warn "Cannot get metrics for pod $POD_NAME"
    fi
elif [[ -n "$NAMESPACE" ]]; then
    NS_PODS=$(kubectl top pods -n "$NAMESPACE" --no-headers 2>/dev/null | sort -k2 -nr | head -10 || true)
    if [[ -n "$NS_PODS" ]]; then
        echo -e "  ${BOLD}NAME                    CPU(cores)   MEMORY(bytes)${NC}"
        echo "  ────────────────────────────────────────────────────────────"
        echo "$NS_PODS" | while IFS= read -r line; do
            echo "  $line"
        done
    else
        print_warn "No pod metrics available"
    fi
fi

# =============================================================================
# D1.3: OOM 和重启检查
# =============================================================================
print_section "D1.3: OOM & Restarts / OOM 和重启"

if [[ -n "$NAMESPACE" ]]; then
    PODS_JSON=$(kubectl get pods -n "$NAMESPACE" -o json 2>/dev/null || echo '{"items":[]}')
    OOM_PODS=$(echo "$PODS_JSON" | jq -r '.items[] | select(.status.containerStatuses[]?.lastState.terminated?.reason == "OOMKilled") | "\(.metadata.name):OOMKilled"')
    RESTART_PODS=$(echo "$PODS_JSON" | jq -r '.items[] | select(.status.containerStatuses[]?.restartCount > 5) | "\(.metadata.name):\(.status.containerStatuses[0].restartCount)"')

    if [[ -n "$OOM_PODS" ]]; then
        echo -e "  ${BOLD}OOMKilled Pods:${NC}"
        echo "$OOM_PODS" | while IFS=: read -r name reason; do
            echo -e "    ${RED}$name ($reason)${NC}"
            add_finding "D1.3: Pod $name OOMKilled - RC-002"
        done
    else
        print_ok "No OOMKilled pods found"
    fi

    if [[ -n "$RESTART_PODS" ]]; then
        echo -e "  ${BOLD}High Restart Pods:${NC}"
        echo "$RESTART_PODS" | while IFS=: read -r name count; do
            echo -e "    ${YELLOW}$name (restarts: $count)${NC}"
            add_finding "D1.3: Pod $name has $count restarts"
        done
    fi
fi

# =============================================================================
# D1.4: CPU Throttling 检查
# =============================================================================
print_section "D1.4: CPU Throttling / CPU 节流"

if [[ -n "$NAMESPACE" && -n "$POD_NAME" ]]; then
    # 尝试通过 metrics API 获取节流信息（需要 prometheus）
    print_info "CPU throttling check requires Prometheus metrics"
    print_info "Query: rate(container_cpu_cfs_throttled_seconds_total{pod=\"$POD_NAME\"}[5m])"
fi

# =============================================================================
# D1.5: 节点条件检查
# =============================================================================
print_section "D1.5: Node Conditions / 节点状态"

NODES_JSON=$(kubectl get nodes -o json 2>/dev/null || echo '{"items":[]}')
CONDITIONS=$(echo "$NODES_JSON" | jq -r '.items[] | .metadata.name as $name | .status.conditions[] | select(.status == "True" and (.type | test("Pressure"; "i"))) | "\($name):\(.type)"')

if [[ -n "$CONDITIONS" ]]; then
    echo -e "  ${BOLD}Pressure Conditions:${NC}"
    echo "$CONDITIONS" | while IFS=: read -r node cond; do
        echo -e "    ${RED}$node: $cond${NC}"
        add_finding "D1.5: Node $node has $cond - RC-003"
    done
else
    print_ok "No pressure conditions on nodes"
fi

# =============================================================================
# D1.6: 事件检查
# =============================================================================
print_section "D1.6: Events / 事件"

if [[ -n "$NAMESPACE" ]]; then
    EVENTS=$(kubectl get events -n "$NAMESPACE" --no-headers 2>/dev/null | grep -iE "throttle|oom|evict|fail.*schedul" | tail -10 || true)
    if [[ -n "$EVENTS" ]]; then
        echo "$EVENTS" | while IFS= read -r line; do
            echo -e "  ${YELLOW}$line${NC}"
        done
    else
        print_info "No relevant performance events"
    fi
fi

# =============================================================================
# 诊断总结
# =============================================================================
print_header "Diagnosis Summary / 诊断总结"

if [[ -n "$NAMESPACE" ]]; then
    echo -e "  Namespace: ${BOLD}${NAMESPACE}${NC}"
fi
if [[ -n "$POD_NAME" ]]; then
    echo -e "  Pod:       ${BOLD}${POD_NAME}${NC}"
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
echo -e "    ${GREEN}参考: reference/remediation-playbook.md${NC}"

echo ""
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════════════${NC}"
echo -e "  Phase 1 Quick Diagnosis Complete - $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════════════${NC}"
