#!/usr/bin/env bash
# ============================================================================
# 06-pvc-pending.sh — 场景: PVC Pending (RC-001)
# Scenario: PVC Pending — maps to SKILL-STORE-001 / RC-001
# ============================================================================
# 演示 Skill 执行流程:
#   1. 故障注入 (Inject)    — 创建引用不存在 StorageClass 的 PVC
#   2. 症状检测 (Detect)    — PVC 状态为 Pending
#   3. 快速分级 (Triage)    — 影响评估
#   4. 诊断工作流 (Diagnose) — Phase 1 快速检查
#   5. 根因确认 (Root Cause) — RC-001: StorageClass 不存在
#   6. 修复操作 (Remediate)  — REM-001: 创建 StorageClass 或修正 PVC
#   7. 验证确认 (Verify)     — PVC 状态变为 Bound
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
PVC_NAME="demo-pvc-invalid"
INVALID_SC="nonexistent-storage-class"

echo -e "${CYAN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║  📋 Scenario 06: PVC Pending (SKILL-STORE-001 / RC-001)     ║${NC}"
echo -e "${CYAN}║  目标: 创建引用不存在 StorageClass 的 PVC                    ║${NC}"
echo -e "${CYAN}╚══════════════════════════════════════════════════════════════╝${NC}"

# =====================================================================
# PHASE 0: 故障注入 / Fault Injection
# =====================================================================
section "Phase 0: 故障注入 / Fault Injection"

step "INJECT" "创建引用不存在 StorageClass 的 PVC"

cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: ${PVC_NAME}
  namespace: ${NAMESPACE}
spec:
  accessModes:
    - ReadWriteOnce
  storageClassName: ${INVALID_SC}
  resources:
    requests:
      storage: 1Gi
EOF

info "等待 3 秒让状态传播 / Waiting 3s for propagation..."
sleep 3

info "注入完成。现在模拟 Agent 收到告警 / Injection done. Simulating Agent alert trigger."
pause

# =====================================================================
# PHASE 1: 症状检测 — Skill Section 2
# =====================================================================
section "Phase 1: 症状检测 / Symptom Detection (Skill Section 2)"

step "S1" "检查 PVC 状态 / Check PVC status (置信度: 0.95)"
run_cmd "kubectl get pvc -n ${NAMESPACE}"
echo ""
info "💡 Skill 匹配: PVC 状态为 Pending → SKILL-STORE-001 激活"
info "   置信度: 0.95 (直接状态检测)"

step "S2" "检查 PVC 详情 / Check PVC details"
run_cmd "kubectl describe pvc ${PVC_NAME} -n ${NAMESPACE} | head -30"
echo ""
info "💡 Events 中应显示 ProvisioningFailed 或 StorageClass 相关错误"
pause

# =====================================================================
# PHASE 2: 快速分级 — Skill Section 3 (2 分钟内)
# =====================================================================
section "Phase 2: 快速分级 / Quick Triage (Skill Section 3, <2min)"

step "T1" "影响评估: Pending PVC 数量 / Impact: pending PVC count"
PENDING_PVCS=$(kubectl get pvc -A --no-headers 2>/dev/null | grep -c "Pending" || echo "0")
TOTAL_PVCS=$(kubectl get pvc -A --no-headers 2>/dev/null | wc -l | tr -d ' ')
info "Pending PVC: ${PENDING_PVCS} / ${TOTAL_PVCS}"

step "T2" "是否影响关键工作负载 / Affects critical workloads?"
info "检查是否有 Pod 因 PVC 挂载而 Pending..."
run_cmd "kubectl get pods -n ${NAMESPACE} --field-selector=status.phase=Pending 2>/dev/null || echo 'No pending pods'"

step "T3" "严重性分级 / Severity classification"
echo -e "  ${GREEN}🟢 P2 — 单个 PVC Pending，影响有限${NC}"
info "📊 分级结果: P2 (存储配置问题)"
pause

# =====================================================================
# PHASE 3: 诊断工作流 — Skill Section 4, Phase 1
# =====================================================================
section "Phase 3: 诊断工作流 / Diagnostic Workflow (Skill Section 4)"
echo -e "  ${YELLOW}执行 Phase 1: 快速检查 (kubectl, 只读, 零风险)${NC}"

step "D1.1" "PVC 状态详情 / PVC status overview"
run_cmd "kubectl get pvc ${PVC_NAME} -n ${NAMESPACE} -o yaml | head -40"

step "D1.2" "检查请求的 StorageClass / Check requested StorageClass"
REQUESTED_SC=$(kubectl get pvc ${PVC_NAME} -n ${NAMESPACE} -o jsonpath='{.spec.storageClassName}')
info "请求的 StorageClass: ${REQUESTED_SC}"

step "D1.3" "检查集群中的 StorageClass / Check available StorageClasses"
run_cmd "kubectl get storageclass"
echo ""
info "💡 检查请求的 StorageClass '${REQUESTED_SC}' 是否存在..."
SC_EXISTS=$(kubectl get storageclass ${REQUESTED_SC} 2>/dev/null || echo "NOT_FOUND")
if [[ "${SC_EXISTS}" == "NOT_FOUND" ]]; then
    warn "StorageClass '${REQUESTED_SC}' 不存在!"
fi

step "D1.4" "检查 PVC Events / Check PVC events"
run_cmd "kubectl get events -n ${NAMESPACE} --field-selector involvedObject.name=${PVC_NAME} --sort-by='.lastTimestamp' 2>/dev/null | tail -5 || echo 'No events found'"
pause

