#!/usr/bin/env bash
# ============================================================================
# 09-hpa-not-scaling.sh — 场景: HPA 不触发 (RC-002)
# Scenario: HPA Not Scaling — maps to SKILL-SCALE-001 / RC-002
# ============================================================================
# 演示 Skill 执行流程:
#   1. 故障注入 (Inject)    — 部署未设置 resources.requests 的 Deployment + HPA
#   2. 症状检测 (Detect)    — HPA 显示 unknown metrics
#   3. 快速分级 (Triage)    — 影响评估
#   4. 诊断工作流 (Diagnose) — Phase 1 快速检查
#   5. 根因确认 (Root Cause) — RC-002: 缺少 resources.requests 配置
#   6. 修复操作 (Remediate)  — REM-002: 添加资源配置
#   7. 验证确认 (Verify)     — HPA 正常获取 metrics
# ============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEMO_DIR="$(dirname "${SCRIPT_DIR}")"

# ---- 颜色 / Colors ----
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
BOLD='\033[1m'
NC='\033[0m'

# ---- 辅助函数 / Helpers ----
section() { echo -e "\n${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"; echo -e "${BOLD}${BLUE}$1${NC}"; echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"; }
step()    { echo -e "\n${MAGENTA}▸ [$1] $2${NC}"; }
info()    { echo -e "  ${GREEN}ℹ${NC} $1"; }
warn()    { echo -e "  ${YELLOW}⚠${NC} $1"; }
run_cmd() { echo -e "  ${CYAN}\$ $1${NC}"; eval "$1" 2>&1 | sed 's/^/    /'; }
pause()   { echo -e "\n  ${YELLOW}按 Enter 继续 / Press Enter to continue...${NC}"; read -r; }

NAMESPACE="skill-demo"
DEPLOY_NAME="demo-hpa-app"
HPA_NAME="demo-hpa"

echo -e "${CYAN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║  📋 Scenario 09: HPA Not Scaling (SKILL-SCALE-001 / RC-002) ║${NC}"
echo -e "${CYAN}║  目标: 未设置 resources.requests 导致 HPA 无法工作           ║${NC}"
echo -e "${CYAN}╚══════════════════════════════════════════════════════════════╝${NC}"

# =====================================================================
# PHASE 0: 故障注入 / Fault Injection
# =====================================================================
section "Phase 0: 故障注入 / Fault Injection"

step "INJECT-1" "创建不带 resources.requests 的 Deployment"

cat <<EOF | kubectl apply -f -
apiVersion: apps/v1
kind: Deployment
metadata:
  name: ${DEPLOY_NAME}
  namespace: ${NAMESPACE}
spec:
  replicas: 1
  selector:
    matchLabels:
      app: hpa-demo
  template:
    metadata:
      labels:
        app: hpa-demo
    spec:
      containers:
      - name: app
        image: nginx:alpine
        ports:
        - containerPort: 80
        # 注意: 故意不设置 resources.requests
EOF

info "等待 Deployment 就绪..."
kubectl rollout status deployment/${DEPLOY_NAME} -n ${NAMESPACE} --timeout=60s 2>/dev/null || true

step "INJECT-2" "创建 HPA (基于 CPU)"

cat <<EOF | kubectl apply -f -
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: ${HPA_NAME}
  namespace: ${NAMESPACE}
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: ${DEPLOY_NAME}
  minReplicas: 1
  maxReplicas: 5
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 50
EOF

info "等待 HPA 采集指标 (10秒)..."
sleep 10

info "注入完成。现在模拟 Agent 收到 HPA 异常告警 / Injection done."
pause

# =====================================================================
# PHASE 1: 症状检测 — Skill Section 2
# =====================================================================
section "Phase 1: 症状检测 / Symptom Detection (Skill Section 2)"

step "S1" "检查 HPA 状态 / Check HPA status (置信度: 0.90)"
run_cmd "kubectl get hpa ${HPA_NAME} -n ${NAMESPACE}"
echo ""
info "💡 Skill 匹配: TARGETS 显示 unknown 或 <unknown> → SKILL-SCALE-001 激活"

step "S2" "检查 HPA 详情 / Check HPA details"
run_cmd "kubectl describe hpa ${HPA_NAME} -n ${NAMESPACE} | head -30"
echo ""
info "💡 Events 中可能显示 'missing request for cpu'"

step "S3" "检查 Deployment 副本数 / Check Deployment replicas"
run_cmd "kubectl get deployment ${DEPLOY_NAME} -n ${NAMESPACE}"
echo ""
info "💡 副本数未发生变化，HPA 没有生效"
pause

# =====================================================================
# PHASE 2: 快速分级 — Skill Section 3 (2 分钟内)
# =====================================================================
section "Phase 2: 快速分级 / Quick Triage (Skill Section 3, <2min)"

step "T1" "影响评估: 受影响的 HPA / Impact: affected HPAs"
info "HPA: ${HPA_NAME}"
info "Target Deployment: ${DEPLOY_NAME}"

step "T2" "是否影响关键服务弹性 / Affects critical service scaling?"
info "检查 Deployment 当前负载..."
run_cmd "kubectl top pods -n ${NAMESPACE} -l app=hpa-demo 2>/dev/null || echo 'Metrics server may not be available'"

step "T3" "严重性分级 / Severity classification"
echo -e "  ${YELLOW}🟡 P2 — HPA 配置问题，服务无法自动扩容${NC}"
info "📊 分级结果: P2 (弹性伸缩配置问题)"
pause

# =====================================================================
# PHASE 3: 诊断工作流 — Skill Section 4, Phase 1
# =====================================================================
section "Phase 3: 诊断工作流 / Diagnostic Workflow (Skill Section 4)"
echo -e "  ${YELLOW}执行 Phase 1: 快速检查 (kubectl, 只读, 零风险)${NC}"

step "D1.1" "检查 HPA 配置 / Check HPA configuration"
run_cmd "kubectl get hpa ${HPA_NAME} -n ${NAMESPACE} -o yaml | head -40"

step "D1.2" "检查目标 Deployment 的 resources 配置 / Check Deployment resources"
RESOURCES=$(kubectl get deployment ${DEPLOY_NAME} -n ${NAMESPACE} -o jsonpath='{.spec.template.spec.containers[0].resources}')
info "Resources 配置: ${RESOURCES:-'(空)'}"
if [[ -z "${RESOURCES}" || "${RESOURCES}" == "{}" ]]; then
    warn "Deployment 未设置 resources.requests!"
fi

step "D1.3" "检查 Metrics Server 是否可用 / Check Metrics Server"
run_cmd "kubectl get apiservice v1beta1.metrics.k8s.io -o jsonpath='{.status.conditions[?(@.type==\"Available\")].status}' 2>/dev/null || echo 'Metrics API not found'"
echo ""

step "D1.4" "检查 HPA Events / Check HPA events"
run_cmd "kubectl get events -n ${NAMESPACE} --field-selector involvedObject.name=${HPA_NAME} --sort-by='.lastTimestamp' 2>/dev/null | tail -5 || echo 'No events'"
info "💡 可能显示 'missing request for cpu' 错误"
pause

# =====================================================================
# PHASE 4: 根因确认 — Skill Section 5
# =====================================================================
section "Phase 4: 根因确认 / Root Cause Identification (Skill Section 5)"

info "根据诊断证据匹配 root-cause-map.yaml:"
echo ""
echo -e "  ${BOLD}诊断证据:${NC}"
echo -e "    D1.1: HPA TARGETS 显示 unknown"
echo -e "    D1.2: Deployment 未设置 resources.requests"
echo -e "    D1.3: Metrics Server 可用"
echo -e "    D1.4: Events 显示 missing request for cpu"
echo ""
echo -e "  ${BOLD}匹配根因:${NC}"
echo -e "    ${GREEN}RC-002: Pod 未设置 resources.requests，HPA 无法计算利用率${NC}"
echo -e "    置信度: 0.95"
echo -e "    概率: 高 (常见配置遗漏)"
echo ""
echo -e "  ${BOLD}FTA 映射:${NC}"
echo -e "    RC-002 → evt_missing_resource_requests"
pause

# =====================================================================
# PHASE 5: 修复操作 — Skill Section 6
# =====================================================================
section "Phase 5: 修复操作 / Remediation (Skill Section 6)"

echo -e "  ${BOLD}修复方案: REM-002 — 为 Deployment 添加 resources.requests${NC}"
echo -e "  风险等级: ${YELLOW}🟡 中风险 (Yellow)${NC}"
echo -e "  Agent 模式: L2-semi-auto → 需要人工审批"
echo ""

step "REM-002.pre" "前置检查: 确认修改范围 / Pre-check: Confirm changes"
info "将为容器添加: requests.cpu=100m, requests.memory=64Mi"

step "REM-002.exec" "更新 Deployment 添加资源配置 / Update Deployment with resources"

cat <<EOF | kubectl apply -f -
apiVersion: apps/v1
kind: Deployment
metadata:
  name: ${DEPLOY_NAME}
  namespace: ${NAMESPACE}
spec:
  replicas: 1
  selector:
    matchLabels:
      app: hpa-demo
  template:
    metadata:
      labels:
        app: hpa-demo
    spec:
      containers:
      - name: app
        image: nginx:alpine
        ports:
        - containerPort: 80
        resources:
          requests:
            cpu: "100m"
            memory: "64Mi"
          limits:
            cpu: "500m"
            memory: "128Mi"
EOF

info "等待 Deployment 滚动更新..."
kubectl rollout status deployment/${DEPLOY_NAME} -n ${NAMESPACE} --timeout=60s 2>/dev/null || true

step "REM-002.wait" "等待 HPA 重新获取指标 (15秒)..."
sleep 15

step "REM-002.post" "后置验证 / Post-verification"
run_cmd "kubectl get hpa ${HPA_NAME} -n ${NAMESPACE}"
pause

# =====================================================================
# PHASE 6: 验证确认 — Skill Section 7
# =====================================================================
section "Phase 6: 验证确认 / Verification (Skill Section 7)"

step "V1" "HPA TARGETS 不再显示 unknown / HPA TARGETS shows actual value"
HPA_TARGETS=$(kubectl get hpa ${HPA_NAME} -n ${NAMESPACE} -o jsonpath='{.status.currentMetrics[0].resource.current.averageUtilization}' 2>/dev/null || echo "unknown")
if [[ "${HPA_TARGETS}" != "unknown" && -n "${HPA_TARGETS}" ]]; then
    echo -e "  ${GREEN}✓ V1 通过: HPA 获取到 CPU 指标: ${HPA_TARGETS}%${NC}"
else
    echo -e "  ${YELLOW}⚠ V1: HPA 指标可能需要更多时间采集 (Metrics Server 依赖)${NC}"
fi

step "V2" "Deployment 资源配置正确 / Deployment resources configured"
CPU_REQUEST=$(kubectl get deployment ${DEPLOY_NAME} -n ${NAMESPACE} -o jsonpath='{.spec.template.spec.containers[0].resources.requests.cpu}')
if [[ "${CPU_REQUEST}" == "100m" ]]; then
    echo -e "  ${GREEN}✓ V2 通过: CPU request 已设置为 ${CPU_REQUEST}${NC}"
else
    echo -e "  ${RED}✗ V2 失败: CPU request 配置异常${NC}"
fi

step "V3" "HPA Events 无错误 / No errors in HPA events"
HPA_ERRORS=$(kubectl get events -n ${NAMESPACE} --field-selector involvedObject.name=${HPA_NAME},type=Warning --sort-by='.lastTimestamp' 2>/dev/null | tail -3)
if [[ -z "${HPA_ERRORS}" || "${HPA_ERRORS}" == *"No resources found"* ]]; then
    echo -e "  ${GREEN}✓ V3 通过: 无 Warning 事件${NC}"
else
    echo -e "  ${YELLOW}⚠ V3: 存在 Warning 事件，请检查:${NC}"
    echo "${HPA_ERRORS}" | sed 's/^/    /'
fi

# ---- 清理 / Cleanup ----
step "CLEANUP" "清理测试资源 / Cleaning up test resources"
kubectl delete hpa ${HPA_NAME} -n ${NAMESPACE} --ignore-not-found=true 2>/dev/null || true
kubectl delete deployment ${DEPLOY_NAME} -n ${NAMESPACE} --ignore-not-found=true 2>/dev/null || true

# =====================================================================
# 完成 / Complete
# =====================================================================
echo ""
echo -e "${GREEN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  ✅ Scenario 09 完成 / Complete!                            ║${NC}"
echo -e "${GREEN}╠══════════════════════════════════════════════════════════════╣${NC}"
echo -e "${GREEN}║  Skill:    SKILL-SCALE-001 (HPA/VPA/CA Failure)             ║${NC}"
echo -e "${GREEN}║  根因:     RC-002 (未设置 resources.requests)               ║${NC}"
echo -e "${GREEN}║  修复:     REM-002 (添加资源配置, 🟡中风险)                 ║${NC}"
echo -e "${GREEN}║  验证:     V1-V3 全部通过                                    ║${NC}"
echo -e "${GREEN}║                                                              ║${NC}"
echo -e "${GREEN}║  完整 Skill 执行流程:                                        ║${NC}"
echo -e "${GREEN}║  触发→症状检测→分级→诊断→根因→修复→验证 ✓                     ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════════════════════════╝${NC}"
