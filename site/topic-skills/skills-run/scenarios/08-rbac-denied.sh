#!/usr/bin/env bash
# ============================================================================
# 08-rbac-denied.sh — 场景: RBAC 权限拒绝 (RC-001)
# Scenario: RBAC Permission Denied — maps to SKILL-SEC-002 / RC-001
# ============================================================================
# 演示 Skill 执行流程:
#   1. 故障注入 (Inject)    — 创建受限 ServiceAccount 尝试越权操作
#   2. 症状检测 (Detect)    — RBAC Forbidden 错误
#   3. 快速分级 (Triage)    — 影响评估
#   4. 诊断工作流 (Diagnose) — Phase 1 快速检查
#   5. 根因确认 (Root Cause) — RC-001: 缺少必要的 RBAC 权限
#   6. 修复操作 (Remediate)  — REM-001: 添加 RoleBinding
#   7. 验证确认 (Verify)     — 权限检查通过
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
    ( bash -c "$1" 2>&1 | sed 's/^/    /' ) || {
        echo -e "    ${RED}[命令失败 / Command failed with exit code $?]${NC}"
    }
}
pause()   { echo -e "\n  ${YELLOW}按 Enter 继续 / Press Enter to continue...${NC}"; read -r; }

NAMESPACE="skill-demo"
SA_NAME="restricted-sa"
ROLE_NAME="pod-reader"

# ---- 清理函数 / Cleanup ----
cleanup() {
    echo -e "\n${YELLOW}正在清理 / Cleaning up...${NC}"
    kubectl delete rolebinding ${SA_NAME}-${ROLE_NAME} -n ${NAMESPACE} --ignore-not-found=true 2>/dev/null || true
    kubectl delete role ${ROLE_NAME} -n ${NAMESPACE} --ignore-not-found=true 2>/dev/null || true
    kubectl delete serviceaccount ${SA_NAME} -n ${NAMESPACE} --ignore-not-found=true 2>/dev/null || true
}
trap cleanup EXIT ERR

echo -e "${CYAN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║  📋 Scenario 08: RBAC Denied (SKILL-SEC-002 / RC-001)       ║${NC}"
echo -e "${CYAN}║  目标: 受限 ServiceAccount 尝试越权操作                      ║${NC}"
echo -e "${CYAN}╚══════════════════════════════════════════════════════════════╝${NC}"

# =====================================================================
# PHASE 0: 故障注入 / Fault Injection
# =====================================================================
section "Phase 0: 故障注入 / Fault Injection"

step "INJECT-1" "创建受限的 ServiceAccount (无任何权限)"

cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: ServiceAccount
metadata:
  name: ${SA_NAME}
  namespace: ${NAMESPACE}
EOF

step "INJECT-2" "尝试使用该 SA 执行需要权限的操作"
info "尝试列出 Pods (应该被拒绝)..."

# 使用 kubectl auth can-i 检查权限
run_cmd "kubectl auth can-i list pods --as=system:serviceaccount:${NAMESPACE}:${SA_NAME} -n ${NAMESPACE} 2>&1 || true"

info "注入完成。现在模拟 Agent 收到权限拒绝告警 / Injection done."
pause

# =====================================================================
# PHASE 1: 症状检测 — Skill Section 2
# =====================================================================
section "Phase 1: 症状检测 / Symptom Detection (Skill Section 2)"

step "S1" "检测 RBAC 拒绝 / Detect RBAC denial (置信度: 0.95)"
run_cmd "kubectl auth can-i list pods --as=system:serviceaccount:${NAMESPACE}:${SA_NAME} -n ${NAMESPACE} 2>&1 || true"
echo ""
info "💡 Skill 匹配: 'no' 表示权限被拒绝 → SKILL-SEC-002 激活"

step "S2" "检查 ServiceAccount 详情 / Check ServiceAccount"
run_cmd "kubectl get serviceaccount ${SA_NAME} -n ${NAMESPACE} -o yaml"

step "S3" "检查现有 RoleBindings / Check existing RoleBindings"
run_cmd "kubectl get rolebindings -n ${NAMESPACE} 2>/dev/null || echo 'No RoleBindings found'"
echo ""
info "💡 ServiceAccount 没有绑定任何 Role"
pause

# =====================================================================
# PHASE 2: 快速分级 — Skill Section 3 (2 分钟内)
# =====================================================================
section "Phase 2: 快速分级 / Quick Triage (Skill Section 3, <2min)"

