#!/usr/bin/env bash
# =============================================================================
# K8s Ingress Gateway Failure - Post-Remediation Verification
#
# Usage: bash verify-ingress.sh <namespace> <ingress-name>
# Risk: NONE (read-only)
# Source: SKILL-ING-001 Section 7
# =============================================================================
set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

PASS_COUNT=0
FAIL_COUNT=0
TOTAL_CHECKS=5

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

print_pass() {
    echo -e "  ${GREEN}[PASS]${NC} $1"
    PASS_COUNT=$((PASS_COUNT + 1))
}

print_fail() {
    echo -e "  ${RED}[FAIL]${NC} $1"
    FAIL_COUNT=$((FAIL_COUNT + 1))
}

print_info() {
    echo -e "  ${BLUE}[INFO]${NC} $1"
}

if [[ $# -lt 2 ]]; then
    echo -e "${RED}Error: Missing required arguments.${NC}"
    echo ""
    echo "Usage: bash verify-ingress.sh <namespace> <ingress-name>"
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

print_header "K8s Ingress Gateway - Post-Remediation Verification"
echo -e "  Namespace:  ${BOLD}${NAMESPACE}${NC}"
echo -e "  Ingress:    ${BOLD}${INGRESS_NAME}${NC}"
echo -e "  Timestamp:  $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "  Checks:     V1-V5 (${TOTAL_CHECKS} total)"

# =============================================================================
# V1: Ingress 存在且有地址
# =============================================================================
print_section "V1: Ingress Address"

INGRESS_JSON=$(kubectl get ingress "$INGRESS_NAME" -n "$NAMESPACE" -o json 2>/dev/null || true)
if [[ -z "$INGRESS_JSON" || "$INGRESS_JSON" == *"NotFound"* ]]; then
    GATEWAY_JSON=$(kubectl get gateway "$INGRESS_NAME" -n "$NAMESPACE" -o json 2>/dev/null || true)
    if [[ -n "$GATEWAY_JSON" && "$GATEWAY_JSON" != *"NotFound"* ]]; then
        GW_ADDRS=$(echo "$GATEWAY_JSON" | jq -r '.status.addresses[]? | (.ip // .hostname) // empty')
        if [[ -n "$GW_ADDRS" ]]; then
            print_pass "V1: Gateway has assigned addresses"
        else
            print_fail "V1: Gateway has no assigned addresses"
        fi
    else
        print_fail "V1: Neither Ingress nor Gateway found"
    fi
else
    INGRESS_ADDRS=$(echo "$INGRESS_JSON" | jq -r '.status.loadBalancer.ingress[]? | (.ip // .hostname) // empty')
    if [[ -n "$INGRESS_ADDRS" ]]; then
        print_pass "V1: Ingress has assigned addresses"
    else
        print_fail "V1: Ingress has no assigned addresses"
    fi
fi

# =============================================================================
# V2: Ingress Controller Running
# =============================================================================
print_section "V2: Ingress Controller Pods"

IC_RUNNING=false
for ns in ingress-nginx kube-system; do
    IC_PODS=$(kubectl get pods -n "$ns" --no-headers 2>/dev/null | grep -E "ingress|nginx" | grep "Running" | head -1 || true)
    if [[ -n "$IC_PODS" ]]; then
        IC_RUNNING=true
        break
    fi
done

if [[ "$IC_RUNNING" == "true" ]]; then
    print_pass "V2: Ingress Controller is Running"
else
    print_fail "V2: No Running Ingress Controller found"
fi

# =============================================================================
# V3: 后端 Service 有 Endpoints
# =============================================================================
print_section "V3: Backend Endpoints"

if [[ -n "$INGRESS_JSON" && "$INGRESS_JSON" != *"NotFound"* ]]; then
    BACKEND_SERVICES=$(echo "$INGRESS_JSON" | jq -r '.spec.rules[]?.http.paths[]?.backend.service.name // empty' | sort -u)

    ALL_HAVE_ENDPOINTS=true
    if [[ -n "$BACKEND_SERVICES" ]]; then
        for svc in $BACKEND_SERVICES; do
            ENDPOINTS=$(kubectl get endpoints "$svc" -n "$NAMESPACE" -o json 2>/dev/null | jq -r '.subsets[]?.addresses | length')
            ENDPOINTS=${ENDPOINTS:-0}
            if [[ "$ENDPOINTS" -eq 0 ]]; then
                ALL_HAVE_ENDPOINTS=false
            fi
        done
    fi

    if [[ "$ALL_HAVE_ENDPOINTS" == "true" ]]; then
        pass_pass "V3: All backend services have endpoints"
    else
        print_fail "V3: Some backend services have no endpoints"
    fi
else
    print_info "V3: Cannot check backends (Ingress not found)"
    TOTAL_CHECKS=$((TOTAL_CHECKS - 1))
fi

# =============================================================================
# V4: TLS Secret 存在且有效
# =============================================================================
print_section "V4: TLS Certificates"

if [[ -n "$INGRESS_JSON" && "$INGRESS_JSON" != *"NotFound"* ]]; then
    TLS_SECRETS=$(echo "$INGRESS_JSON" | jq -r '.spec.tls[]?.secretName // empty')

    if [[ -n "$TLS_SECRETS" ]]; then
        TLS_OK=true
        for secret in $TLS_SECRETS; do
            if ! kubectl get secret "$secret" -n "$NAMESPACE" &>/dev/null; then
                TLS_OK=false
            fi
        done

        if [[ "$TLS_OK" == "true" ]]; then
            print_pass "V4: All TLS secrets exist"
        else
            print_fail "V4: Some TLS secrets missing"
        fi
    else
        print_pass "V4: No TLS configured (OK)"
    fi
else
    print_info "V4: Cannot check TLS (Ingress not found)"
    TOTAL_CHECKS=$((TOTAL_CHECKS - 1))
fi

# =============================================================================
# V5: 无失败事件
# =============================================================================
print_section "V5: Events"

EVENTS=$(kubectl get events -n "$NAMESPACE" --field-selector "involvedObject.name=${INGRESS_NAME}" --no-headers 2>/dev/null | grep -iE "error|fail" || true)

if [[ -z "$EVENTS" ]]; then
    print_pass "V5: No failure events for Ingress"
else
    print_fail "V5: Failure events found"
fi

# =============================================================================
# 验证总结
# =============================================================================
print_header "Verification Summary / 验证总结"

echo -e "  Ingress: ${BOLD}${NAMESPACE}/${INGRESS_NAME}${NC}"
echo -e "  Time:    $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo ""
echo -e "  ${BOLD}Results:${NC}"
echo -e "    ${GREEN}PASS: ${PASS_COUNT}${NC} / ${TOTAL_CHECKS}"
echo -e "    ${RED}FAIL: ${FAIL_COUNT}${NC} / ${TOTAL_CHECKS}"
echo ""

if [[ $FAIL_COUNT -eq 0 ]]; then
    echo -e "  ${GREEN}${BOLD}╔══════════════════════════════════════════╗${NC}"
    echo -e "  ${GREEN}${BOLD}║       ✅  ALL CHECKS PASSED             ║${NC}"
    echo -e "  ${GREEN}${BOLD}║  Ingress/Gateway is healthy.           ║${NC}"
    echo -e "  ${GREEN}${BOLD}╚══════════════════════════════════════════╝${NC}"
else
    echo -e "  ${RED}${BOLD}╔══════════════════════════════════════════╗${NC}"
    echo -e "  ${RED}${BOLD}║       ❌  SOME CHECKS FAILED            ║${NC}"
    echo -e "  ${RED}${BOLD}║  Ingress/Gateway NOT fully recovered.  ║${NC}"
    echo -e "  ${RED}${BOLD}╚══════════════════════════════════════════╝${NC}"
fi

echo ""
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════════════${NC}"
echo -e "  Verification Complete - $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
echo -e "${BLUE}${BOLD}════════════════════════════════════════════════════════════════${NC}"

if [[ $FAIL_COUNT -gt 0 ]]; then
    exit 1
fi
exit 0
