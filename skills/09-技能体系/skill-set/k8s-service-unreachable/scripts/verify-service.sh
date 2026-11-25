#!/usr/bin/env bash
# =============================================================================
# K8s Service Unreachable - Post-Remediation Verification
# Service 无法访问修复后验证脚本
#
# Usage: bash verify-service.sh <service-name> <namespace>
# Risk: NONE (read-only, 仅执行查询类命令)
# Source: SKILL-SVC-001
# =============================================================================
set -euo pipefail

# --- 颜色定义 / Color Definitions ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# --- 统计变量 / Statistics Variables ---
PASS_COUNT=0
FAIL_COUNT=0
WARN_COUNT=0
TOTAL_CHECKS=8

# --- 工具函数 / Utility Functions ---
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

print_warn() {
    echo -e "  ${YELLOW}[WARN]${NC} $1"
    WARN_COUNT=$((WARN_COUNT + 1))
}

print_info() {
    echo -e "  ${BLUE}[INFO]${NC} $1"
}

# --- 参数验证 / Argument Validation ---
if [[ $# -lt 2 ]]; then
    echo -e "${RED}Error: Missing required arguments.${NC}"
    echo ""
    echo "Usage: bash verify-service.sh <service-name> <namespace>"
    echo ""
    echo "  <service-name>  Name of the Kubernetes Service to verify"
    echo "  <namespace>     Namespace where the Service resides"
    echo ""
    echo "Examples:"
    echo "  bash verify-service.sh backend-api production"
    echo "  bash verify-service.sh nginx-ingress ingress-nginx"
    echo ""
    echo "Verification checks:"
    echo "  V1: Service exists and type is valid"
    echo "  V2: Service selector matches backend Pods"
    echo "  V3: Endpoints/EndpointSlice are non-empty and consistent"
    echo "  V4: Backend Pods are Running and Ready"
    echo "  V5: Readiness probes are passing"
    echo "  V6: DNS resolution works inside cluster"
    echo "  V7: Layer-4 connectivity to ClusterIP succeeds"
    echo "  V8: (Apsara Stack/Alibaba Cloud) SLB backend health check passes"
    exit 1
fi

SERVICE_NAME="$1"
NAMESPACE="$2"

# --- 检查 kubectl 可用性 / Check kubectl availability ---
if ! command -v kubectl &>/dev/null; then
    echo -e "${RED}Error: kubectl is not installed or not in PATH.${NC}"
    exit 1
fi

print_header "Service Verification: ${SERVICE_NAME}.${NAMESPACE}"

# --- V1: Service exists and type is valid ---
print_section "V1: Service 存在且类型合法"
if kubectl get svc "${SERVICE_NAME}" -n "${NAMESPACE}" &>/dev/null; then
    SVC_TYPE=$(kubectl get svc "${SERVICE_NAME}" -n "${NAMESPACE}" -o jsonpath='{.spec.type}')
    CLUSTER_IP=$(kubectl get svc "${SERVICE_NAME}" -n "${NAMESPACE}" -o jsonpath='{.spec.clusterIP}')
    print_pass "Service ${SERVICE_NAME} 存在，类型: ${SVC_TYPE}，ClusterIP: ${CLUSTER_IP}"
    if [[ "${SVC_TYPE}" == "LoadBalancer" ]]; then
        LB_IP=$(kubectl get svc "${SERVICE_NAME}" -n "${NAMESPACE}" -o jsonpath='{.status.loadBalancer.ingress[0].ip}' 2>/dev/null || true)
        if [[ -n "${LB_IP}" ]]; then
            print_info "LoadBalancer IP: ${LB_IP}"
        else
            print_warn "LoadBalancer IP 尚未分配"
        fi
    fi
else
    print_fail "Service ${SERVICE_NAME} 在 namespace ${NAMESPACE} 中不存在"
    # 后续检查无意义
    echo ""
    echo -e "${RED}${BOLD}验证失败：Service 不存在${NC}"
    exit 1
fi

# --- V2: Service selector matches backend Pods ---
print_section "V2: Service selector 匹配后端 Pod"
SELECTOR_JSON=$(kubectl get svc "${SERVICE_NAME}" -n "${NAMESPACE}" -o jsonpath='{.spec.selector}')
if [[ -z "${SELECTOR_JSON}" || "${SELECTOR_JSON}" == "map[]" || "${SELECTOR_JSON}" == "<no value>" ]]; then
    if [[ "${SVC_TYPE}" == "ExternalName" ]]; then
        print_pass "ExternalName Service 无需 selector"
    else
        print_fail "Service selector 为空，且类型不是 ExternalName"
    fi
else
    # 构造 --selector 参数
    SELECTOR_STR=$(echo "${SELECTOR_JSON}" | tr -d '{} ' | tr ',' '\n' | sed 's/^ //' | paste -sd ',' -)
    MATCHING_PODS=$(kubectl get pods -n "${NAMESPACE}" --selector="${SELECTOR_STR}" --no-headers 2>/dev/null | wc -l | tr -d ' ')
    if [[ "${MATCHING_PODS}" -gt 0 ]]; then
        print_pass "Service selector 匹配到 ${MATCHING_PODS} 个 Pod"
    else
        print_fail "Service selector 未匹配到任何 Pod (selector: ${SELECTOR_STR})"
    fi
fi

# --- V3: Endpoints/EndpointSlice are non-empty and consistent ---
print_section "V3: Endpoints 与 EndpointSlice 非空且一致"
ENDPOINTS_COUNT=$(kubectl get endpoints "${SERVICE_NAME}" -n "${NAMESPACE}" -o jsonpath='{.subsets[*].addresses[*].ip}' 2>/dev/null | wc -w | tr -d ' ')
if [[ "${ENDPOINTS_COUNT}" -gt 0 ]]; then
    print_pass "Endpoints 包含 ${ENDPOINTS_COUNT} 个可用地址"
else
    print_fail "Endpoints 为空 (<none>)"
fi

EPS_COUNT=$(kubectl get endpointslices -n "${NAMESPACE}" --no-headers 2>/dev/null | grep "^${SERVICE_NAME}-" | wc -l | tr -d ' ')
if [[ "${EPS_COUNT}" -gt 0 ]]; then
    EPS_ADDRS=$(kubectl get endpointslices -n "${NAMESPACE}" -l "kubernetes.io/service-name=${SERVICE_NAME}" -o jsonpath='{.items[*].endpoints[*].addresses[*]}' 2>/dev/null | wc -w | tr -d ' ')
    if [[ "${EPS_ADDRS}" -gt 0 ]]; then
        print_pass "EndpointSlice 包含 ${EPS_ADDRS} 个可用地址"
    else
        print_warn "EndpointSlice 存在但地址为空"
    fi
else
    print_warn "未找到匹配的 EndpointSlice（旧版本集群可能未启用）"
fi

# --- V4: Backend Pods are Running and Ready ---
print_section "V4: 后端 Pod 运行且 Ready"
if [[ -n "${SELECTOR_STR:-}" ]]; then
    NOT_READY_PODS=$(kubectl get pods -n "${NAMESPACE}" --selector="${SELECTOR_STR}" --no-headers 2>/dev/null | awk '$2 !~ /^([0-9]+)\/\1$/ {print}' | wc -l | tr -d ' ')
    NOT_RUNNING_PODS=$(kubectl get pods -n "${NAMESPACE}" --selector="${SELECTOR_STR}" --no-headers 2>/dev/null | awk '$3 != "Running" && $3 != "Completed" {print}' | wc -l | tr -d ' ')
    if [[ "${NOT_RUNNING_PODS}" -eq 0 && "${NOT_READY_PODS}" -eq 0 ]]; then
        print_pass "所有后端 Pod 均 Running 且 Ready"
    else
        print_fail "存在 ${NOT_RUNNING_PODS} 个非 Running Pod，${NOT_READY_PODS} 个未 Ready Pod"
    fi
else
    print_warn "无法检查后端 Pod（selector 为空）"
fi

# --- V5: Readiness probes are passing ---
print_section "V5: Readiness 探针通过"
if [[ -n "${SELECTOR_STR:-}" ]]; then
    PROBE_FAIL_PODS=$(kubectl get pods -n "${NAMESPACE}" --selector="${SELECTOR_STR}" -o json 2>/dev/null | \
        python3 -c "import sys,json; d=json.load(sys.stdin); items=d.get('items',[]); fails=[p['metadata']['name'] for p in items if any(c.get('ready')==False and c.get('started')==True for c in p.get('status',{}).get('containerStatuses',[]))]; print(len(fails))" 2>/dev/null || echo "0")
    if [[ "${PROBE_FAIL_PODS}" -eq 0 ]]; then
        print_pass "所有容器 Readiness 探针通过"
    else
        print_fail "${PROBE_FAIL_PODS} 个容器 Readiness 探针未通过"
    fi
else
    print_warn "无法检查 Readiness 探针（selector 为空）"
fi

# --- V6: DNS resolution works inside cluster ---
print_section "V6: 集群内 DNS 解析正常"
DNS_TEST_IMAGE="busybox:1.36"
TEST_POD_NAME="dns-verify-${RANDOM}"
DNS_RESULT=$(kubectl run "${TEST_POD_NAME}" --rm -i --restart=Never --image="${DNS_TEST_IMAGE}" --timeout=30s -- \
    nslookup "${SERVICE_NAME}.${NAMESPACE}.svc.cluster.local" 2>/dev/null | grep -c "Address" || true)
if [[ "${DNS_RESULT}" -gt 0 ]]; then
    print_pass "DNS 解析成功 (${SERVICE_NAME}.${NAMESPACE}.svc.cluster.local)"
else
    print_fail "DNS 解析失败 (${SERVICE_NAME}.${NAMESPACE}.svc.cluster.local)"
fi

# --- V7: Layer-4 connectivity to ClusterIP succeeds ---
print_section "V7: ClusterIP 四层连通性"
if [[ -n "${CLUSTER_IP}" && "${CLUSTER_IP}" != "None" ]]; then
    # 获取第一个 targetPort
    TARGET_PORT=$(kubectl get svc "${SERVICE_NAME}" -n "${NAMESPACE}" -o jsonpath='{.spec.ports[0].targetPort}' 2>/dev/null)
    SVC_PORT=$(kubectl get svc "${SERVICE_NAME}" -n "${NAMESPACE}" -o jsonpath='{.spec.ports[0].port}' 2>/dev/null)
    if [[ -n "${TARGET_PORT}" ]]; then
        CONN_TEST_POD="conn-verify-${RANDOM}"
        CONN_RESULT=$(kubectl run "${CONN_TEST_POD}" --rm -i --restart=Never --image="${DNS_TEST_IMAGE}" --timeout=30s -- \
            timeout 5 sh -c "nc -vz ${CLUSTER_IP} ${SVC_PORT}" 2>&1 | grep -c "open" || true)
        if [[ "${CONN_RESULT}" -gt 0 ]]; then
            print_pass "ClusterIP ${CLUSTER_IP}:${SVC_PORT} 四层连通"
        else
            print_fail "ClusterIP ${CLUSTER_IP}:${SVC_PORT} 四层连通失败"
        fi
    else
        print_warn "Service 没有定义端口，跳过连通性检查"
    fi
else
    print_warn "Service 无 ClusterIP（如 Headless Service），跳过连通性检查"
fi

# --- V8: (Apsara Stack/Alibaba Cloud) SLB backend health check ---
print_section "V8: 阿里云/专有云 SLB 后端健康检查"
if [[ "${SVC_TYPE}" == "LoadBalancer" ]]; then
    # 尝试获取 SLB ID 注解
    SLB_ID=$(kubectl get svc "${SERVICE_NAME}" -n "${NAMESPACE}" -o jsonpath='{.metadata.annotations.service\.beta\.kubernetes\.io/alibaba-cloud-loadbalancer-id}' 2>/dev/null || true)
    if [[ -n "${SLB_ID}" ]]; then
        print_info "检测到 SLB ID: ${SLB_ID}"
        if command -v aliyun &>/dev/null; then
            SLB_HEALTH=$(aliyun slb DescribeLoadBalancerAttribute --LoadBalancerId "${SLB_ID}" --RegionId "${ALICLOUD_REGION_ID:-cn-hangzhou}" 2>/dev/null | grep -c '"BackendServers"' || true)
            if [[ "${SLB_HEALTH}" -gt 0 ]]; then
                print_pass "SLB 后端服务器组信息可查询"
            else
                print_warn "SLB 后端服务器组信息查询失败，请检查 aliyun CLI 权限"
            fi
        else
            print_warn "未安装 aliyun CLI，请登录 ASO/ACK 控制台检查 SLB 后端健康状态"
        fi
    else
        print_warn "Service 未关联阿里云 SLB ID，可能使用其他 LB 方案"
    fi
else
    print_info "Service 类型不是 LoadBalancer，跳过 SLB 检查"
fi

# --- 汇总 / Summary ---
print_header "验证汇总"
echo -e "  总检查项: ${BOLD}${TOTAL_CHECKS}${NC}"
echo -e "  ${GREEN}通过: ${PASS_COUNT}${NC}"
echo -e "  ${RED}失败: ${FAIL_COUNT}${NC}"
echo -e "  ${YELLOW}警告: ${WARN_COUNT}${NC}"
echo ""

if [[ ${FAIL_COUNT} -gt 0 ]]; then
    echo -e "${RED}${BOLD}Service ${SERVICE_NAME}.${NAMESPACE} 验证未通过，请继续排查。${NC}"
    exit 1
elif [[ ${WARN_COUNT} -gt 0 ]]; then
    echo -e "${YELLOW}${BOLD}Service ${SERVICE_NAME}.${NAMESPACE} 基本恢复，但存在警告项，建议持续关注。${NC}"
    exit 0
else
    echo -e "${GREEN}${BOLD}Service ${SERVICE_NAME}.${NAMESPACE} 验证全部通过。${NC}"
    exit 0
fi