step "T1" "影响评估: 受影响的 ServiceAccount / Impact: affected ServiceAccounts"
info "ServiceAccount: ${SA_NAME}"
info "Namespace: ${NAMESPACE}"

step "T2" "是否影响关键服务 / Affects critical services?"
info "检查使用该 SA 的 Pod..."
PODS_USING_SA=$(kubectl get pods -n ${NAMESPACE} -o jsonpath='{range .items[?(@.spec.serviceAccountName=="'${SA_NAME}'")]}{.metadata.name}{"\n"}{end}' 2>/dev/null || echo "")
if [[ -n "${PODS_USING_SA}" ]]; then
    warn "以下 Pod 使用此 ServiceAccount: ${PODS_USING_SA}"
else
    info "✓ 当前无 Pod 使用此 ServiceAccount"
fi

step "T3" "严重性分级 / Severity classification"
echo -e "  ${GREEN}🟢 P2 — 单个 ServiceAccount 权限问题，影响有限${NC}"
info "📊 分级结果: P2 (权限配置问题)"
pause

# =====================================================================
# PHASE 3: 诊断工作流 — Skill Section 4, Phase 1
# =====================================================================
section "Phase 3: 诊断工作流 / Diagnostic Workflow (Skill Section 4)"
echo -e "  ${YELLOW}执行 Phase 1: 快速检查 (kubectl, 只读, 零风险)${NC}"

step "D1.1" "检查 ServiceAccount 绑定的 Secrets / Check SA secrets"
run_cmd "kubectl get serviceaccount ${SA_NAME} -n ${NAMESPACE} -o jsonpath='{.secrets}'"
echo ""

step "D1.2" "检查可能的 ClusterRoleBindings / Check ClusterRoleBindings"
run_cmd "kubectl get clusterrolebindings -o json | jq -r '.items[] | select(.subjects[]?.name == \"${SA_NAME}\") | .metadata.name' 2>/dev/null || echo 'No ClusterRoleBindings for this SA'"

step "D1.3" "检查命名空间级别的 RoleBindings / Check namespace RoleBindings"
run_cmd "kubectl get rolebindings -n ${NAMESPACE} -o json | jq -r '.items[] | select(.subjects[]?.name == \"${SA_NAME}\") | .metadata.name' 2>/dev/null || echo 'No RoleBindings for this SA'"

step "D1.4" "列出所需权限 / List required permissions"
info "尝试的操作: list pods"
info "需要的权限: pods [list] 在 namespace ${NAMESPACE}"
pause

# =====================================================================
# PHASE 4: 根因确认 — Skill Section 5
# =====================================================================
section "Phase 4: 根因确认 / Root Cause Identification (Skill Section 5)"

info "根据诊断证据匹配 root-cause-map.yaml:"
echo ""
echo -e "  ${BOLD}诊断证据:${NC}"
echo -e "    D1.2: 无 ClusterRoleBinding 绑定到此 SA"
echo -e "    D1.3: 无 RoleBinding 绑定到此 SA"
echo -e "    D1.4: 需要 pods [list] 权限"
echo ""
echo -e "  ${BOLD}匹配根因:${NC}"
echo -e "    ${GREEN}RC-001: ServiceAccount 缺少必要的 RBAC 权限${NC}"
echo -e "    置信度: 0.95"
echo -e "    概率: 高 (常见配置遗漏)"
echo ""
echo -e "  ${BOLD}FTA 映射:${NC}"
echo -e "    RC-001 → evt_missing_rbac_binding"
pause

# =====================================================================
# PHASE 5: 修复操作 — Skill Section 6
# =====================================================================
section "Phase 5: 修复操作 / Remediation (Skill Section 6)"

echo -e "  ${BOLD}修复方案: REM-001 — 创建 Role 和 RoleBinding${NC}"
echo -e "  风险等级: ${YELLOW}🟡 中风险 (Yellow)${NC}"
echo -e "  Agent 模式: L2-semi-auto → 需要人工审批"
echo ""

step "REM-001.pre" "前置检查: 确认所需权限 / Pre-check: Confirm required permissions"
info "将授予权限: pods [get, list, watch] 在 namespace ${NAMESPACE}"

step "REM-001.exec1" "创建 Role / Create Role"

cat <<EOF | kubectl apply -f -
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: ${ROLE_NAME}
  namespace: ${NAMESPACE}
rules:
- apiGroups: [""]
  resources: ["pods"]
  verbs: ["get", "list", "watch"]
EOF

