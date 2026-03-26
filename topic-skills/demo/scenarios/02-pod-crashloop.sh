#!/usr/bin/env bash
# ============================================================================
# 02-pod-crashloop.sh — 场景: Pod CrashLoopBackOff & OOMKilled
# Scenario: Pod CrashLoopBackOff — maps to SKILL-POD-001
# ============================================================================
# 演示 Skill 执行流程:
#   1. 故障注入 (Inject)    — 部署一个必定 CrashLoop 的 Pod
#   2. 症状检测 (Detect)    — Pod 状态 CrashLoopBackOff
#   3. 快速分级 (Triage)    — 影响评估
#   4. 诊断工作流 (Diagnose) — 容器日志/事件分析
#   5. 根因确认 (Root Cause) — 启动命令错误
#   6. 修复操作 (Remediate)  — 修正 Deployment 配置
#   7. 验证确认 (Verify)     — Pod 恢复 Running
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

section() { echo -e "\n${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"; echo -e "${BOLD}${BLUE}$1${NC}"; echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"; }
step()    { echo -e "\n${MAGENTA}▸ [$1] $2${NC}"; }
info()    { echo -e "  ${GREEN}ℹ${NC} $1"; }
warn()    { echo -e "  ${YELLOW}⚠${NC} $1"; }
run_cmd() { echo -e "  ${CYAN}\$ $1${NC}"; eval "$1" 2>&1 | sed 's/^/    /'; }
pause()   { echo -e "\n  ${YELLOW}按 Enter 继续 / Press Enter to continue...${NC}"; read -r; }

NS="skill-demo"
DEPLOY_NAME="crashloop-demo"

echo -e "${CYAN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║  📋 Scenario 02: Pod CrashLoopBackOff (SKILL-POD-001)      ║${NC}"
echo -e "${CYAN}╚══════════════════════════════════════════════════════════════╝${NC}"

# =====================================================================
# PHASE 0: 故障注入 / Fault Injection
# =====================================================================
section "Phase 0: 故障注入 / Fault Injection"

step "INJECT" "部署一个启动命令错误的 Pod / Deploying a pod with bad command"
kubectl apply -f - <<EOF
apiVersion: apps/v1
kind: Deployment
metadata:
  name: ${DEPLOY_NAME}
  namespace: ${NS}
  labels:
    app: crashloop-demo
    scenario: skill-pod-001
spec:
  replicas: 1
  selector:
    matchLabels:
      app: crashloop-demo
  template:
    metadata:
      labels:
        app: crashloop-demo
    spec:
      containers:
        - name: app
          image: busybox:1.36
          command: ["sh", "-c", "echo 'Application starting...'; exit 1"]
          resources:
            requests:
              cpu: 10m
              memory: 16Mi
            limits:
              cpu: 50m
              memory: 32Mi
EOF

info "等待 Pod 进入 CrashLoopBackOff 状态 (约 30 秒)..."
sleep 15
echo -e "  等待中... (15s/30s)"
sleep 15
info "注入完成。"
pause

# =====================================================================
# PHASE 1: 症状检测 — Skill Section 2
# =====================================================================
section "Phase 1: 症状检测 / Symptom Detection (Skill Section 2)"

step "S1" "检查 Pod 状态 / Check Pod status (置信度: 0.95)"
run_cmd "kubectl get pods -n ${NS} -l app=crashloop-demo"
echo ""
info "💡 Pod 状态显示 CrashLoopBackOff → SKILL-POD-001 激活"

step "S2" "检查容器重启次数 / Check container restart count"
RESTARTS=$(kubectl get pods -n ${NS} -l app=crashloop-demo -o jsonpath='{.items[0].status.containerStatuses[0].restartCount}' 2>/dev/null || echo "0")
info "重启次数: ${RESTARTS}"
if (( RESTARTS >= 3 )); then
    info "💡 重启次数 >= 3 → CrashLoop 确认"
fi

step "S3" "检查退出码 / Check exit code"
run_cmd "kubectl get pods -n ${NS} -l app=crashloop-demo -o jsonpath='{.items[0].status.containerStatuses[0].lastState.terminated.exitCode}'"
echo ""
info "💡 Exit code 1 → 应用程序错误 (非 OOMKilled)"
info "   若 Exit code 137 → OOMKilled 路径"
pause

# =====================================================================
# PHASE 2: 快速分级 — Skill Section 3
# =====================================================================
section "Phase 2: 快速分级 / Quick Triage (Skill Section 3)"

step "T1" "影响评估: 故障 Pod 数量 / Impact: affected pod count"
run_cmd "kubectl get pods -n ${NS} -l app=crashloop-demo --no-headers | wc -l"

step "T2" "是否为关键服务 / Is critical service?"
info "deployment: ${DEPLOY_NAME} → demo 应用，非关键服务"

step "T3" "严重性分级 / Severity classification"
echo -e "  ${GREEN}🟢 P3 — 单个非关键 Pod CrashLoop${NC}"
info "若为关键业务 Pod → P1; 若影响大量副本 → P0"
pause

# =====================================================================
# PHASE 3: 诊断工作流 — Skill Section 4
# =====================================================================
section "Phase 3: 诊断工作流 / Diagnostic Workflow (Skill Section 4)"

step "D1.1" "Pod 详细状态 / Pod detailed status"
POD_NAME=$(kubectl get pods -n ${NS} -l app=crashloop-demo -o jsonpath='{.items[0].metadata.name}')
run_cmd "kubectl describe pod ${POD_NAME} -n ${NS} | tail -20"

step "D1.2" "容器日志 / Container logs (当前 + 前一次)"
info "当前日志 / Current logs:"
run_cmd "kubectl logs ${POD_NAME} -n ${NS} --tail=10 2>/dev/null || echo '  (容器未运行)'"
echo ""
info "前一次日志 / Previous logs:"
run_cmd "kubectl logs ${POD_NAME} -n ${NS} --previous --tail=10 2>/dev/null || echo '  (无前一次日志)'"

step "D1.3" "Events 分析 / Events analysis"
run_cmd "kubectl get events -n ${NS} --field-selector involvedObject.name=${POD_NAME} --sort-by='.lastTimestamp' | tail -10"

step "D1.4" "容器资源使用 / Container resource usage"
info "检查是否为 OOMKilled..."
OOM_CHECK=$(kubectl get pod "${POD_NAME}" -n "${NS}" -o jsonpath='{.status.containerStatuses[0].lastState.terminated.reason}' 2>/dev/null || echo "")
if [[ "${OOM_CHECK}" == "OOMKilled" ]]; then
    echo -e "  ${RED}🔴 OOMKilled 确认! → 需要增加内存 limits${NC}"
else
    info "✓ 非 OOMKilled (reason: ${OOM_CHECK:-Error})"
fi
pause

# =====================================================================
# PHASE 4: 根因确认 — Skill Section 5
# =====================================================================
section "Phase 4: 根因确认 / Root Cause Identification (Skill Section 5)"

info "根据诊断证据匹配:"
echo ""
echo -e "  ${BOLD}诊断证据:${NC}"
echo -e "    D1.2: 日志显示 'Application starting...' 后退出"
echo -e "    D1.3: Exit code = 1 (应用错误)"
echo -e "    D1.4: 非 OOMKilled"
echo ""
echo -e "  ${BOLD}匹配根因:${NC}"
echo -e "    ${GREEN}RC-APP-001: 应用启动命令/配置错误${NC}"
echo -e "    置信度: 0.90"
echo ""
echo -e "  ${BOLD}排除项:${NC}"
echo -e "    ✗ RC-OOM: Exit code != 137"
echo -e "    ✗ RC-IMG: 镜像拉取成功"
echo -e "    ✗ RC-CONFIG: 无 ConfigMap/Secret 挂载失败"
pause

# =====================================================================
# PHASE 5: 修复操作 — Skill Section 6
# =====================================================================
section "Phase 5: 修复操作 / Remediation (Skill Section 6)"

echo -e "  ${BOLD}修复方案: 修正容器启动命令${NC}"
echo -e "  风险等级: ${GREEN}🟢 低风险${NC}"
echo ""

step "REM.exec" "修正 Deployment 配置 / Fix Deployment config"
kubectl patch deployment ${DEPLOY_NAME} -n ${NS} --type='json' -p='[
  {"op": "replace", "path": "/spec/template/spec/containers/0/command", "value": ["sh", "-c", "echo Application started successfully; while true; do echo heartbeat; sleep 30; done"]}
]'
info "✓ 已修正启动命令"

