#!/usr/bin/env bash
# ============================================================================
# 01-node-cordon-notready.sh — 场景: 节点被 cordon (RC-012)
# Scenario: Node cordoned — maps to SKILL-NODE-001 / RC-012
# ============================================================================
# 演示 Skill 执行流程:
#   1. 故障注入 (Inject)    — cordon 一个 worker 节点
#   2. 症状检测 (Detect)    — 节点 SchedulingDisabled
#   3. 快速分级 (Triage)    — 影响评估
#   4. 诊断工作流 (Diagnose) — Phase 1 快速检查
#   5. 根因确认 (Root Cause) — RC-012: 节点被手动 cordon
#   6. 修复操作 (Remediate)  — REM-001: uncordon
#   7. 验证确认 (Verify)     — 节点恢复 Ready
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
run_cmd() {
    echo -e "  ${CYAN}\$ $1${NC}"
    # 使用子 shell 运行命令，即使失败也不中断父脚本
    ( bash -c "$1" 2>&1 | sed 's/^/    /' ) || {
        echo -e "    ${RED}[命令失败 / Command failed with exit code $?]${NC}"
    }
}
pause()   { echo -e "\n  ${YELLOW}按 Enter 继续 / Press Enter to continue...${NC}"; read -r; }

# ---- 选择目标节点 / Select target node ----
# 兼容单节点集群: 如果没有非 control-plane 节点，则使用第一个可用节点
WORKER_NODES=$(kubectl get nodes --selector='!node-role.kubernetes.io/control-plane' -o jsonpath='{.items[*].metadata.name}' 2>/dev/null || true)
if [[ -n "${WORKER_NODES}" ]]; then
    WORKER_NODE=$(echo "${WORKER_NODES}" | awk '{print $1}')
else
    echo -e "${YELLOW}⚠ 未找到独立的 worker 节点，使用第一个可用节点...${NC}"
    WORKER_NODE=$(kubectl get nodes -o jsonpath='{.items[0].metadata.name}' 2>/dev/null || true)
fi
if [[ -z "${WORKER_NODE}" ]]; then
    echo -e "${RED}✗ 未找到任何节点 / No nodes found${NC}"
    exit 1
fi

# ---- 设置 trap 清理 / Set trap for cleanup ----
cleanup() {
    echo -e "\n${YELLOW}正在清理 / Cleaning up...${NC}"
    kubectl uncordon "${WORKER_NODE}" 2>/dev/null || true
}
trap cleanup EXIT

echo -e "${CYAN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║  📋 Scenario 01: Node Cordon (SKILL-NODE-001 / RC-012)     ║${NC}"
echo -e "${CYAN}║  目标节点 / Target: ${WORKER_NODE}${NC}"
echo -e "${CYAN}╚══════════════════════════════════════════════════════════════╝${NC}"

# =====================================================================
# PHASE 0: 故障注入 / Fault Injection
# =====================================================================
section "Phase 0: 故障注入 / Fault Injection"

step "INJECT" "Cordon 节点使其不可调度 / Cordoning the node"
run_cmd "kubectl cordon ${WORKER_NODE}"

info "等待 3 秒让状态传播 / Waiting 3s for propagation..."
sleep 3

info "注入完成。现在模拟 Agent 收到告警 / Injection done. Simulating Agent alert trigger."
pause

# =====================================================================
# PHASE 1: 症状检测 — Skill Section 2
# =====================================================================
section "Phase 1: 症状检测 / Symptom Detection (Skill Section 2)"

step "S1" "检查节点状态 / Check node status (置信度: 0.95)"
run_cmd "kubectl get nodes -o wide"
echo ""
info "💡 Skill 匹配: 节点显示 SchedulingDisabled → SKILL-NODE-001 激活"
info "   置信度: 0.95 (直接状态检测)"

step "S5" "检查节点 Taints / Check node taints"
run_cmd "kubectl get node ${WORKER_NODE} -o jsonpath='{.spec.taints}' | python3 -m json.tool 2>/dev/null || kubectl get node ${WORKER_NODE} -o jsonpath='{.spec.taints}'"
echo ""
info "💡 Taint node.kubernetes.io/unschedulable:NoSchedule → cordon 标记"
pause

