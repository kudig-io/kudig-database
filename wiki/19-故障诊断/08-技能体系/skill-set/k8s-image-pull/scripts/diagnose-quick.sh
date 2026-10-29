#!/usr/bin/env bash
# =============================================================================
# K8s Image Pull Failure - Phase 1 Quick Diagnosis (Read-only)
#
# Usage: bash diagnose-quick.sh <namespace> <pod-name>
# Risk: NONE (read-only kubectl operations)
# Source: SKILL-IMG-001 D1.1-D1.6
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
    echo -e "${RED}Error: Pod '$POD_NAME' not found in namespace '$NAMESPACE'.${NC}"
    exit 1
fi

print_header "K8s Image Pull Failure - Phase 1 Quick Diagnosis"
echo -e "  Namespace:  ${BOLD}${NAMESPACE}${NC}"
echo -e "  Pod:        ${BOLD}${POD_NAME}${NC}"
echo -e "  Timestamp:  $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "  Risk Level: ${GREEN}NONE (read-only)${NC}"

# =============================================================================
# D1.1: Pod 状态和容器状态
# =============================================================================
print_section "D1.1: Pod & Container Status / Pod 和容器状态"

POD_JSON=$(kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o json 2>&1)
POD_PHASE=$(echo "$POD_JSON" | jq -r '.status.phase')
POD_STATUS_REASON=$(echo "$POD_JSON" | jq -r '.status.reason // "N/A"')

printf "  %-20s %s\n" "Phase:" "$POD_PHASE"
printf "  %-20s %s\n" "Reason:" "$POD_STATUS_REASON"

# 容器状态
CONTAINER_STATUSES=$(echo "$POD_JSON" | jq -r '.status.containerStatuses // [] | .[] | "\(.name):\(.state | keys[0]):\(.state[.state | keys[0]].reason // \"N/A\"):\(.state[.state | keys[0]].message // \"N/A\")"')

if [[ -n "$CONTAINER_STATUSES" ]]; then
    echo -e "  ${BOLD}Container Statuses:${NC}"
    echo "$CONTAINER_STATUSES" | while IFS=: read -r cname cstate creason cmessage; do
        case "$cstate" in
            waiting)
                if echo "$creason" | grep -qiE "ImagePullBackOff|ErrImagePull"; then
                    echo -e "    ${RED}$cname: $cstate ($creason)${NC}"
                    add_finding "D1.1: $cname - $creason"
                else
                    echo -e "    ${YELLOW}$cname: $cstate ($creason)${NC}"
                fi
                ;;
            running)
                echo -e "    ${GREEN}$cname: $cstate${NC}"
                ;;
            terminated)
                echo -e "    ${YELLOW}$cname: $cstate${NC}"
                ;;
            *)
                echo "    $cname: $cstate"
                ;;
        esac
    done
fi

# Init 容器状态
INIT_STATUSES=$(echo "$POD_JSON" | jq -r '.status.initContainerStatuses // [] | .[] | "\(.name):\(.state | keys[0]):\(.state[.state | keys[0]].reason // \"N/A\")"')
if [[ -n "$INIT_STATUSES" ]]; then
    echo ""
    echo -e "  ${BOLD}Init Container Statuses:${NC}"
    echo "$INIT_STATUSES" | while IFS=: read -r cname cstate creason; do
        if echo "$creason" | grep -qiE "ImagePullBackOff|ErrImagePull"; then
            echo -e "    ${RED}$cname: $cstate ($creason)${NC}"
            add_finding "D1.1: Init $cname - $creason"
        else
            echo "    $cname: $cstate ($creason)"
        fi
    done
fi

# =============================================================================
# D1.2: 镜像信息提取
# =============================================================================
print_section "D1.2: Image Info / 镜像信息"

IMAGES=$(echo "$POD_JSON" | jq -r '.spec.containers[].image')
echo -e "  ${BOLD}Container Images:${NC}"
echo "$IMAGES" | sed 's/^/    /'

INIT_IMAGES=$(echo "$POD_JSON" | jq -r '.spec.initContainers[]?.image // empty')
if [[ -n "$INIT_IMAGES" ]]; then
    echo -e "  ${BOLD}Init Container Images:${NC}"
    echo "$INIT_IMAGES" | sed 's/^/    /'
fi

# 检查镜像名称格式
for img in $IMAGES $INIT_IMAGES; do
    if echo "$img" | grep -q ":latest$"; then
        print_warn "Image uses 'latest' tag: $img"
        add_finding "D1.2: Image uses 'latest' tag - may cause non-deterministic pulls"
    fi
    if ! echo "$img" | grep -q ":"; then
        print_warn "Image has no explicit tag (defaults to latest): $img"
        add_finding "D1.2: Image has no explicit tag"
    fi
done

# =============================================================================
# D1.3: Events 分析
# =============================================================================
print_section "D1.3: Events / 事件分析"

EVENTS=$(kubectl get events -n "$NAMESPACE" --field-selector "involvedObject.name=${POD_NAME},involvedObject.kind=Pod" --sort-by=.lastTimestamp --no-headers 2>&1 | tail -20 || true)

