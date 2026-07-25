#!/usr/bin/env bash
# =============================================================================
# K8s Deployment Rollout Failure - Phase 1 Quick Diagnosis (Read-only)
# 快速诊断脚本 - 通过 kubectl 收集 Deployment 发布状态信息
#
# Usage: bash diagnose-quick.sh <namespace> <deployment-name>
# Risk: NONE (read-only kubectl operations)
# Source: SKILL-DEPLOY-001 D1.1-D1.6
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

# --- 参数验证 ---
if [[ $# -lt 2 ]]; then
    echo -e "${RED}Error: Missing required arguments.${NC}"
    echo ""
    echo "Usage: bash diagnose-quick.sh <namespace> <deployment-name>"
    echo ""
    echo "Examples:"
    echo "  bash diagnose-quick.sh default my-app"
    echo "  bash diagnose-quick.sh production api-gateway"
    exit 1
fi

NAMESPACE="$1"
DEPLOY_NAME="$2"

if ! command -v kubectl &>/dev/null; then
    echo -e "${RED}Error: kubectl is not installed or not in PATH.${NC}"
    exit 1
fi

if ! kubectl cluster-info &>/dev/null; then
    echo -e "${RED}Error: Cannot connect to Kubernetes cluster.${NC}"
    exit 1
fi

# 验证 namespace 和 deployment 存在
if ! kubectl get ns "$NAMESPACE" &>/dev/null; then
    echo -e "${RED}Error: Namespace '$NAMESPACE' not found.${NC}"
    exit 1
fi

if ! kubectl get deployment "$DEPLOY_NAME" -n "$NAMESPACE" &>/dev/null; then
    echo -e "${RED}Error: Deployment '$DEPLOY_NAME' not found in namespace '$NAMESPACE'.${NC}"
    echo ""
    echo "Available deployments in $NAMESPACE:"
    kubectl get deployment -n "$NAMESPACE" --no-headers 2>/dev/null | awk '{print "  " $1}' || true
    exit 1
fi

print_header "K8s Deployment Rollout Failure - Phase 1 Quick Diagnosis"
echo -e "  Namespace:   ${BOLD}${NAMESPACE}${NC}"
echo -e "  Deployment:  ${BOLD}${DEPLOY_NAME}${NC}"
echo -e "  Timestamp:   $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "  Risk Level:  ${GREEN}NONE (read-only)${NC}"

# =============================================================================
# D1.1: Deployment 全局状态概览
# =============================================================================
print_section "D1.1: Deployment Status Overview / 部署状态概览"

DEPLOY_JSON=$(kubectl get deployment "$DEPLOY_NAME" -n "$NAMESPACE" -o json 2>&1)

DESIRED=$(echo "$DEPLOY_JSON" | jq -r '.spec.replicas // 0')
READY=$(echo "$DEPLOY_JSON" | jq -r '.status.readyReplicas // 0')
UPDATED=$(echo "$DEPLOY_JSON" | jq -r '.status.updatedReplicas // 0')
AVAILABLE=$(echo "$DEPLOY_JSON" | jq -r '.status.availableReplicas // 0')
UNAVAILABLE=$(echo "$DEPLOY_JSON" | jq -r '.status.unavailableReplicas // 0')
OBSERVED_GEN=$(echo "$DEPLOY_JSON" | jq -r '.status.observedGeneration // 0')
SPEC_GEN=$(echo "$DEPLOY_JSON" | jq -r '.metadata.generation // 0')
PAUSED=$(echo "$DEPLOY_JSON" | jq -r '.spec.paused // false')
STRATEGY=$(echo "$DEPLOY_JSON" | jq -r '.spec.strategy.type // "RollingUpdate"')

printf "  %-20s %s\n" "Desired:" "$DESIRED"
printf "  %-20s %s\n" "Ready:" "$READY"
printf "  %-20s %s\n" "Updated:" "$UPDATED"
printf "  %-20s %s\n" "Available:" "$AVAILABLE"
printf "  %-20s %s\n" "Unavailable:" "$UNAVAILABLE"
printf "  %-20s %s\n" "Strategy:" "$STRATEGY"
printf "  %-20s %s\n" "Paused:" "$PAUSED"
printf "  %-20s %s / %s\n" "Generation:" "$OBSERVED_GEN (observed)" "$SPEC_GEN (spec)"

if [[ "$PAUSED" == "true" ]]; then
    print_error "Deployment is paused"
    add_finding "D1.1: Deployment paused - RC-005"
fi

if [[ "$OBSERVED_GEN" -ne "$SPEC_GEN" ]]; then
    print_warn "Observed generation ($OBSERVED_GEN) != spec generation ($SPEC_GEN)"
    add_finding "D1.1: Generation mismatch - rollout may be in progress or stuck"
fi

if [[ "$UNAVAILABLE" -gt 0 ]]; then
    print_warn "$UNAVAILABLE replicas unavailable"
    add_finding "D1.1: $UNAVAILABLE replicas unavailable"
fi

if [[ "$READY" -lt "$DESIRED" ]]; then
    print_error "Ready ($READY) < Desired ($DESIRED)"
    add_finding "D1.1: Ready < Desired - rollout may be stuck"
else
    print_ok "Ready ($READY) >= Desired ($DESIRED)"
fi

# =============================================================================
# D1.2: ReplicaSet 状态检查
# =============================================================================
print_section "D1.2: ReplicaSet Status / ReplicaSet 状态"

RS_LIST=$(kubectl get rs -n "$NAMESPACE" -l "app=$(echo "$DEPLOY_JSON" | jq -r '.spec.selector.matchLabels.app // empty')" --no-headers 2>/dev/null || true)

if [[ -z "$RS_LIST" ]]; then
    RS_LIST=$(kubectl get rs -n "$NAMESPACE" --no-headers 2>/dev/null | grep "^${DEPLOY_NAME}-" || true)
fi

if [[ -z "$RS_LIST" ]]; then
    print_error "No ReplicaSet found for deployment"
    add_finding "D1.2: No ReplicaSet found"
else
    echo -e "  ${BOLD}NAME                          DESIRED  CURRENT  READY  AGE${NC}"
    echo "  ────────────────────────────────────────────────────────────"
    echo "$RS_LIST" | while IFS= read -r line; do
        echo "  $line"
    done

    # 检查是否有多个 RS
    RS_COUNT=$(echo "$RS_LIST" | wc -l | tr -d ' ')
    if [[ "$RS_COUNT" -gt 1 ]]; then
        print_warn "Multiple ReplicaSets found ($RS_COUNT)"
        add_finding "D1.2: Multiple ReplicaSets - may indicate stuck rollout"
    fi

    # 检查是否有旧 RS 仍有副本
    OLD_RS_RUNNING=$(echo "$RS_LIST" | awk '$3 > 0 {print $1}')
    if [[ -n "$OLD_RS_RUNNING" && "$RS_COUNT" -gt 1 ]]; then
        print_warn "Old ReplicaSet still has running pods"
        add_finding "D1.2: Old ReplicaSet still has pods - rollout strategy may be blocking"
    fi
fi

# =============================================================================
# D1.3: Pod 状态检查
# =============================================================================
print_section "D1.3: Pod Status / Pod 状态"

PODS_OUTPUT=$(kubectl get pods -n "$NAMESPACE" -l "app=$(echo "$DEPLOY_JSON" | jq -r '.spec.selector.matchLabels.app // empty')" --no-headers 2>/dev/null || true)

if [[ -z "$PODS_OUTPUT" ]]; then
    PODS_OUTPUT=$(kubectl get pods -n "$NAMESPACE" --no-headers 2>/dev/null | grep "^${DEPLOY_NAME}-" || true)
fi

if [[ -z "$PODS_OUTPUT" ]]; then
    print_error "No pods found for deployment"
    add_finding "D1.3: No pods found"
else
    echo -e "  ${BOLD}NAME                          READY  STATUS        RESTARTS  AGE${NC}"
    echo "  ────────────────────────────────────────────────────────────"
    echo "$PODS_OUTPUT" | while IFS= read -r line; do
        STATUS=$(echo "$line" | awk '{print $3}')
        case "$STATUS" in
            Running)
                echo -e "  ${GREEN}$line${NC}"
                ;;
            Pending|ContainerCreating|Init:*)  
                echo -e "  ${YELLOW}$line${NC}"
                ;;
            ImagePullBackOff|ErrImagePull|CrashLoopBackOff|Error|Failed)
                echo -e "  ${RED}$line${NC}"
                ;;
            *)
                echo "  $line"
                ;;
        esac
    done

    # 统计
    PENDING_COUNT=$(echo "$PODS_OUTPUT" | awk '$3 == "Pending" {count++} END {print count+0}')
    IMAGEPULL_COUNT=$(echo "$PODS_OUTPUT" | grep -c "ImagePullBackOff\|ErrImagePull" || echo "0")
    CRASH_COUNT=$(echo "$PODS_OUTPUT" | grep -c "CrashLoopBackOff\|Error\|Failed" || echo "0")
    CREATING_COUNT=$(echo "$PODS_OUTPUT" | grep -c "ContainerCreating" || echo "0")
    INIT_COUNT=$(echo "$PODS_OUTPUT" | grep -c "Init:" || echo "0")

    if [[ "$PENDING_COUNT" -gt 0 ]]; then
        add_finding "D1.3: $PENDING_COUNT pods Pending - may be RC-001 (resource) or RC-006 (scheduling)"
    fi
    if [[ "$IMAGEPULL_COUNT" -gt 0 ]]; then
        add_finding "D1.3: $IMAGEPULL_COUNT pods ImagePullBackOff - RC-002"
    fi
    if [[ "$CRASH_COUNT" -gt 0 ]]; then
        add_finding "D1.3: $CRASH_COUNT pods crashing - RC-003 (health check) or RC-007 (init)"
    fi
    if [[ "$CREATING_COUNT" -gt 0 ]]; then
        add_finding "D1.3: $CREATING_COUNT pods ContainerCreating - may be RC-002 (image) or storage"
    fi
    if [[ "$INIT_COUNT" -gt 0 ]]; then
        add_finding "D1.3: $INIT_COUNT pods stuck in init - RC-007"
    fi