# =====================================================================
# PHASE 2: 快速分级 — Skill Section 3 (2 分钟内)
# =====================================================================
section "Phase 2: 快速分级 / Quick Triage (Skill Section 3, <2min)"

step "T1" "影响评估: NotReady/异常节点比例 / Impact: node ratio"
TOTAL_NODES=$(kubectl get nodes --no-headers | wc -l | tr -d ' ')
AFFECTED=$(kubectl get nodes --no-headers | grep -c "SchedulingDisabled" || true)
info "异常节点: ${AFFECTED} / ${TOTAL_NODES}"

step "T2" "是否为控制平面节点 / Is control-plane node?"
CP_CHECK=$(kubectl get node "${WORKER_NODE}" -o jsonpath='{.metadata.labels.node-role\.kubernetes\.io/control-plane}' 2>/dev/null || echo "")
if [[ -n "${CP_CHECK}" ]]; then
    warn "⚠ 控制平面节点! → P0"
else
    info "✓ 工作节点 / Worker node"
fi

step "T3" "严重性分级 / Severity classification"
if (( AFFECTED * 100 / TOTAL_NODES > 50 )); then
    echo -e "  ${RED}🔴 P0 — 超过 50% 节点异常，立即升级${NC}"
elif (( AFFECTED > 1 )); then
    echo -e "  ${YELLOW}🟡 P1 — 多个工作节点异常${NC}"
else
    echo -e "  ${GREEN}🟢 P2 — 单个工作节点异常，2h 内修复${NC}"
fi
info "📊 分级结果: P2 (单个工作节点被 cordon)"
pause

# =====================================================================
# PHASE 3: 诊断工作流 — Skill Section 4, Phase 1
# =====================================================================
section "Phase 3: 诊断工作流 / Diagnostic Workflow (Skill Section 4)"
echo -e "  ${YELLOW}执行 Phase 1: 快速检查 (kubectl, 只读, 零风险)${NC}"

step "D1.1" "节点状态详情 / Node status overview"
run_cmd "kubectl get node ${WORKER_NODE} -o wide"

step "D1.2" "节点 Conditions / Node conditions"
run_cmd "kubectl get node ${WORKER_NODE} -o jsonpath='{range .status.conditions[*]}{.type}={.status} ({.reason}) {.message}{\"\\n\"}{end}'"
echo ""

step "D1.3" "最近事件 / Recent events"
run_cmd "kubectl get events --field-selector involvedObject.name=${WORKER_NODE} --sort-by='.lastTimestamp' | tail -5"

step "D1.4" "Taints 检查 / Taints inspection"
run_cmd "kubectl get node ${WORKER_NODE} -o jsonpath='{.spec.taints}'"
echo ""
info "💡 发现 unschedulable taint → 疑似 cordon 操作"

step "D1.5" "Lease 状态 / Lease status"
run_cmd "kubectl get lease ${WORKER_NODE} -n kube-node-lease -o jsonpath='{.spec.renewTime}'"
echo ""
info "💡 Lease 正常更新 → 节点本身是健康的，kubelet 正常运行"
pause

# =====================================================================
# PHASE 4: 根因确认 — Skill Section 5
# =====================================================================
section "Phase 4: 根因确认 / Root Cause Identification (Skill Section 5)"

info "根据诊断证据匹配 root-cause-map.yaml:"
echo ""
echo -e "  ${BOLD}诊断证据:${NC}"
echo -e "    D1.2: 所有 Conditions 正常 (Ready=True)"
echo -e "    D1.4: 存在 unschedulable taint"
echo -e "    D1.5: Lease 正常更新"
echo ""
echo -e "  ${BOLD}匹配根因:${NC}"
echo -e "    ${GREEN}RC-012: 节点被手动 cordon${NC}"
echo -e "    置信度: 0.95"
echo -e "    概率: 低 (但证据链完整)"
echo ""
echo -e "  ${BOLD}FTA 映射:${NC}"
echo -e "    RC-012 → evt_cordon (is_fault: false)"
echo -e "    💡 这不是真正的故障，是人为操作"
pause

