#!/usr/bin/env bash
# =============================================================================
# K8s Logging Pipeline Failure - Phase 1 Quick Diagnosis (Read-only)
#
# Usage: bash diagnose-quick.sh [logging-namespace]
#   Default namespace: logging
# Risk: NONE (read-only kubectl operations)
# Source: SKILL-LOG-001 D1.1-D1.5
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

LOG_NS="${1:-logging}"

if ! command -v kubectl &>/dev/null; then
    echo -e "${RED}Error: kubectl is not installed.${NC}"
    exit 1
fi

if ! kubectl cluster-info &>/dev/null; then
    echo -e "${RED}Error: Cannot connect to cluster.${NC}"
    exit 1
fi

print_header "K8s Logging Pipeline Failure - Phase 1 Quick Diagnosis"
echo -e "  Namespace:  ${BOLD}${LOG_NS}${NC}"
echo -e "  Timestamp:  $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "  Risk Level: ${GREEN}NONE (read-only)${NC}"

# =============================================================================
# D1.1: 日志代理 Pod 状态
# =============================================================================
print_section "D1.1: Log Agents / 日志代理状态"

AGENT_PATTERNS=("fluentd" "fluent-bit" "filebeat" "vector" "promtail")
AGENT_FOUND=false

for pattern in "${AGENT_PATTERNS[@]}"; do
    AGENT_PODS=$(kubectl get pods -n "$LOG_NS" --no-headers 2>/dev/null | grep "$pattern" | head -5 || true)
    if [[ -n "$AGENT_PODS" ]]; then
        AGENT_FOUND=true
        echo -e "  ${BOLD}Pattern: $pattern${NC}"
        echo -e "  ${BOLD}NAME                          READY  STATUS${NC}"
        echo "$AGENT_PODS" | while IFS= read -r line; do
            POD_STATUS=$(echo "$line" | awk '{print $3}')
            if [[ "$POD_STATUS" == "Running" ]]; then
                echo "  $line"
            else
                echo -e "  ${RED}$line${NC}"
                POD_NAME=$(echo "$line" | awk '{print $1}')
                add_finding "D1.1: Log agent $POD_NAME is $POD_STATUS - RC-001"
            fi
        done
        echo ""
    fi
done

if [[ "$AGENT_FOUND" == "false" ]]; then
    # 尝试在所有 namespace 查找
    for pattern in "${AGENT_PATTERNS[@]}"; do
        AGENT_PODS=$(kubectl get pods --all-namespaces --no-headers 2>/dev/null | grep "$pattern" | head -3 || true)
        if [[ -n "$AGENT_PODS" ]]; then
            AGENT_FOUND=true
            print_info "Found $pattern agents in other namespaces:"
            echo "$AGENT_PODS" | sed 's/^/    /'
        fi
    done
fi

if [[ "$AGENT_FOUND" == "false" ]]; then
    print_warn "No log agents found in cluster"
    add_finding "D1.1: No log agents detected - logging may not be configured"
fi

# =============================================================================
# D1.2: 日志后端状态
# =============================================================================
print_section "D1.2: Log Backend / 日志后端状态"

# Elasticsearch
ES_PODS=$(kubectl get pods -n "$LOG_NS" --no-headers 2>/dev/null | grep -E "elasticsearch|es-" | head -3 || true)
if [[ -n "$ES_PODS" ]]; then
    echo -e "  ${BOLD}Elasticsearch:${NC}"
    echo "$ES_PODS" | while IFS= read -r line; do
        POD_STATUS=$(echo "$line" | awk '{print $3}')
        if [[ "$POD_STATUS" != "Running" ]]; then
            echo -e "  ${RED}$line${NC}"
            add_finding "D1.2: Elasticsearch pod not Running - RC-002"
        else
            echo "  $line"
        fi
    done

    # 检查 ES 集群健康
    ES_SVC=$(kubectl get svc -n "$LOG_NS" --no-headers 2>/dev/null | grep elasticsearch | grep -v headless | head -1 | awk '{print $1}' || true)
    if [[ -n "$ES_SVC" ]]; then
        ES_HEALTH=$(kubectl run es-check --rm -i --restart=Never --image=curlimages/curl -n "$LOG_NS" -- \
          "http://${ES_SVC}:9200/_cluster/health" 2>/dev/null | jq -r '.status' || echo "unknown")
        printf "  ${BOLD}Cluster Health:${NC} %s\n" "$ES_HEALTH"
        if [[ "$ES_HEALTH" == "red" ]]; then
            print_error "Elasticsearch cluster health is RED"
            add_finding "D1.2: ES cluster health RED - RC-002"
        elif [[ "$ES_HEALTH" == "yellow" ]]; then
            print_warn "Elasticsearch cluster health is YELLOW"
            add_finding "D1.2: ES cluster health YELLOW - may become RED"
        elif [[ "$ES_HEALTH" == "green" ]]; then
            print_ok "Elasticsearch cluster health is GREEN"
        fi
    fi