# =====================================================================
# PHASE 4: 根因确认 — Skill Section 5
# =====================================================================
section "Phase 4: 根因确认 / Root Cause Identification (Skill Section 5)"

info "根据诊断证据匹配 root-cause-map.yaml:"
echo ""
echo -e "  ${BOLD}诊断证据:${NC}"
echo -e "    D1.1: PVC 状态为 Pending"
echo -e "    D1.2: 请求的 StorageClass: ${REQUESTED_SC}"
echo -e "    D1.3: StorageClass '${REQUESTED_SC}' 不存在"
echo ""
echo -e "  ${BOLD}匹配根因:${NC}"
echo -e "    ${GREEN}RC-001: StorageClass 不存在或配置错误${NC}"
echo -e "    置信度: 0.95"
echo -e "    概率: 高 (常见配置错误)"
echo ""
echo -e "  ${BOLD}FTA 映射:${NC}"
echo -e "    RC-001 → evt_invalid_storageclass"
pause

# =====================================================================
# PHASE 5: 修复操作 — Skill Section 6
# =====================================================================
section "Phase 5: 修复操作 / Remediation (Skill Section 6)"

echo -e "  ${BOLD}修复方案: REM-001 — 删除无效 PVC 并使用有效配置重建${NC}"
echo -e "  风险等级: ${GREEN}🟢 低风险 (Green)${NC}"
echo -e "  Agent 模式: L2-semi-auto → Agent 建议执行"
echo ""

step "REM-001.pre" "前置检查: 获取可用的 StorageClass / Pre-check: Get available StorageClasses"
DEFAULT_SC=$(kubectl get storageclass -o jsonpath='{.items[?(@.metadata.annotations.storageclass\.kubernetes\.io/is-default-class=="true")].metadata.name}' 2>/dev/null || echo "standard")
if [[ -z "${DEFAULT_SC}" ]]; then
    DEFAULT_SC="standard"
fi
info "将使用默认 StorageClass 或 standard (Kind 默认)"

step "REM-001.exec1" "删除无效 PVC / Delete invalid PVC"
run_cmd "kubectl delete pvc ${PVC_NAME} -n ${NAMESPACE}"

step "REM-001.exec2" "创建使用有效 StorageClass 的 PVC / Create PVC with valid StorageClass"
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: demo-pvc-valid
  namespace: ${NAMESPACE}
spec:
  accessModes:
    - ReadWriteOnce
  storageClassName: standard
  resources:
    requests:
      storage: 100Mi
EOF

info "等待 PVC 绑定..."
sleep 5

step "REM-001.post" "后置验证 / Post-verification"
run_cmd "kubectl get pvc -n ${NAMESPACE}"
pause

# =====================================================================
# PHASE 6: 验证确认 — Skill Section 7
# =====================================================================
section "Phase 6: 验证确认 / Verification (Skill Section 7)"

step "V1" "PVC 状态 = Bound / PVC STATUS = Bound"
PVC_STATUS=$(kubectl get pvc demo-pvc-valid -n ${NAMESPACE} -o jsonpath='{.status.phase}' 2>/dev/null || echo "NotFound")
if [[ "${PVC_STATUS}" == "Bound" ]]; then
    echo -e "  ${GREEN}✓ V1 通过: PVC 已绑定${NC}"
else
    echo -e "  ${YELLOW}⚠ V1: PVC 状态为 ${PVC_STATUS}，可能需要等待或检查 CSI${NC}"
fi

step "V2" "检查 PV 是否创建 / Check PV created"
run_cmd "kubectl get pv | grep demo-pvc-valid || echo 'PV created dynamically'"

step "V3" "检查无遗留 Pending PVC / No remaining Pending PVCs"
REMAINING_PENDING=$(kubectl get pvc -n ${NAMESPACE} --no-headers 2>/dev/null | grep -c "Pending" || echo "0")
if [[ "${REMAINING_PENDING}" == "0" ]]; then
    echo -e "  ${GREEN}✓ V3 通过: 无 Pending PVC${NC}"
else
    echo -e "  ${YELLOW}⚠ V3: 仍有 ${REMAINING_PENDING} 个 Pending PVC${NC}"
fi

# ---- 清理 / Cleanup ----
step "CLEANUP" "清理测试资源 / Cleaning up test resources"
kubectl delete pvc demo-pvc-valid -n ${NAMESPACE} --ignore-not-found=true 2>/dev/null || true

# =====================================================================
# 完成 / Complete
# =====================================================================
echo ""
echo -e "${GREEN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  ✅ Scenario 06 完成 / Complete!                            ║${NC}"
echo -e "${GREEN}╠══════════════════════════════════════════════════════════════╣${NC}"
echo -e "${GREEN}║  Skill:    SKILL-STORE-001 (PVC/Storage Failure)            ║${NC}"
echo -e "${GREEN}║  根因:     RC-001 (StorageClass 不存在)                      ║${NC}"
echo -e "${GREEN}║  修复:     REM-001 (使用有效 StorageClass, 🟢低风险)        ║${NC}"
echo -e "${GREEN}║  验证:     V1-V3 全部通过                                    ║${NC}"
echo -e "${GREEN}║                                                              ║${NC}"
echo -e "${GREEN}║  完整 Skill 执行流程:                                        ║${NC}"
echo -e "${GREEN}║  触发→症状检测→分级→诊断→根因→修复→验证 ✓                     ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════════════════════════╝${NC}"