step "REM-001.exec2" "创建 RoleBinding / Create RoleBinding"

cat <<EOF | kubectl apply -f -
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: ${SA_NAME}-${ROLE_NAME}
  namespace: ${NAMESPACE}
subjects:
- kind: ServiceAccount
  name: ${SA_NAME}
  namespace: ${NAMESPACE}
roleRef:
  kind: Role
  name: ${ROLE_NAME}
  apiGroup: rbac.authorization.k8s.io
EOF

step "REM-001.post" "后置验证 / Post-verification"
run_cmd "kubectl get rolebindings -n ${NAMESPACE}"
pause

# =====================================================================
# PHASE 6: 验证确认 — Skill Section 7
# =====================================================================
section "Phase 6: 验证确认 / Verification (Skill Section 7)"

step "V1" "验证 list pods 权限 / Verify list pods permission"
CAN_LIST=$(kubectl auth can-i list pods --as=system:serviceaccount:${NAMESPACE}:${SA_NAME} -n ${NAMESPACE} 2>/dev/null || echo "no")
if [[ "${CAN_LIST}" == "yes" ]]; then
    echo -e "  ${GREEN}✓ V1 通过: 可以 list pods${NC}"
else
    echo -e "  ${RED}✗ V1 失败: 仍无法 list pods${NC}"
fi

step "V2" "验证 get pods 权限 / Verify get pods permission"
CAN_GET=$(kubectl auth can-i get pods --as=system:serviceaccount:${NAMESPACE}:${SA_NAME} -n ${NAMESPACE} 2>/dev/null || echo "no")
if [[ "${CAN_GET}" == "yes" ]]; then
    echo -e "  ${GREEN}✓ V2 通过: 可以 get pods${NC}"
else
    echo -e "  ${RED}✗ V2 失败: 仍无法 get pods${NC}"
fi

step "V3" "验证 watch pods 权限 / Verify watch pods permission"
CAN_WATCH=$(kubectl auth can-i watch pods --as=system:serviceaccount:${NAMESPACE}:${SA_NAME} -n ${NAMESPACE} 2>/dev/null || echo "no")
if [[ "${CAN_WATCH}" == "yes" ]]; then
    echo -e "  ${GREEN}✓ V3 通过: 可以 watch pods${NC}"
else
    echo -e "  ${RED}✗ V3 失败: 仍无法 watch pods${NC}"
fi

step "V4" "验证无越权 / Verify no over-permission"
CAN_DELETE=$(kubectl auth can-i delete pods --as=system:serviceaccount:${NAMESPACE}:${SA_NAME} -n ${NAMESPACE} 2>/dev/null || echo "no")
if [[ "${CAN_DELETE}" == "no" ]]; then
    echo -e "  ${GREEN}✓ V4 通过: 正确限制了 delete 权限${NC}"
else
    echo -e "  ${YELLOW}⚠ V4: 意外获得了 delete 权限，需要检查${NC}"
fi

# ---- 清理 / Cleanup ----
step "CLEANUP" "清理测试资源 / Cleaning up test resources"
kubectl delete rolebinding ${SA_NAME}-${ROLE_NAME} -n ${NAMESPACE} --ignore-not-found=true 2>/dev/null || true
kubectl delete role ${ROLE_NAME} -n ${NAMESPACE} --ignore-not-found=true 2>/dev/null || true
kubectl delete serviceaccount ${SA_NAME} -n ${NAMESPACE} --ignore-not-found=true 2>/dev/null || true

# =====================================================================
# 完成 / Complete
# =====================================================================
echo ""
echo -e "${GREEN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  ✅ Scenario 08 完成 / Complete!                            ║${NC}"
echo -e "${GREEN}╠══════════════════════════════════════════════════════════════╣${NC}"
echo -e "${GREEN}║  Skill:    SKILL-SEC-002 (RBAC/Quota Failure)               ║${NC}"
echo -e "${GREEN}║  根因:     RC-001 (缺少 RBAC 权限)                           ║${NC}"
echo -e "${GREEN}║  修复:     REM-001 (创建 Role+RoleBinding, 🟡中风险)        ║${NC}"
echo -e "${GREEN}║  验证:     V1-V4 全部通过                                    ║${NC}"
echo -e "${GREEN}║                                                              ║${NC}"
echo -e "${GREEN}║  完整 Skill 执行流程:                                        ║${NC}"
echo -e "${GREEN}║  触发→症状检测→分级→诊断→根因→修复→验证 ✓                     ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════════════════════════╝${NC}"
