#!/usr/bin/env bash
# =============================================================================
# K8s Control Plane Failure - Phase 1 Quick Diagnosis (Read-only)
#
# Usage: bash diagnose-quick.sh [context]
# Risk: NONE (read-only kubectl operations)
# Source: SKILL-CTRL-001 D1.1-D1.6
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

KUBECTL="kubectl"
if [[ $# -ge 1 ]]; then
    KUBECTL="kubectl --context=$1"
fi

if ! command -v kubectl &>/dev/null; then
    echo -e "${RED}Error: kubectl is not installed.${NC}"
    exit 1
fi

print_header "K8s Control Plane Failure - Phase 1 Quick Diagnosis"
echo -e "  Timestamp:  $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "  Risk Level: ${GREEN}NONE (read-only)${NC}"

# =============================================================================
# D1.1: API Server 连通性
# =============================================================================
print_section "D1.1: API Server Connectivity / API Server 连通性"

if $KUBECTL cluster-info &>/dev/null; then
    CLUSTER_INFO=$($KUBECTL cluster-info 2>&1 | head -3)
    print_ok "API Server is reachable"
    echo "$CLUSTER_INFO" | sed 's/^/  /'
else
    print_error "API Server is NOT reachable"
    add_finding "D1.1: API Server unreachable - RC-004 or RC-001 (cert)"
fi

# =============================================================================
# D1.2: 控制平面节点状态
# =============================================================================
print_section "D1.2: Control Plane Nodes / 控制平面节点"

CP_NODES=$($KUBECTL get nodes -l node-role.kubernetes.io/control-plane --no-headers 2>/dev/null || \
           $KUBECTL get nodes -l node-role.kubernetes.io/master --no-headers 2>/dev/null || true)

if [[ -z "$CP_NODES" ]]; then
    print_warn "No control plane nodes found with standard labels"
    CP_NODES=$($KUBECTL get nodes --no-headers 2>/dev/null | head -3 || true)
    if [[ -n "$CP_NODES" ]]; then
        print_info "Showing first nodes (may include control plane):"
        echo "$CP_NODES" | sed 's/^/  /'
    fi
else
    echo -e "  ${BOLD}NAME                          STATUS   ROLES           AGE${NC}"
    echo "  ────────────────────────────────────────────────────────────"
    echo "$CP_NODES" | while IFS= read -r line; do
        if echo "$line" | grep -q "NotReady"; then
            echo -e "  ${RED}$line${NC}"
            NODE_NAME=$(echo "$line" | awk '{print $1}')
            add_finding "D1.2: Control plane node $NODE_NAME is NotReady - RC-007"
        else
            echo "  $line"
        fi
    done

    CP_COUNT=$(echo "$CP_NODES" | wc -l | tr -d ' ')
    CP_NOTREADY=$(echo "$CP_NODES" | grep -c "NotReady" || echo "0")
    print_info "Control plane nodes: $CP_COUNT total, $CP_NOTREADY NotReady"

    if [[ "$CP_NOTREADY" -gt 0 && "$CP_COUNT" -le 2 ]]; then
        add_finding "D1.2: $CP_NOTREADY/$CP_COUNT control plane nodes NotReady - HA at risk"
    fi
fi

# =============================================================================
# D1.3: kube-system Pod 状态
# =============================================================================
print_section "D1.3: kube-system Pods / 控制平面组件 Pod"

if $KUBECTL get ns kube-system &>/dev/null; then
    SYSTEM_PODS=$($KUBECTL get pods -n kube-system --no-headers 2>/dev/null || true)

    if [[ -n "$SYSTEM_PODS" ]]; then
        echo -e "  ${BOLD}NAME                                          READY  STATUS${NC}"
        echo "  ────────────────────────────────────────────────────────────"

        echo "$SYSTEM_PODS" | while IFS= read -r line; do
            POD_STATUS=$(echo "$line" | awk '{print $3}')
            POD_NAME=$(echo "$line" | awk '{print $1}')
            case "$POD_STATUS" in
                Running|Completed)
                    echo "  $line"
                    ;;
                CrashLoopBackOff|Error|Failed)
                    echo -e "  ${RED}$line${NC}"
                    if echo "$POD_NAME" | grep -qi "apiserver"; then
                        add_finding "D1.3: API Server Pod CrashLoopBackOff - RC-004"
                    elif echo "$POD_NAME" | grep -qi "etcd"; then
                        add_finding "D1.3: etcd Pod CrashLoopBackOff - RC-002"
                    elif echo "$POD_NAME" | grep -qi "scheduler"; then
                        add_finding "D1.3: Scheduler Pod CrashLoopBackOff - RC-005"
                    elif echo "$POD_NAME" | grep -qi "controller-manager"; then
                        add_finding "D1.3: Controller Manager Pod CrashLoopBackOff - RC-006"
                    fi
                    ;;
                Pending)
                    echo -e "  ${YELLOW}$line${NC}"
                    ;;
                *)
                    echo -e "  ${YELLOW}$line${NC}"
                    ;;
            esac
        done
    fi
