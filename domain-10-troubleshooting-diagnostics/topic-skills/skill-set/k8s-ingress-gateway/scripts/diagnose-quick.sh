#!/usr/bin/env bash
# =============================================================================
# K8s Ingress Gateway Failure - Phase 1 Quick Diagnosis (Read-only)
#
# Usage: bash diagnose-quick.sh <namespace> <ingress-name>
# Risk: NONE (read-only kubectl operations)
# Source: SKILL-ING-001 D1.1-D1.6
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
    echo "Usage: bash diagnose-quick.sh <namespace> <ingress-name>"
    exit 1
fi

NAMESPACE="$1"
INGRESS_NAME="$2"

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

print_header "K8s Ingress Gateway Failure - Phase 1 Quick Diagnosis"
echo -e "  Namespace:  ${BOLD}${NAMESPACE}${NC}"
echo -e "  Ingress:    ${BOLD}${INGRESS_NAME}${NC}"
echo -e "  Timestamp:  $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "  Risk Level: ${GREEN}NONE (read-only)${NC}"

# =============================================================================
# D1.1: Ingress 状态
# =============================================================================
print_section "D1.1: Ingress Status / Ingress 状态"

INGRESS_JSON=$(kubectl get ingress "$INGRESS_NAME" -n "$NAMESPACE" -o json 2>/dev/null || true)

if [[ -z "$INGRESS_JSON" || "$INGRESS_JSON" == *"NotFound"* ]]; then
    # 尝试 Gateway
    GATEWAY_JSON=$(kubectl get gateway "$INGRESS_NAME" -n "$NAMESPACE" -o json 2>/dev/null || true)
    if [[ -n "$GATEWAY_JSON" && "$GATEWAY_JSON" != *"NotFound"* ]]; then
        print_info "Found Gateway resource (not Ingress)"
        GATEWAY_LISTENERS=$(echo "$GATEWAY_JSON" | jq -r '.spec.listeners[]? | "\(.name):\(.port):\(.protocol)"')
        GATEWAY_ADDRESSES=$(echo "$GATEWAY_JSON" | jq -r '.status.addresses[]? | "\(.type):\(.value)"')
        echo -e "  ${BOLD}Listeners:${NC}"
        echo "$GATEWAY_LISTENERS" | sed 's/^/    /'
        echo -e "  ${BOLD}Addresses:${NC}"
        echo "$GATEWAY_ADDRESSES" | sed 's/^/    /'

        if [[ -z "$GATEWAY_ADDRESSES" ]]; then
            add_finding "D1.1: Gateway has no assigned addresses"
        fi
    else
        print_error "Neither Ingress nor Gateway '$INGRESS_NAME' found in '$NAMESPACE'"
        exit 1
    fi
else
    INGRESS_CLASS=$(echo "$INGRESS_JSON" | jq -r '.spec.ingressClassName // .metadata.annotations["kubernetes.io/ingress.class"] // "default"')
    INGRESS_RULES=$(echo "$INGRESS_JSON" | jq -r '.spec.rules[]? | "\(.host) -> \(.http.paths[0].backend.service.name // .http.paths[0].backend.service.name)"')
    INGRESS_TLS=$(echo "$INGRESS_JSON" | jq -r '.spec.tls[]? | "hosts:\(.hosts | join(\",\")) secret:\(.secretName)"')
    INGRESS_ADDRS=$(echo "$INGRESS_JSON" | jq -r '.status.loadBalancer.ingress[]? | (.ip // .hostname) // empty')

    printf "  %-20s %s\n" "IngressClass:" "$INGRESS_CLASS"
    echo -e "  ${BOLD}Rules:${NC}"
    echo "$INGRESS_RULES" | sed 's/^/    /'
    if [[ -n "$INGRESS_TLS" ]]; then
        echo -e "  ${BOLD}TLS:${NC}"
        echo "$INGRESS_TLS" | sed 's/^/    /'
    fi
    echo -e "  ${BOLD}Addresses:${NC}"
    if [[ -n "$INGRESS_ADDRS" ]]; then
        echo "$INGRESS_ADDRS" | sed 's/^/    /'
    else
        print_warn "No load balancer addresses assigned"
        add_finding "D1.1: Ingress has no address - Ingress Controller may be down or misconfigured"
    fi
