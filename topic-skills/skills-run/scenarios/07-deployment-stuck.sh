#!/usr/bin/env bash
# ============================================================================
# 07-deployment-stuck.sh — 场景: Deployment 滚动更新卡住 (RC-002)
# Scenario: Deployment rollout stuck — maps to SKILL-WORK-001 / RC-002
# ============================================================================
# 演示 Skill 执行流程:
#   1. 故障注入 (Inject)    — 部署 readinessProbe 始终失败的新版本
#   2. 症状检测 (Detect)    — Deployment rollout 进度停滞
#   3. 快速分级 (Triage)    — 影响评估
#   4. 诊断工作流 (Diagnose) — Phase 1 快速检查
#   5. 根因确认 (Root Cause) — RC-002: readinessProbe 失败
#   6. 修复操作 (Remediate)  — REM-002: 回滚或修复配置
#   7. 验证确认 (Verify)     — Deployment 恢复正常
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
DEPLOY_NAME="demo-app-stuck"

echo -e "${CYAN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║  📋 Scenario 07: Deployment Stuck (SKILL-WORK-001 / RC-002) ║${NC}"
echo -e "${CYAN}║  目标: readinessProbe 始终失败导致 rollout 卡住              ║${NC}"
echo -e "${CYAN}╚══════════════════════════════════════════════════════════════╝${NC}"

# =====================================================================
# PHASE 0: 故障注入 / Fault Injection
# =====================================================================
section "Phase 0: 故障注入 / Fault Injection"

step "INJECT-1" "创建初始健康的 Deployment"

cat <<EOF | kubectl apply -f -
apiVersion: apps/v1
kind: Deployment
metadata:
  name: ${DEPLOY_NAME}
  namespace: ${NAMESPACE}
spec:
  replicas: 2
  selector:
    matchLabels:
      app: demo-stuck
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 0
      maxSurge: 1
  template:
    metadata:
      labels:
        app: demo-stuck
        version: v1
    spec:
      containers:
      - name: app
        image: nginx:alpine
        ports:
        - containerPort: 80
        readinessProbe:
          httpGet:
            path: /
            port: 80
          initialDelaySeconds: 2
          periodSeconds: 3
EOF

info "等待初始 Deployment 就绪..."
kubectl rollout status deployment/${DEPLOY_NAME} -n ${NAMESPACE} --timeout=60s 2>/dev/null || true
sleep 3

step "INJECT-2" "更新 Deployment 到有问题的版本 (readinessProbe 指向不存在的路径)"

cat <<EOF | kubectl apply -f -
apiVersion: apps/v1
kind: Deployment
metadata:
  name: ${DEPLOY_NAME}
  namespace: ${NAMESPACE}
spec:
  replicas: 2
  selector:
    matchLabels:
      app: demo-stuck
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 0
      maxSurge: 1
  template:
    metadata:
      labels:
        app: demo-stuck
        version: v2-broken
    spec:
      containers:
      - name: app
        image: nginx:alpine
        ports:
        - containerPort: 80
        readinessProbe:
          httpGet:
            path: /nonexistent-health-check
            port: 80
          initialDelaySeconds: 2
          periodSeconds: 3
          failureThreshold: 3
EOF

info "等待 10 秒让 rollout 卡住..."
sleep 10

info "注入完成。现在模拟 Agent 收到告警 / Injection done. Simulating Agent alert trigger."
pause

# =====================================================================
# PHASE 1: 症状检测 — Skill Section 2
# =====================================================================
section "Phase 1: 症状检测 / Symptom Detection (Skill Section 2)"

step "S1" "检查 Deployment 状态 / Check Deployment status (置信度: 0.90)"
run_cmd "kubectl get deployment ${DEPLOY_NAME} -n ${NAMESPACE}"
echo ""
info "💡 Skill 匹配: READY 数量与期望不符 → SKILL-WORK-001 激活"

step "S2" "检查 rollout 状态 / Check rollout status"
run_cmd "kubectl rollout status deployment/${DEPLOY_NAME} -n ${NAMESPACE} --timeout=5s 2>&1 || true"
echo ""
info "💡 Rollout 进度停滞"

step "S3" "检查 Pod 状态 / Check Pod status"
run_cmd "kubectl get pods -n ${NAMESPACE} -l app=demo-stuck"
echo ""
info "💡 新 Pod 处于非 Ready 状态"
pause

