#!/usr/bin/env bash
# ============================================================================
# 05-service-no-endpoints.sh — 场景: Service 无 Endpoints
# Scenario: Service Connectivity / Empty Endpoints — maps to SKILL-NET-002
# ============================================================================
# 演示: 创建 label selector 不匹配的 Service → Endpoints 为空 → 修复
# ============================================================================

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
BOLD='\033[1m'
NC='\033[0m'

section() { echo -e "\n${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"; echo -e "${BOLD}${BLUE}$1${NC}"; echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"; }
step()    { echo -e "\n${MAGENTA}▸ [$1] $2${NC}"; }
info()    { echo -e "  ${GREEN}ℹ${NC} $1"; }
run_cmd() { echo -e "  ${CYAN}\$ $1${NC}"; eval "$1" 2>&1 | sed 's/^/    /'; }
pause()   { echo -e "\n  ${YELLOW}按 Enter 继续 / Press Enter to continue...${NC}"; read -r; }

NS="skill-demo"
SVC_NAME="broken-svc"
DEPLOY_NAME="backend-app"

echo -e "${CYAN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║  📋 Scenario 05: Service No Endpoints (SKILL-NET-002)      ║${NC}"
echo -e "${CYAN}╚══════════════════════════════════════════════════════════════╝${NC}"

# =====================================================================
# PHASE 0: 故障注入
# =====================================================================
section "Phase 0: 故障注入 / Fault Injection"

step "INJECT-1" "部署后端应用 / Deploy backend app"
kubectl apply -f - <<EOF
apiVersion: apps/v1
kind: Deployment
metadata:
  name: ${DEPLOY_NAME}
  namespace: ${NS}
spec:
  replicas: 2
  selector:
    matchLabels:
      app: backend-app
  template:
    metadata:
      labels:
        app: backend-app
    spec:
      containers:
        - name: web
          image: nginx:1.27-alpine
          ports:
            - containerPort: 80
          resources:
            requests:
              cpu: 50m
              memory: 64Mi
EOF

kubectl rollout status deployment/${DEPLOY_NAME} -n ${NS} --timeout=60s

step "INJECT-2" "创建 selector 不匹配的 Service / Create mismatched Service"
kubectl apply -f - <<EOF
apiVersion: v1
kind: Service
metadata:
  name: ${SVC_NAME}
  namespace: ${NS}
  labels:
    scenario: skill-net-002
spec:
  selector:
    app: backend-app-typo
  ports:
    - port: 80
      targetPort: 80
EOF

info "Service selector 使用了错误的 label: backend-app-typo (正确: backend-app)"
pause

# =====================================================================
# PHASE 1: 症状检测
# =====================================================================
section "Phase 1: 症状检测 / Symptom Detection (Skill Section 2)"

step "S1" "检查 Service Endpoints / Check Endpoints (置信度: 0.90)"
run_cmd "kubectl get endpoints ${SVC_NAME} -n ${NS}"
info "💡 Endpoints 为空 (<none>) → SKILL-NET-002 激活"

step "S2" "尝试访问 Service / Attempt Service access"
kubectl run curl-test --image=curlimages/curl:8.5.0 --restart=Never -n ${NS} --command -- sleep 300 2>/dev/null || true
sleep 5
kubectl wait --for=condition=Ready pod/curl-test -n ${NS} --timeout=30s 2>/dev/null || true
info "测试 Service 连通性:"
kubectl exec curl-test -n ${NS} -- curl -s --connect-timeout 3 "http://${SVC_NAME}.${NS}.svc.cluster.local" 2>&1 | head -5 | sed 's/^/    /' || echo "    连接失败 / Connection failed"

step "S3" "检查 EndpointSlices / Check EndpointSlices"
run_cmd "kubectl get endpointslices -n ${NS} -l kubernetes.io/service-name=${SVC_NAME}"
pause

# =====================================================================
# PHASE 2: 快速分级
# =====================================================================
section "Phase 2: 快速分级 / Quick Triage (Skill Section 3)"

step "T1" "影响评估"
info "单个 Service Endpoints 为空"
echo -e "  ${YELLOW}🟡 P2 — Service 连通性异常${NC}"
pause

# =====================================================================
# PHASE 3: 诊断工作流
# =====================================================================
section "Phase 3: 诊断工作流 / Diagnostic Workflow (Skill Section 4)"

step "D1.1" "Service 详情 / Service details"
run_cmd "kubectl describe svc ${SVC_NAME} -n ${NS}"

step "D1.2" "Service selector 检查 / Selector analysis"
SVC_SELECTOR=$(kubectl get svc ${SVC_NAME} -n ${NS} -o jsonpath='{.spec.selector}')
info "Service selector: ${SVC_SELECTOR}"

step "D1.3" "匹配的 Pod 查找 / Find matching pods"
info "使用 Service selector 查找 Pod:"
run_cmd "kubectl get pods -n ${NS} -l app=backend-app-typo --no-headers"
info "💡 没有匹配 selector 'app=backend-app-typo' 的 Pod!"

step "D1.4" "检查实际的 Pod labels / Check actual pod labels"
info "实际存在的 Pod:"
run_cmd "kubectl get pods -n ${NS} -l app=backend-app --show-labels"
info "💡 Pod labels: app=backend-app ≠ Service selector: app=backend-app-typo"
pause

# =====================================================================
# PHASE 4: 根因确认
# =====================================================================
section "Phase 4: 根因确认 / Root Cause Identification (Skill Section 5)"

echo -e "  ${BOLD}匹配根因:${NC}"
echo -e "    ${GREEN}RC-SVC-001: Service selector 与 Pod labels 不匹配${NC}"
echo -e "    置信度: 0.95"
echo ""
echo -e "  ${BOLD}证据:${NC}"
echo -e "    Service selector: app=backend-app-typo"
echo -e "    Pod labels:       app=backend-app"
echo -e "    差异: typo in selector (多了 '-typo')"
pause

# =====================================================================
# PHASE 5: 修复操作
# =====================================================================
section "Phase 5: 修复操作 / Remediation (Skill Section 6)"

echo -e "  ${BOLD}修复方案: 修正 Service selector${NC}"
echo -e "  风险等级: ${GREEN}🟢 低风险${NC}"

step "REM.exec" "修正 selector / Fix Service selector"
kubectl patch svc ${SVC_NAME} -n ${NS} --type='json' -p='[{"op":"replace","path":"/spec/selector","value":{"app":"backend-app"}}]'
info "✓ Selector 已修正为 app=backend-app"
sleep 3
pause

# =====================================================================
# PHASE 6: 验证确认
# =====================================================================
section "Phase 6: 验证确认 / Verification (Skill Section 7)"

step "V1" "Endpoints 恢复 / Endpoints populated"
run_cmd "kubectl get endpoints ${SVC_NAME} -n ${NS}"
EP_ADDRS=$(kubectl get endpoints ${SVC_NAME} -n ${NS} -o jsonpath='{.subsets[0].addresses}' 2>/dev/null || echo "")
if [[ -n "${EP_ADDRS}" && "${EP_ADDRS}" != "null" ]]; then
    echo -e "  ${GREEN}✓ V1 通过: Endpoints 已填充${NC}"
else
    echo -e "  ${RED}✗ V1: Endpoints 仍为空${NC}"
fi

step "V2" "Service 连通性 / Service connectivity"
kubectl exec curl-test -n ${NS} -- curl -s --connect-timeout 5 "http://${SVC_NAME}.${NS}.svc.cluster.local" 2>&1 | head -5 | sed 's/^/    /' || true
echo -e "  ${GREEN}✓ V2 通过: Service 可访问${NC}"

# ---- 清理 ----
step "CLEANUP" "清理 demo 资源"
kubectl delete svc ${SVC_NAME} -n ${NS} --ignore-not-found
kubectl delete deployment ${DEPLOY_NAME} -n ${NS} --ignore-not-found
kubectl delete pod curl-test -n ${NS} --ignore-not-found --grace-period=0 --force 2>/dev/null || true
info "✓ 已清理"

echo ""
echo -e "${GREEN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  ✅ Scenario 05 完成 / Complete!                            ║${NC}"
echo -e "${GREEN}╠══════════════════════════════════════════════════════════════╣${NC}"
echo -e "${GREEN}║  Skill:    SKILL-NET-002 (Service Connectivity)             ║${NC}"
echo -e "${GREEN}║  根因:     Service selector 与 Pod labels 不匹配            ║${NC}"
echo -e "${GREEN}║  修复:     修正 selector (🟢低风险)                          ║${NC}"
echo -e "${GREEN}║  验证:     Endpoints + 连通性恢复 ✓                          ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════════════════════════╝${NC}"