info "等待 rollout 完成..."
kubectl rollout status deployment/${DEPLOY_NAME} -n ${NS} --timeout=90s
pause

# =====================================================================
# PHASE 6: 验证确认 — Skill Section 7
# =====================================================================
section "Phase 6: 验证确认 / Verification (Skill Section 7)"

step "V1" "Pod 状态 = Running / Pod STATUS = Running"
run_cmd "kubectl get pods -n ${NS} -l app=crashloop-demo"
POD_STATUS=$(kubectl get pods -n ${NS} -l app=crashloop-demo -o jsonpath='{.items[0].status.phase}')
if [[ "${POD_STATUS}" == "Running" ]]; then
    echo -e "  ${GREEN}✓ V1 通过: Pod Running${NC}"
else
    echo -e "  ${RED}✗ V1 失败: Pod ${POD_STATUS}${NC}"
fi

step "V2" "容器日志正常 / Container logs normal"
NEW_POD=$(kubectl get pods -n ${NS} -l app=crashloop-demo -o jsonpath='{.items[0].metadata.name}')
run_cmd "kubectl logs ${NEW_POD} -n ${NS} --tail=5"

step "V3" "重启次数归零 / Restart count = 0"
NEW_RESTARTS=$(kubectl get pods -n ${NS} -l app=crashloop-demo -o jsonpath='{.items[0].status.containerStatuses[0].restartCount}' 2>/dev/null || echo "?")
info "新 Pod 重启次数: ${NEW_RESTARTS}"

# ---- 清理 / Cleanup ----
step "CLEANUP" "清理 demo 资源 / Cleaning up"
kubectl delete deployment ${DEPLOY_NAME} -n ${NS} --ignore-not-found
info "✓ 已清理"

# =====================================================================
# 完成 / Complete
# =====================================================================
echo ""
echo -e "${GREEN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  ✅ Scenario 02 完成 / Complete!                            ║${NC}"
echo -e "${GREEN}╠══════════════════════════════════════════════════════════════╣${NC}"
echo -e "${GREEN}║  Skill:    SKILL-POD-001 (Pod CrashLoopBackOff)             ║${NC}"
echo -e "${GREEN}║  根因:     应用启动命令错误 (exit code 1)                    ║${NC}"
echo -e "${GREEN}║  修复:     修正启动命令 (🟢低风险)                           ║${NC}"
echo -e "${GREEN}║  验证:     Pod 恢复 Running ✓                               ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════════════════════════╝${NC}"