fi

# =============================================================================
# D1.2: Ingress Controller 状态
# =============================================================================
print_section "D1.2: Ingress Controller / 控制器状态"

for ns in ingress-nginx kube-system; do
    IC_PODS=$(kubectl get pods -n "$ns" --no-headers 2>/dev/null | grep -E "ingress|nginx" | head -5 || true)
    if [[ -n "$IC_PODS" ]]; then
        echo -e "  ${BOLD}Namespace: $ns${NC}"
        echo -e "  ${BOLD}NAME                          READY  STATUS${NC}"
        echo "$IC_PODS" | while IFS= read -r line; do
            if echo "$line" | grep -q "Running"; then
                echo "  $line"
            else
                echo -e "  ${RED}$line${NC}"
                add_finding "D1.2: Ingress Controller pod not Running in $ns - RC-002"
            fi
        done
    fi
done

# Istio Gateway
ISTIO_PODS=$(kubectl get pods -n istio-system --no-headers 2>/dev/null | grep gateway | head -3 || true)
if [[ -n "$ISTIO_PODS" ]]; then
    echo -e "  ${BOLD}Istio Gateway pods:${NC}"
    echo "$ISTIO_PODS" | sed 's/^/    /'
fi

# =============================================================================
# D1.3: 后端 Service 和 Endpoints
# =============================================================================
print_section "D1.3: Backend Services / 后端服务"

if [[ -n "$INGRESS_JSON" && "$INGRESS_JSON" != *"NotFound"* ]]; then
    BACKEND_SERVICES=$(echo "$INGRESS_JSON" | jq -r '.spec.rules[]?.http.paths[]?.backend.service.name // empty' | sort -u)

    if [[ -n "$BACKEND_SERVICES" ]]; then
        for svc in $BACKEND_SERVICES; do
            SVC_JSON=$(kubectl get service "$svc" -n "$NAMESPACE" -o json 2>/dev/null || true)
            if [[ -n "$SVC_JSON" && "$SVC_JSON" != *"NotFound"* ]]; then
                SVC_PORTS=$(echo "$SVC_JSON" | jq -r '.spec.ports[]? | "\(.port):\(.targetPort)"')
                ENDPOINTS=$(kubectl get endpoints "$svc" -n "$NAMESPACE" -o json 2>/dev/null | jq -r '.subsets[]?.addresses | length')
                ENDPOINTS=${ENDPOINTS:-0}

                printf "  ${BOLD}Service:${NC} %s\n" "$svc"
                printf "    Ports: %s\n" "$(echo "$SVC_PORTS" | tr '\n' ' ')"
                printf "    Endpoints: %s\n" "$ENDPOINTS"

                if [[ "$ENDPOINTS" -eq 0 ]]; then
                    print_error "Service '$svc' has 0 endpoints"
                    add_finding "D1.3: Backend service $svc has no endpoints - RC-003"
                else
                    print_ok "Service '$svc' has $ENDPOINTS endpoint(s)"
                fi
            else
                print_error "Backend service '$svc' not found"
                add_finding "D1.3: Backend service $svc not found - RC-003"
            fi
        done
    fi
fi

# =============================================================================
# D1.4: TLS Secret 检查
# =============================================================================
print_section "D1.4: TLS Secrets / 证书密钥"