fi

# =============================================================================
# D1.4: Events 检查
# =============================================================================
print_section "D1.4: Events / 事件分析"

EVENTS_OUTPUT=$(kubectl get events -n "$NAMESPACE" --field-selector "involvedObject.name=${DEPLOY_NAME},involvedObject.kind=Deployment" --sort-by=.lastTimestamp --no-headers 2>&1 | tail -20)

if [[ -z "$EVENTS_OUTPUT" || "$EVENTS_OUTPUT" == *"No resources found"* ]]; then
    print_info "No recent deployment events"
else
    echo "$EVENTS_OUTPUT" | while IFS= read -r line; do
        if echo "$line" | grep -qi "warning\|error\|failed\|stuck"; then
            echo -e "  ${RED}$line${NC}"
        else
            echo "  $line"
        fi
    done

    if echo "$EVENTS_OUTPUT" | grep -qi "ProgressDeadlineExceeded"; then
        add_finding "D1.4: ProgressDeadlineExceeded event - rollout definitely stuck"
    fi
    if echo "$EVENTS_OUTPUT" | grep -qi "FailedCreate\|FailedScheduling"; then
        add_finding "D1.4: FailedCreate/FailedScheduling event - RC-001 or RC-006"
    fi
    if echo "$EVENTS_OUTPUT" | grep -qi "FailedMount"; then
        add_finding "D1.4: FailedMount event - storage or config issue"
    fi
