#!/usr/bin/env bash
# ============================================================================
# 03-pod-pending.sh — 场景: Pod Pending / 调度失败
# Scenario: Pod Pending — maps to SKILL-POD-002
# ============================================================================
# 演示: 部署资源请求超出可用容量的 Pod → FailedScheduling → 修复
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
run_cmd() {
    echo -e "  ${CYAN}\$ $1${NC}"
    ( bash -c "$1" 2>&1 | sed 's/^/    /' ) || {
        echo -e "    ${RED}[命令失败 / Command failed with exit code $?]${NC}"
    }
}
pause()   { echo -e "\n  ${YELLOW}按 Enter 继续 / Press Enter to continue...${NC}"; read -r; }

NS="skill-demo"
POD_NAME="pending-demo"

# ---- trap 清理: 脚本退出时自动清理资源 ----
cleanup() {
    echo -e "\n${YELLOW}正在清理 / Cleaning up...${NC}"
    kubectl delete pod ${POD_NAME} -n ${NS} --ignore-not-found=true 2>/dev/null || true
}
trap cleanup EXIT ERR

echo -e "${CYAN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║  📋 Scenario 03: Pod Pending (SKILL-POD-002)               ║${NC}"
echo -e "${CYAN}╚══════════════════════════════════════════════════════════════╝${NC}"

# =====================================================================
# PHASE 0: 故障注入
# =====================================================================
section "Phase 0: 故障注入 / Fault Injection"

step "INJECT" "部署资源请求过大的 Pod / Deploying pod with excessive resource requests"
kubectl apply -f - <<EOF
apiVersion: v1
kind: Pod
metadata:
  name: ${POD_NAME}
  namespace: ${NS}
  labels:
    app: pending-demo
    scenario: skill-pod-002
spec:
  containers:
    - name: app
      image: nginx:1.27-alpine
      resources:
        requests:
          cpu: "100"
          memory: "512Gi"
        limits:
          cpu: "100"
          memory: "512Gi"
EOF

info "等待 Pod 进入 Pending 状态..."
sleep 5
pause

# =====================================================================
# PHASE 1: 症状检测
# =====================================================================
section "Phase 1: 症状检测 / Symptom Detection (Skill Section 2)"

step "S1" "Pod 状态检查 / Pod status (置信度: 0.95)"
run_cmd "kubectl get pod ${POD_NAME} -n ${NS}"
info "💡 Pod 处于 Pending 状态 → SKILL-POD-002 激活"

step "S2" "Events 检查 / Events (FailedScheduling)"
run_cmd "kubectl get events -n ${NS} --field-selector involvedObject.name=${POD_NAME},reason=FailedScheduling --sort-by='.lastTimestamp'"
info "💡 FailedScheduling event → 调度失败确认"
pause

# =====================================================================
# PHASE 2: 快速分级
# =====================================================================
section "Phase 2: 快速分级 / Quick Triage (Skill Section 3)"

step "T1" "Pending Pod 数量 / Pending pod count"
PENDING_COUNT=$(kubectl get pods -n ${NS} --field-selector status.phase=Pending --no-headers | wc -l | tr -d ' ')
info "Pending Pod 数量: ${PENDING_COUNT}"

step "T2" "严重性分级 / Severity classification"
echo -e "  ${GREEN}🟢 P3 — 单个 Pod Pending，非关键服务${NC}"
pause

# =====================================================================
# PHASE 3: 诊断工作流
# =====================================================================
section "Phase 3: 诊断工作流 / Diagnostic Workflow (Skill Section 4)"

step "D1.1" "Pod describe / Pod 详情"
run_cmd "kubectl describe pod ${POD_NAME} -n ${NS} | grep -A 5 'Events\|Conditions\|Requests\|Limits'"

step "D1.2" "集群资源容量 / Cluster resource capacity"
run_cmd "kubectl top nodes 2>/dev/null || kubectl describe nodes | grep -A 5 'Allocatable' | head -20"