fi

# Loki
LOKI_PODS=$(kubectl get pods -n "$LOG_NS" --no-headers 2>/dev/null | grep loki | head -3 || true)
if [[ -n "$LOKI_PODS" ]]; then
    echo -e "  ${BOLD}Loki:${NC}"
    echo "$LOKI_PODS" | while IFS= read -r line; do
        POD_STATUS=$(echo "$line" | awk '{print $3}')
        if [[ "$POD_STATUS" != "Running" ]]; then
            echo -e "  ${RED}$line${NC}"
            add_finding "D1.2: Loki pod not Running - RC-002"
        else
            echo "  $line"
        fi
    done
fi

if [[ -z "$ES_PODS" && -z "$LOKI_PODS" ]]; then
    print_info "No ES or Loki backend found in $LOG_NS"
fi

# =============================================================================
# D1.3: 日志代理日志中的错误
# =============================================================================
print_section "D1.3: Agent Logs / 代理日志"

for pattern in "${AGENT_PATTERNS[@]}"; do
    AGENT_POD=$(kubectl get pods -n "$LOG_NS" --no-headers 2>/dev/null | grep "$pattern" | grep "Running" | head -1 | awk '{print $1}' || true)
    if [[ -n "$AGENT_POD" ]]; then
        LOG_ERRORS=$(kubectl logs "$AGENT_POD" -n "$LOG_NS" --tail=30 2>/dev/null | grep -iE "error|fail|reject|drop|buffer.*full|retry.*exhausted" | tail -5 || true)
        if [[ -n "$LOG_ERRORS" ]]; then
            print_warn "$pattern has recent errors"
            echo "$LOG_ERRORS" | sed 's/^/    /'
            add_finding "D1.3: $pattern errors in logs - RC-001, RC-003, or RC-005"
        else
            print_ok "$pattern logs look clean (last 30 lines)"
        fi
        break
    fi
done

# =============================================================================
# D1.4: 节点日志文件检查
# =============================================================================
print_section "D1.4: Node Logs / 节点日志文件"

# 检查是否有 DaemonSet 在所有节点运行
for pattern in "fluentd" "fluent-bit" "filebeat" "promtail"; do
    DS=$(kubectl get ds -n "$LOG_NS" --no-headers 2>/dev/null | grep "$pattern" | head -1 || true)
    if [[ -n "$DS" ]]; then
        DS_NAME=$(echo "$DS" | awk '{print $1}')
        DS_DESIRED=$(echo "$DS" | awk '{print $2}')
        DS_CURRENT=$(echo "$DS" | awk '{print $3}')
        DS_READY=$(echo "$DS" | awk '{print $4}')

        printf "  ${BOLD}DaemonSet:${NC} %s (desired=%s, current=%s, ready=%s)\n" "$DS_NAME" "$DS_DESIRED" "$DS_CURRENT" "$DS_READY"

        if [[ "$DS_DESIRED" != "$DS_READY" ]]; then
            print_warn "DaemonSet $DS_NAME not fully ready ($DS_READY/$DS_DESIRED)"
            add_finding "D1.4: Log agent DaemonSet not fully scheduled - RC-004"
        else
            print_ok "DaemonSet $DS_NAME fully scheduled"
        fi
        break
    fi
done

# =============================================================================
# D1.5: 缓冲/队列状态
# =============================================================================
print_section "D1.5: Buffer Status / 缓冲状态"

AGENT_POD=$(kubectl get pods -n "$LOG_NS" --no-headers 2>/dev/null | grep -E "fluentd|fluent-bit" | grep "Running" | head -1 | awk '{print $1}' || true)

if [[ -n "$AGENT_POD" ]]; then
    # 尝试检查缓冲区使用率
    BUFFER_INFO=$(kubectl exec "$AGENT_POD" -n "$LOG_NS" -- sh -c "df -h /var/log/fluentd-buffers /tmp/fluent-bit 2>/dev/null || df -h" 2>/dev/null || true)
    if [[ -n "$BUFFER_INFO" ]]; then
        echo -e "  ${BOLD}Disk usage (buffer dirs):${NC}"
        echo "$BUFFER_INFO" | sed 's/^/    /'
    fi
else
    print_info "No running Fluentd/Fluent Bit pod to check buffers"
fi

# =============================================================================
# 诊断总结
# =============================================================================
print_header "Diagnosis Summary / 诊断总结"

echo -e "  Namespace: ${BOLD}${LOG_NS}${NC}"
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