fi

# =============================================================================
# D1.5: 滚动更新策略检查
# =============================================================================
print_section "D1.5: RollingUpdate Strategy / 滚动更新策略"

if [[ "$STRATEGY" == "RollingUpdate" ]]; then
    MAX_UNAVAILABLE=$(echo "$DEPLOY_JSON" | jq -r '.spec.strategy.rollingUpdate.maxUnavailable // "25%"')
    MAX_SURGE=$(echo "$DEPLOY_JSON" | jq -r '.spec.strategy.rollingUpdate.maxSurge // "25%"')
    printf "  %-20s %s\n" "maxUnavailable:" "$MAX_UNAVAILABLE"
    printf "  %-20s %s\n" "maxSurge:" "$MAX_SURGE"

    if [[ "$MAX_UNAVAILABLE" == "0" && "$DESIRED" -le 1 ]]; then
        print_warn "maxUnavailable=0 with only 1 replica - rollout cannot proceed"
        add_finding "D1.5: maxUnavailable=0 with 1 replica - RC-004"
    fi
else
    print_info "Strategy is $STRATEGY (not RollingUpdate)"
fi

# =============================================================================
# D1.6: 资源配额和限制检查
# =============================================================================
print_section "D1.6: Resource Quota / 资源配额检查"

QUOTA_OUTPUT=$(kubectl describe quota -n "$NAMESPACE" 2>&1 || true)
if echo "$QUOTA_OUTPUT" | grep -qi "used\|hard"; then
    echo "$QUOTA_OUTPUT" | grep -A 5 "Resource Quotas" || echo "$QUOTA_OUTPUT" | head -20
    if echo "$QUOTA_OUTPUT" | grep -qi "used.*hard"; then
        add_finding "D1.6: ResourceQuota configured - verify not exhausted"
    fi