if [[ -n "$INGRESS_JSON" && "$INGRESS_JSON" != *"NotFound"* ]]; then
    TLS_SECRETS=$(echo "$INGRESS_JSON" | jq -r '.spec.tls[]?.secretName // empty')

    if [[ -n "$TLS_SECRETS" ]]; then
        for secret in $TLS_SECRETS; do
            if kubectl get secret "$secret" -n "$NAMESPACE" &>/dev/null; then
                SECRET_TYPE=$(kubectl get secret "$secret" -n "$NAMESPACE" -o jsonpath='{.type}')
                if [[ "$SECRET_TYPE" == "kubernetes.io/tls" ]]; then
                    print_ok "TLS secret '$secret' exists"

                    # 检查证书有效期
                    CERT_DATA=$(kubectl get secret "$secret" -n "$NAMESPACE" -o jsonpath='{.data.tls\.crt}' | base64 -d)
                    if command -v openssl &>/dev/null && [[ -n "$CERT_DATA" ]]; then
                        END_DATE=$(echo "$CERT_DATA" | openssl x509 -noout -enddate 2>/dev/null | cut -d= -f2 || echo "")
                        if [[ -n "$END_DATE" ]]; then
                            END_EPOCH=$(date -j -f "%b %d %T %Y %Z" "$END_DATE" +%s 2>/dev/null || date -d "$END_DATE" +%s 2>/dev/null || echo "")
                            NOW_EPOCH=$(date +%s)
                            if [[ -n "$END_EPOCH" && -n "$NOW_EPOCH" ]]; then
                                DAYS_LEFT=$(( (END_EPOCH - NOW_EPOCH) / 86400 ))
                                if [[ "$DAYS_LEFT" -lt 0 ]]; then
                                    print_error "Certificate in '$secret' EXPIRED"
                                    add_finding "D1.4: TLS certificate expired - RC-004"
                                elif [[ "$DAYS_LEFT" -lt 7 ]]; then
                                    print_warn "Certificate in '$secret' expires in $DAYS_LEFT days"
                                    add_finding "D1.4: TLS certificate expires soon - RC-004"
                                fi
                            fi
                        fi
                    fi
                else
                    print_warn "Secret '$secret' is not kubernetes.io/tls (type=$SECRET_TYPE)"
                    add_finding "D1.4: Secret $secret wrong type - RC-004"
                fi
            else
                print_error "TLS secret '$secret' not found"
                add_finding "D1.4: TLS secret $secret missing - RC-004"
            fi
        done
    else
        print_info "Ingress has no TLS configuration"
    fi
fi

# =============================================================================
# D1.5: 简单 HTTP 测试（从集群内部）
# =============================================================================
print_section "D1.5: Internal HTTP Test / 内部 HTTP 测试"

# 获取 Ingress Controller Service 的 ClusterIP 或 NodePort
IC_SVC=$(kubectl get svc -n ingress-nginx --no-headers 2>/dev/null | grep controller | grep -v admission | head -1 || true)
if [[ -z "$IC_SVC" ]]; then
    IC_SVC=$(kubectl get svc -n kube-system --no-headers 2>/dev/null | grep ingress | head -1 || true)
fi

if [[ -n "$IC_SVC" ]]; then
    IC_SVC_NAME=$(echo "$IC_SVC" | awk '{print $1}')
    IC_NS=$(echo "$IC_SVC" | awk '{print $NF}')
    IC_CLUSTERIP=$(echo "$IC_SVC" | awk '{print $3}')
    print_info "Ingress Controller Service: $IC_SVC_NAME ($IC_CLUSTERIP)"
    add_finding "D1.5: Use kubectl run curl test for deeper diagnosis"
else
    print_info "Ingress Controller Service not found with standard naming"
fi

# =============================================================================
# D1.6: Events
# =============================================================================
print_section "D1.6: Events / 事件"

EVENTS=$(kubectl get events -n "$NAMESPACE" --field-selector "involvedObject.name=${INGRESS_NAME}" --no-headers 2>/dev/null | tail -10 || true)
if [[ -n "$EVENTS" && "$EVENTS" != *"No resources found"* ]]; then
    echo "$EVENTS" | while IFS= read -r line; do
        if echo "$line" | grep -qiE "error|fail|warning"; then
            echo -e "  ${YELLOW}$line${NC}"
        else
            echo "  $line"
        fi
    done
fi

# =============================================================================
# 诊断总结
# =============================================================================
print_header "Diagnosis Summary / 诊断总结"

echo -e "  Ingress: ${BOLD}${NAMESPACE}/${INGRESS_NAME}${NC}"
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