if [[ -z "$EVENTS" || "$EVENTS" == *"No resources found"* ]]; then
    print_info "No recent events for this pod"
else
    echo "$EVENTS" | while IFS= read -r line; do
        if echo "$line" | grep -qiE "pull.*denied|unauthorized|not found|manifest unknown|forbidden|rate limit|too many requests"; then
            echo -e "  ${RED}$line${NC}"
        else
            echo "  $line"
        fi
    done

    if echo "$EVENTS" | grep -qi "pull.*denied\|unauthorized"; then
        add_finding "D1.3: Pull access denied/unauthorized - RC-002 (registry auth)"
    fi
    if echo "$EVENTS" | grep -qi "not found\|manifest unknown"; then
        add_finding "D1.3: Image/tag not found - RC-001 (wrong image/tag)"
    fi
    if echo "$EVENTS" | grep -qi "rate limit\|too many requests\|429"; then
        add_finding "D1.3: Rate limit hit - RC-006"
    fi
    if echo "$EVENTS" | grep -qi "timeout\|i/o timeout\|connection refused"; then
        add_finding "D1.3: Network timeout - RC-003 (registry unreachable)"
    fi
fi

# =============================================================================
# D1.4: imagePullSecrets 检查
# =============================================================================
print_section "D1.4: imagePullSecrets / 镜像拉取密钥"

POD_SA=$(echo "$POD_JSON" | jq -r '.spec.serviceAccountName // "default"')
POD_SECRETS=$(echo "$POD_JSON" | jq -r '.spec.imagePullSecrets[]?.name // empty')
SA_SECRETS=$(kubectl get sa "$POD_SA" -n "$NAMESPACE" -o json 2>/dev/null | jq -r '.imagePullSecrets[]?.name // empty')

printf "  %-20s %s\n" "ServiceAccount:" "$POD_SA"

if [[ -n "$POD_SECRETS" ]]; then
    echo -e "  ${BOLD}Pod imagePullSecrets:${NC}"
    echo "$POD_SECRETS" | sed 's/^/    /'
else
    print_info "Pod has no explicit imagePullSecrets"
fi

if [[ -n "$SA_SECRETS" ]]; then
    echo -e "  ${BOLD}SA imagePullSecrets:${NC}"
    echo "$SA_SECRETS" | sed 's/^/    /'
else
    print_warn "ServiceAccount '$POD_SA' has no imagePullSecrets"
    add_finding "D1.4: No imagePullSecrets configured - may be RC-002 if private registry"
fi

# 检查是否使用私有仓库
for img in $IMAGES $INIT_IMAGES; do
    if echo "$img" | grep -qvE "^(docker\.io|gcr\.io|registry\.k8s\.io|k8s\.gcr\.io|public\.ecr\.aws|quay\.io|ghcr\.io)"; then
        if echo "$img" | grep -qE "\.azurecr\.io|\.amazonaws\.com|\.gcr\.io|\.artifactory\.|\.harbor\.|\.gitlab\.|:[0-9]+"; then
            print_info "Private registry detected: $img"
            if [[ -z "$POD_SECRETS" && -z "$SA_SECRETS" ]]; then
                add_finding "D1.4: Private registry without imagePullSecrets - RC-002"
            fi
        fi
    fi
done

# =============================================================================
# D1.5: 节点状态检查
# =============================================================================
print_section "D1.5: Node Status / 节点状态"

POD_NODE=$(echo "$POD_JSON" | jq -r '.spec.nodeName // "N/A"')
printf "  %-20s %s\n" "Node:" "$POD_NODE"

if [[ "$POD_NODE" != "N/A" ]]; then
    NODE_STATUS=$(kubectl get node "$POD_NODE" --no-headers 2>/dev/null | awk '{print $2}' || echo "Unknown")
    printf "  %-20s %s\n" "Node Status:" "$NODE_STATUS"

    if echo "$NODE_STATUS" | grep -q "NotReady"; then
        add_finding "D1.5: Node is NotReady - may affect image pull → SKILL-NODE-001"
    fi

    # 检查节点磁盘压力
    NODE_CONDITIONS=$(kubectl get node "$POD_NODE" -o jsonpath='{range .status.conditions[*]}{.type}={.status}{"\n"}{end}' 2>/dev/null || true)
    if echo "$NODE_CONDITIONS" | grep -q "DiskPressure=True"; then
        add_finding "D1.5: Node DiskPressure=True - RC-004"
    fi
fi

# =============================================================================
# D1.6: 镜像平台检查
# =============================================================================
print_section "D1.6: Image Platform / 镜像平台兼容性"

NODE_ARCH=$(kubectl get node "$POD_NODE" -o jsonpath='{.status.nodeInfo.architecture}' 2>/dev/null || echo "unknown")
printf "  %-20s %s\n" "Node Architecture:" "$NODE_ARCH"

if [[ "$NODE_ARCH" == "arm64" ]]; then
    print_warn "Node is arm64 - ensure image supports arm64 platform"
    add_finding "D1.6: Node is arm64 - may be RC-005 if image lacks arm64 support"
fi

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