else
    print_info "No ResourceQuota in namespace"
fi

# 检查 Pod 资源请求
POD_TEMPLATE=$(echo "$DEPLOY_JSON" | jq -r '.spec.template.spec.containers[0].resources.requests // empty')
if [[ -n "$POD_TEMPLATE" && "$POD_TEMPLATE" != "null" && "$POD_TEMPLATE" != "{}" ]]; then
    print_info "Container resource requests:"
    echo "$POD_TEMPLATE" | sed 's/^/  /'
else
    print_warn "No resource requests configured for containers"
    add_finding "D1.6: No resource requests - may cause scheduling issues"
fi

# =============================================================================
# 诊断总结
# =============================================================================
print_header "Diagnosis Summary / 诊断总结"

echo -e "  Deployment: ${BOLD}${NAMESPACE}/${DEPLOY_NAME}${NC}"
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

# 建议下一步
echo -e "  ${BOLD}Recommended Next Steps / 建议下一步:${NC}"
if echo "$DEPLOY_JSON" | grep -q '"paused":true'; then
    echo -e "    ${YELLOW}1. Resume deployment:${NC} kubectl rollout resume deployment/${DEPLOY_NAME} -n ${NAMESPACE}"
fi
if [[ -n "${PENDING_COUNT:-}" && "${PENDING_COUNT}" -gt 0 ]]; then
    echo -e "    ${YELLOW}2. Check pending pod events:${NC} kubectl describe pod <pod-name> -n ${NAMESPACE}"
fi
if [[ -n "${IMAGEPULL_COUNT:-}" && "${IMAGEPULL_COUNT}" -gt 0 ]]; then
    echo -e "    ${YELLOW}3. Fix image pull issue → SKILL-IMG-001${NC}"
fi
echo -e "    ${GREEN}参考: reference/remediation-playbook.md${NC}"

echo ""
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════════════${NC}"
echo -e "  Phase 1 Quick Diagnosis Complete - $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════════════${NC}"