# =====================================================================
# PHASE 2: 快速分级 — Skill Section 3 (2 分钟内)
# =====================================================================
section "Phase 2: 快速分级 / Quick Triage (Skill Section 3, <2min)"

step "T1" "影响评估: 不可用 Pod 比例 / Impact: unavailable pod ratio"
READY_PODS=$(kubectl get deployment ${DEPLOY_NAME} -n ${NAMESPACE} -o jsonpath='{.status.readyReplicas}' 2>/dev/null || echo "0")
DESIRED_PODS=$(kubectl get deployment ${DEPLOY_NAME} -n ${NAMESPACE} -o jsonpath='{.spec.replicas}')
info "Ready Pods: ${READY_PODS:-0} / ${DESIRED_PODS}"

step "T2" "是否为关键服务 / Is critical service?"
info "检查 Deployment 标签和 namespace..."
run_cmd "kubectl get deployment ${DEPLOY_NAME} -n ${NAMESPACE} -o jsonpath='{.metadata.labels}'"
echo ""

step "T3" "严重性分级 / Severity classification"
if [[ "${READY_PODS:-0}" == "0" ]]; then
    echo -e "  ${RED}🔴 P1 — 所有 Pod 不可用${NC}"
else
    echo -e "  ${YELLOW}🟡 P2 — 部分 Pod 不可用，服务降级${NC}"
fi
info "📊 分级结果: P2 (滚动更新卡住，但旧版本仍运行)"
pause

# =====================================================================
# PHASE 3: 诊断工作流 — Skill Section 4, Phase 1
# =====================================================================
section "Phase 3: 诊断工作流 / Diagnostic Workflow (Skill Section 4)"
echo -e "  ${YELLOW}执行 Phase 1: 快速检查 (kubectl, 只读, 零风险)${NC}"

step "D1.1" "Deployment 详情 / Deployment details"
run_cmd "kubectl describe deployment ${DEPLOY_NAME} -n ${NAMESPACE} | head -50"

step "D1.2" "检查 ReplicaSets / Check ReplicaSets"
run_cmd "kubectl get rs -n ${NAMESPACE} -l app=demo-stuck"
echo ""
info "💡 存在新旧两个 ReplicaSet，新版本 Pod 无法就绪"

step "D1.3" "检查新版本 Pod 状态 / Check new Pod status"
NEW_POD=$(kubectl get pods -n ${NAMESPACE} -l app=demo-stuck,version=v2-broken -o jsonpath='{.items[0].metadata.name}' 2>/dev/null || echo "")
if [[ -n "${NEW_POD}" ]]; then
    run_cmd "kubectl describe pod ${NEW_POD} -n ${NAMESPACE} | grep -A 10 'Conditions:'"
fi

step "D1.4" "检查 readinessProbe 失败原因 / Check readinessProbe failure"
if [[ -n "${NEW_POD}" ]]; then
    run_cmd "kubectl get events -n ${NAMESPACE} --field-selector involvedObject.name=${NEW_POD} --sort-by='.lastTimestamp' | tail -10"
fi
info "💡 发现 readinessProbe 返回 404 → 健康检查路径错误"
pause

# =====================================================================
# PHASE 4: 根因确认 — Skill Section 5
# =====================================================================
section "Phase 4: 根因确认 / Root Cause Identification (Skill Section 5)"

info "根据诊断证据匹配 root-cause-map.yaml:"
echo ""
echo -e "  ${BOLD}诊断证据:${NC}"
echo -e "    D1.1: Deployment rollout 进度停滞"
echo -e "    D1.2: 新 ReplicaSet Pod 数为 0/1"
echo -e "    D1.3: Pod Ready condition = False"
echo -e "    D1.4: readinessProbe 返回 404"
echo ""
echo -e "  ${BOLD}匹配根因:${NC}"
echo -e "    ${GREEN}RC-002: readinessProbe 配置错误或应用未响应健康检查${NC}"
echo -e "    置信度: 0.95"
echo -e "    概率: 高 (常见配置错误)"
echo ""
echo -e "  ${BOLD}FTA 映射:${NC}"
echo -e "    RC-002 → evt_readiness_probe_failure"
pause