step "D1.3" "调度失败原因分析 / Scheduling failure analysis"
EVENTS_MSG=$(kubectl get events -n ${NS} --field-selector involvedObject.name=${POD_NAME},reason=FailedScheduling -o jsonpath='{.items[0].message}' 2>/dev/null || echo "")
info "调度器消息: ${EVENTS_MSG}"
echo ""
info "💡 分析: Pod 请求 100 CPU / 512Gi 内存 → 远超集群容量"
pause

# =====================================================================
# PHASE 4: 根因确认
# =====================================================================
section "Phase 4: 根因确认 / Root Cause Identification (Skill Section 5)"

echo -e "  ${BOLD}匹配根因:${NC}"
echo -e "    ${GREEN}RC-SCHED-001: 资源请求超出集群可用容量${NC}"
echo -e "    置信度: 0.95"
echo ""
echo -e "  ${BOLD}排除项:${NC}"
echo -e "    ✗ RC-AFFINITY: 无 nodeSelector/affinity 限制"
echo -e "    ✗ RC-TAINT: 无 taint 阻止调度"
echo -e "    ✗ RC-PVC: 无 PVC 绑定问题"
pause

# =====================================================================
# PHASE 5: 修复操作
# =====================================================================
section "Phase 5: 修复操作 / Remediation (Skill Section 6)"

echo -e "  ${BOLD}修复方案: 调整资源请求至合理范围${NC}"
echo -e "  风险等级: ${GREEN}🟢 低风险${NC}"
echo ""

step "REM.exec" "删除并重建 Pod / Recreate pod with correct resources"
kubectl delete pod ${POD_NAME} -n ${NS} --ignore-not-found
kubectl apply -f - <<EOF
apiVersion: v1
kind: Pod
metadata:
  name: ${POD_NAME}
  namespace: ${NS}
  labels:
    app: pending-demo
    scenario: skill-pod-002
spec:
  containers:
    - name: app
      image: nginx:1.27-alpine
      resources:
        requests:
          cpu: 50m
          memory: 64Mi
        limits:
          cpu: 100m
          memory: 128Mi
EOF

info "等待 Pod 调度并启动..."
kubectl wait --for=condition=Ready pod/${POD_NAME} -n ${NS} --timeout=60s
pause

# =====================================================================
# PHASE 6: 验证确认
# =====================================================================
section "Phase 6: 验证确认 / Verification (Skill Section 7)"

step "V1" "Pod 状态 = Running"
run_cmd "kubectl get pod ${POD_NAME} -n ${NS}"
POD_STATUS=$(kubectl get pod ${POD_NAME} -n ${NS} -o jsonpath='{.status.phase}')
echo -e "  ${GREEN}✓ V1 通过: Pod ${POD_STATUS}${NC}"

step "V2" "Pod 已调度到节点 / Pod scheduled to node"
NODE=$(kubectl get pod ${POD_NAME} -n ${NS} -o jsonpath='{.spec.nodeName}')
info "调度到节点: ${NODE}"

# ---- 清理 ----
step "CLEANUP" "清理 demo 资源"
kubectl delete pod ${POD_NAME} -n ${NS} --ignore-not-found
info "✓ 已清理"

echo ""
echo -e "${GREEN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  ✅ Scenario 03 完成 / Complete!                            ║${NC}"
echo -e "${GREEN}╠══════════════════════════════════════════════════════════════╣${NC}"
echo -e "${GREEN}║  Skill:    SKILL-POD-002 (Pod Pending)                      ║${NC}"
echo -e "${GREEN}║  根因:     资源请求超出集群容量                              ║${NC}"
echo -e "${GREEN}║  修复:     调整资源请求 (🟢低风险)                           ║${NC}"
echo -e "${GREEN}║  验证:     Pod 成功调度并 Running ✓                          ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════════════════════════╝${NC}"