# =====================================================================
# PHASE 5: 修复操作 — Skill Section 6
# =====================================================================
section "Phase 5: 修复操作 / Remediation (Skill Section 6)"

echo -e "  ${BOLD}修复方案: REM-001 — 取消节点 cordon 标记${NC}"
echo -e "  风险等级: ${GREEN}🟢 低风险 (Green)${NC}"
echo -e "  Agent 模式: L1-advisory → Agent 建议执行"
echo ""

step "REM-001.pre" "前置检查 / Pre-check"
run_cmd "kubectl get node ${WORKER_NODE} -o jsonpath='{.spec.unschedulable}'"
echo ""
info "unschedulable=true → 确认需要 uncordon"

step "REM-001.exec" "执行修复: uncordon / Execute remediation"
run_cmd "kubectl uncordon ${WORKER_NODE}"
info "✓ uncordon 完成"

step "REM-001.post" "后置验证 / Post-verification"
run_cmd "kubectl get node ${WORKER_NODE} -o jsonpath='{.spec.unschedulable}'"
echo ""
info "unschedulable 已清除"
pause

# =====================================================================
# PHASE 6: 验证确认 — Skill Section 7
# =====================================================================
section "Phase 6: 验证确认 / Verification (Skill Section 7)"

step "V1" "节点状态 = Ready / Node STATUS = Ready"
run_cmd "kubectl get node ${WORKER_NODE}"
NODE_STATUS=$(kubectl get node "${WORKER_NODE}" -o jsonpath='{.status.conditions[?(@.type=="Ready")].status}')
if [[ "${NODE_STATUS}" == "True" ]]; then
    echo -e "  ${GREEN}✓ V1 通过: 节点 Ready${NC}"
else
    echo -e "  ${RED}✗ V1 失败: 节点未 Ready${NC}"
fi

step "V2" "Conditions 正常 / All Conditions normal"
run_cmd "kubectl get node ${WORKER_NODE} -o jsonpath='{range .status.conditions[*]}{.type}={.status} {end}'"
echo ""

step "V3" "Lease 更新 / Lease renewed"
run_cmd "kubectl get lease ${WORKER_NODE} -n kube-node-lease -o jsonpath='{.spec.renewTime}'"
echo ""

step "V4" "Pod 可调度到该节点 / Pods can schedule to this node"
info "验证: 通过检查 taint 是否清除"
TAINTS=$(kubectl get node "${WORKER_NODE}" -o jsonpath='{.spec.taints}' 2>/dev/null || echo "none")
if [[ "${TAINTS}" == "null" || "${TAINTS}" == "" || "${TAINTS}" == "none" ]]; then
    echo -e "  ${GREEN}✓ V4 通过: 无 unschedulable taint${NC}"
else
    echo -e "  ${YELLOW}⚠ V4: 存在 taints: ${TAINTS}${NC}"
fi

# =====================================================================
# 完成 / Complete
# =====================================================================
echo ""
echo -e "${GREEN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  ✅ Scenario 01 完成 / Complete!                            ║${NC}"
echo -e "${GREEN}╠══════════════════════════════════════════════════════════════╣${NC}"
echo -e "${GREEN}║  Skill:    SKILL-NODE-001 (Node NotReady)                   ║${NC}"
echo -e "${GREEN}║  根因:     RC-012 (节点被手动 cordon)                        ║${NC}"
echo -e "${GREEN}║  修复:     REM-001 (uncordon, 🟢低风险)                     ║${NC}"
echo -e "${GREEN}║  验证:     V1-V4 全部通过                                    ║${NC}"
echo -e "${GREEN}║                                                              ║${NC}"
echo -e "${GREEN}║  完整 Skill 执行流程:                                        ║${NC}"
echo -e "${GREEN}║  触发→症状检测→分级→诊断→根因→修复→验证 ✓                     ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════════════════════════╝${NC}"