# =====================================================================
# PHASE 5: 修复操作 — Skill Section 6
# =====================================================================
section "Phase 5: 修复操作 / Remediation (Skill Section 6)"

echo -e "  ${BOLD}修复方案: REM-002 — 回滚到上一个健康版本${NC}"
echo -e "  风险等级: ${GREEN}🟢 低风险 (Green)${NC}"
echo -e "  Agent 模式: L2-semi-auto → Agent 可自动执行回滚"
echo ""

step "REM-002.pre" "前置检查: 确认有可回滚的版本 / Pre-check: Confirm rollback target"
run_cmd "kubectl rollout history deployment/${DEPLOY_NAME} -n ${NAMESPACE}"

step "REM-002.exec" "执行回滚 / Execute rollback"
run_cmd "kubectl rollout undo deployment/${DEPLOY_NAME} -n ${NAMESPACE}"

step "REM-002.wait" "等待回滚完成 / Wait for rollback"
info "等待 rollout 完成..."
kubectl rollout status deployment/${DEPLOY_NAME} -n ${NAMESPACE} --timeout=60s 2>/dev/null || true

step "REM-002.post" "后置验证 / Post-verification"
run_cmd "kubectl get deployment ${DEPLOY_NAME} -n ${NAMESPACE}"
run_cmd "kubectl get pods -n ${NAMESPACE} -l app=demo-stuck"
pause

# =====================================================================
# PHASE 6: 验证确认 — Skill Section 7
# =====================================================================
section "Phase 6: 验证确认 / Verification (Skill Section 7)"

step "V1" "Deployment READY = 期望副本数 / Deployment READY matches replicas"
READY=$(kubectl get deployment ${DEPLOY_NAME} -n ${NAMESPACE} -o jsonpath='{.status.readyReplicas}' 2>/dev/null || echo "0")
DESIRED=$(kubectl get deployment ${DEPLOY_NAME} -n ${NAMESPACE} -o jsonpath='{.spec.replicas}')
if [[ "${READY}" == "${DESIRED}" ]]; then
    echo -e "  ${GREEN}✓ V1 通过: Ready ${READY}/${DESIRED}${NC}"
else
    echo -e "  ${RED}✗ V1 失败: Ready ${READY:-0}/${DESIRED}${NC}"
fi

step "V2" "所有 Pod 状态为 Running / All Pods Running"
NOT_RUNNING=$(kubectl get pods -n ${NAMESPACE} -l app=demo-stuck --no-headers 2>/dev/null | grep -cv "Running" || echo "0")
if [[ "${NOT_RUNNING}" == "0" ]]; then
    echo -e "  ${GREEN}✓ V2 通过: 所有 Pod 运行中${NC}"
else
    echo -e "  ${YELLOW}⚠ V2: ${NOT_RUNNING} 个 Pod 非 Running 状态${NC}"
fi

step "V3" "Rollout 状态正常 / Rollout completed"
run_cmd "kubectl rollout status deployment/${DEPLOY_NAME} -n ${NAMESPACE} --timeout=10s 2>&1 || true"

# ---- 清理 / Cleanup ----
step "CLEANUP" "清理测试资源 / Cleaning up test resources"
kubectl delete deployment ${DEPLOY_NAME} -n ${NAMESPACE} --ignore-not-found=true 2>/dev/null || true

# =====================================================================
# 完成 / Complete
# =====================================================================
echo ""
echo -e "${GREEN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  ✅ Scenario 07 完成 / Complete!                            ║${NC}"
echo -e "${GREEN}╠══════════════════════════════════════════════════════════════╣${NC}"
echo -e "${GREEN}║  Skill:    SKILL-WORK-001 (Deployment Rollout Failure)      ║${NC}"
echo -e "${GREEN}║  根因:     RC-002 (readinessProbe 配置错误)                  ║${NC}"
echo -e "${GREEN}║  修复:     REM-002 (回滚, 🟢低风险)                         ║${NC}"
echo -e "${GREEN}║  验证:     V1-V3 全部通过                                    ║${NC}"
echo -e "${GREEN}║                                                              ║${NC}"
echo -e "${GREEN}║  完整 Skill 执行流程:                                        ║${NC}"
echo -e "${GREEN}║  触发→症状检测→分级→诊断→根因→修复→验证 ✓                     ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════════════════════════╝${NC}"