else
    print_error "Cannot access kube-system namespace"
fi

# =============================================================================
# D1.4: etcd 健康检查
# =============================================================================
print_section "D1.4: etcd Health / etcd 健康"

ETCD_PODS=$($KUBECTL get pods -n kube-system --no-headers 2>/dev/null | grep etcd | awk '{print $1}' || true)

if [[ -n "$ETCD_PODS" ]]; then
    for etcd_pod in $ETCD_PODS; do
        HEALTH=$($KUBECTL exec "$etcd_pod" -n kube-system -- etcdctl endpoint health 2>/dev/null || echo "unhealthy")
        if echo "$HEALTH" | grep -qi "healthy"; then
            print_ok "etcd pod $etcd_pod is healthy"
        else
            print_error "etcd pod $etcd_pod is NOT healthy"
            add_finding "D1.4: etcd $etcd_pod unhealthy - RC-002 or RC-003"
        fi
    done
else
    print_warn "No etcd pods found in kube-system (may be external etcd)"
fi

# =============================================================================
# D1.5: 证书有效期检查
# =============================================================================
print_section "D1.5: Certificate Expiry / 证书有效期"

if command -v openssl &>/dev/null; then
    CERT_DIRS=("/etc/kubernetes/pki" "/var/lib/minikube/certs")
    CERT_CHECKED=false

    for cert_dir in "${CERT_DIRS[@]}"; do
        if [[ -d "$cert_dir" ]]; then
            CERT_CHECKED=true
            print_info "Checking certificates in $cert_dir"
            for cert in "$cert_dir"/*.crt; do
                [[ -f "$cert" ]] || continue
                END_DATE=$(openssl x509 -in "$cert" -noout -enddate 2>/dev/null | cut -d= -f2 || echo "")
                if [[ -n "$END_DATE" ]]; then
                    # 简单日期比较
                    END_EPOCH=$(date -j -f "%b %d %T %Y %Z" "$END_DATE" +%s 2>/dev/null || \
                                date -d "$END_DATE" +%s 2>/dev/null || echo "")
                    NOW_EPOCH=$(date +%s)
                    if [[ -n "$END_EPOCH" && -n "$NOW_EPOCH" ]]; then
                        DAYS_LEFT=$(( (END_EPOCH - NOW_EPOCH) / 86400 ))
                        CERT_BN=$(basename "$cert")
                        if [[ "$DAYS_LEFT" -lt 0 ]]; then
                            print_error "$CERT_BN EXPIRED (${DAYS_LEFT} days ago)"
                            add_finding "D1.5: Certificate $CERT_BN expired - RC-001"
                        elif [[ "$DAYS_LEFT" -lt 7 ]]; then
                            print_warn "$CERT_BN expires in $DAYS_LEFT days"
                            add_finding "D1.5: Certificate $CERT_BN expires soon - RC-001"
                        else
                            print_ok "$CERT_BN valid for $DAYS_LEFT days"
                        fi
                    fi
                fi
            done
        fi
    done

    if [[ "$CERT_CHECKED" == "false" ]]; then
        print_info "Cannot access control plane certificate directories from this host"
        print_info "Run on control plane node or use: kubectl exec <etcd-pod> -- openssl x509 ..."
    fi
else
    print_info "openssl not available for certificate checking"
fi

# =============================================================================
# D1.6: ComponentStatus 检查（Deprecated 但仍有用）
# =============================================================================
print_section "D1.6: Component Status / 组件状态"

COMPONENTS=$($KUBECTL get componentstatuses --no-headers 2>/dev/null || true)

if [[ -n "$COMPONENTS" ]]; then
    echo -e "  ${BOLD}NAME                 STATUS    MESSAGE${NC}"
    echo "  ────────────────────────────────────────────────────────────"
    echo "$COMPONENTS" | while IFS= read -r line; do
        if echo "$line" | grep -q "Unhealthy"; then
            echo -e "  ${RED}$line${NC}"
            COMP_NAME=$(echo "$line" | awk '{print $1}')
            add_finding "D1.6: Component $COMP_NAME is Unhealthy"
        else
            echo "  $line"
        fi
    done
else
    print_info "ComponentStatus API not available or deprecated in this cluster"
fi

# =============================================================================
# 诊断总结
# =============================================================================
print_header "Diagnosis Summary / 诊断总结"

echo -e "  Timestamp: $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
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
if [[ ${#ERRORS[@]} -gt 0 ]]; then
    echo -e "    ${YELLOW}1. SSH to control plane nodes for deep diagnosis${NC}"
    echo -e "    ${YELLOW}2. Check component logs: /var/log/pods or crictl logs${NC}"
fi
echo -e "    ${GREEN}参考: reference/remediation-playbook.md${NC}"

echo ""
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════════════${NC}"
echo -e "  Phase 1 Quick Diagnosis Complete - $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════════════${NC}"
