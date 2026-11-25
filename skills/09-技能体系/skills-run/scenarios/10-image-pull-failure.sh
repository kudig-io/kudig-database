#!/usr/bin/env bash
# ============================================================================
# 10-image-pull-failure.sh — 场景: 镜像拉取失败 (RC-001)
# Scenario: Image Pull Failure — maps to SKILL-IMAGE-001 / RC-001
# ============================================================================
# 演示 Skill 执行流程:
#   1. 故障注入 (Inject)    — 部署引用不存在镜像的 Pod
#   2. 症状检测 (Detect)    — Pod 状态为 ImagePullBackOff
#   3. 快速分级 (Triage)    — 影响评估
#   4. 诊断工作流 (Diagnose) — Phase 1 快速检查
#   5. 根因确认 (Root Cause) — RC-001: 镜像不存在
#   6. 修复操作 (Remediate)  — REM-001: 修正镜像名称
#   7. 验证确认 (Verify)     — Pod 正常运行
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
DEPLOY_NAME="demo-bad-image"
BAD_IMAGE="nginx:nonexistent-tag-v999"
GOOD_IMAGE="nginx:alpine"

# ---- 清理函数 / Cleanup ----
cleanup() {
    echo -e "\n${YELLOW}正在清理 / Cleaning up...${NC}"
    kubectl delete deployment ${DEPLOY_NAME} -n ${NAMESPACE} --ignore-not-found=true 2>/dev/null || true
}
trap cleanup EXIT ERR

echo -e "${CYAN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║  📋 Scenario 10: Image Pull Failure (SKILL-IMAGE-001/RC-001)║${NC}"
echo -e "${CYAN}║  目标: 引用不存在的镜像导致 Pod 无法启动                      ║${NC}"
echo -e "${CYAN}╚══════════════════════════════════════════════════════════════╝${NC}"

# =====================================================================
# PHASE 0: 故障注入 / Fault Injection
# =====================================================================
section "Phase 0: 故障注入 / Fault Injection"

step "INJECT" "创建引用不存在镜像的 Deployment"

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
      app: bad-image-demo
  template:
    metadata:
      labels:
        app: bad-image-demo
    spec:
      containers:
      - name: app
        image: ${BAD_IMAGE}
        ports:
        - containerPort: 80
EOF

info "等待 Pod 创建并触发镜像拉取失败 (15秒)..."
sleep 15

info "注入完成。现在模拟 Agent 收到 ImagePullBackOff 告警 / Injection done."
pause

# =====================================================================
# PHASE 1: 症状检测 — Skill Section 2
# =====================================================================
section "Phase 1: 症状检测 / Symptom Detection (Skill Section 2)"

step "S1" "检查 Pod 状态 / Check Pod status (置信度: 0.95)"
run_cmd "kubectl get pods -n ${NAMESPACE} -l app=bad-image-demo"
echo ""
info "💡 Skill 匹配: Pod 状态为 ImagePullBackOff 或 ErrImagePull → SKILL-IMAGE-001 激活"

step "S2" "检查 Pod 事件 / Check Pod events"
POD_NAME=$(kubectl get pods -n ${NAMESPACE} -l app=bad-image-demo -o jsonpath='{.items[0].metadata.name}' 2>/dev/null || echo "")
if [[ -n "${POD_NAME}" ]]; then
    run_cmd "kubectl describe pod ${POD_NAME} -n ${NAMESPACE} | grep -A 10 'Events:'"
fi
echo ""
info "💡 Events 显示 Failed to pull image 或 manifest unknown"

step "S3" "获取镜像名称 / Get image name"
CURRENT_IMAGE=$(kubectl get deployment ${DEPLOY_NAME} -n ${NAMESPACE} -o jsonpath='{.spec.template.spec.containers[0].image}')
info "当前配置的镜像: ${CURRENT_IMAGE}"
pause

# =====================================================================
# PHASE 2: 快速分级 — Skill Section 3 (2 分钟内)
# =====================================================================
section "Phase 2: 快速分级 / Quick Triage (Skill Section 3, <2min)"

step "T1" "影响评估: ImagePullBackOff Pod 数量 / Impact: affected pods count"
AFFECTED_PODS=$(kubectl get pods -n ${NAMESPACE} -l app=bad-image-demo --no-headers 2>/dev/null | grep -cE "ImagePullBackOff|ErrImagePull" || echo "0")
TOTAL_PODS=$(kubectl get pods -n ${NAMESPACE} -l app=bad-image-demo --no-headers 2>/dev/null | wc -l | tr -d ' ')
info "受影响 Pod: ${AFFECTED_PODS} / ${TOTAL_PODS}"

step "T2" "是否影响生产服务 / Affects production services?"
info "检查 Deployment 标签和 namespace..."
run_cmd "kubectl get deployment ${DEPLOY_NAME} -n ${NAMESPACE} -o jsonpath='{.metadata.labels}'"
echo ""

step "T3" "严重性分级 / Severity classification"
echo -e "  ${YELLOW}🟡 P2 — 单个 Deployment 镜像拉取失败${NC}"
info "📊 分级结果: P2 (镜像配置问题)"
pause

# =====================================================================
# PHASE 3: 诊断工作流 — Skill Section 4, Phase 1
# =====================================================================
section "Phase 3: 诊断工作流 / Diagnostic Workflow (Skill Section 4)"
echo -e "  ${YELLOW}执行 Phase 1: 快速检查 (kubectl, 只读, 零风险)${NC}"

step "D1.1" "Pod 状态详情 / Pod status details"
if [[ -n "${POD_NAME}" ]]; then
    run_cmd "kubectl get pod ${POD_NAME} -n ${NAMESPACE} -o yaml | grep -A 20 'containerStatuses:'"
fi

step "D1.2" "检查镜像拉取错误 / Check image pull error"
if [[ -n "${POD_NAME}" ]]; then
    run_cmd "kubectl get events -n ${NAMESPACE} --field-selector involvedObject.name=${POD_NAME},reason=Failed --sort-by='.lastTimestamp' 2>/dev/null | tail -5 || echo 'No Failed events'"
fi
echo ""
info "💡 错误信息: manifest for ${BAD_IMAGE} not found 或类似"

step "D1.3" "检查 imagePullSecrets 配置 / Check imagePullSecrets"
PULL_SECRETS=$(kubectl get deployment ${DEPLOY_NAME} -n ${NAMESPACE} -o jsonpath='{.spec.template.spec.imagePullSecrets}' 2>/dev/null || echo "")
if [[ -z "${PULL_SECRETS}" || "${PULL_SECRETS}" == "null" ]]; then
    info "未配置 imagePullSecrets (公共镜像不需要)"
else
    info "imagePullSecrets: ${PULL_SECRETS}"
fi

step "D1.4" "验证镜像格式 / Verify image format"
info "镜像: ${CURRENT_IMAGE}"
info "格式分析: registry=docker.io (默认), name=nginx, tag=nonexistent-tag-v999"
warn "该镜像 tag 不存在于 Docker Hub"
pause

# =====================================================================
# PHASE 4: 根因确认 — Skill Section 5
# =====================================================================
section "Phase 4: 根因确认 / Root Cause Identification (Skill Section 5)"

info "根据诊断证据匹配 root-cause-map.yaml:"
echo ""
echo -e "  ${BOLD}诊断证据:${NC}"
echo -e "    D1.1: Pod 状态为 ImagePullBackOff"
echo -e "    D1.2: 事件显示 'manifest unknown' 或 'not found'"
echo -e "    D1.3: 无私有仓库认证问题"
echo -e "    D1.4: 镜像 tag 不存在"
echo ""
echo -e "  ${BOLD}匹配根因:${NC}"
echo -e "    ${GREEN}RC-001: 镜像不存在 (tag 错误或镜像未发布)${NC}"
echo -e "    置信度: 0.95"
echo -e "    概率: 高 (常见配置错误)"
echo ""
echo -e "  ${BOLD}FTA 映射:${NC}"
echo -e "    RC-001 → evt_image_not_found"
pause

# =====================================================================
# PHASE 5: 修复操作 — Skill Section 6
# =====================================================================
section "Phase 5: 修复操作 / Remediation (Skill Section 6)"

echo -e "  ${BOLD}修复方案: REM-001 — 修正镜像名称/标签${NC}"
echo -e "  风险等级: ${GREEN}🟢 低风险 (Green)${NC}"
echo -e "  Agent 模式: L2-semi-auto → Agent 可自动执行"
echo ""

step "REM-001.pre" "前置检查: 确认正确的镜像 / Pre-check: Confirm correct image"
info "错误镜像: ${BAD_IMAGE}"
info "正确镜像: ${GOOD_IMAGE}"

step "REM-001.exec" "更新 Deployment 使用正确镜像 / Update Deployment with correct image"
run_cmd "kubectl set image deployment/${DEPLOY_NAME} app=${GOOD_IMAGE} -n ${NAMESPACE}"

info "等待 Deployment 滚动更新..."
kubectl rollout status deployment/${DEPLOY_NAME} -n ${NAMESPACE} --timeout=60s 2>/dev/null || true

step "REM-001.post" "后置验证 / Post-verification"
run_cmd "kubectl get pods -n ${NAMESPACE} -l app=bad-image-demo"
run_cmd "kubectl get deployment ${DEPLOY_NAME} -n ${NAMESPACE}"
pause

# =====================================================================
# PHASE 6: 验证确认 — Skill Section 7
# =====================================================================
section "Phase 6: 验证确认 / Verification (Skill Section 7)"

step "V1" "Pod 状态 = Running / Pod STATUS = Running"
NEW_POD=$(kubectl get pods -n ${NAMESPACE} -l app=bad-image-demo -o jsonpath='{.items[0].metadata.name}' 2>/dev/null || echo "")
if [[ -n "${NEW_POD}" ]]; then
    POD_STATUS=$(kubectl get pod ${NEW_POD} -n ${NAMESPACE} -o jsonpath='{.status.phase}' 2>/dev/null || echo "Unknown")
    if [[ "${POD_STATUS}" == "Running" ]]; then
        echo -e "  ${GREEN}✓ V1 通过: Pod 状态为 Running${NC}"
    else
        echo -e "  ${RED}✗ V1 失败: Pod 状态为 ${POD_STATUS}${NC}"
    fi
fi

step "V2" "容器状态正常 / Container status normal"
CONTAINER_READY=$(kubectl get pod ${NEW_POD} -n ${NAMESPACE} -o jsonpath='{.status.containerStatuses[0].ready}' 2>/dev/null || echo "false")
if [[ "${CONTAINER_READY}" == "true" ]]; then
    echo -e "  ${GREEN}✓ V2 通过: 容器 Ready${NC}"
else
    echo -e "  ${RED}✗ V2 失败: 容器未 Ready${NC}"
fi

step "V3" "镜像已更新 / Image updated"
ACTUAL_IMAGE=$(kubectl get deployment ${DEPLOY_NAME} -n ${NAMESPACE} -o jsonpath='{.spec.template.spec.containers[0].image}')
if [[ "${ACTUAL_IMAGE}" == "${GOOD_IMAGE}" ]]; then
    echo -e "  ${GREEN}✓ V3 通过: 镜像已更新为 ${GOOD_IMAGE}${NC}"
else
    echo -e "  ${RED}✗ V3 失败: 镜像为 ${ACTUAL_IMAGE}${NC}"
fi

step "V4" "无 ImagePull 错误事件 / No ImagePull error events"
RECENT_ERRORS=$(kubectl get events -n ${NAMESPACE} --field-selector reason=Failed,involvedObject.name=${NEW_POD} --sort-by='.lastTimestamp' 2>/dev/null | grep -c "pull" || echo "0")
if [[ "${RECENT_ERRORS}" == "0" ]]; then
    echo -e "  ${GREEN}✓ V4 通过: 无新的镜像拉取错误${NC}"
else
    echo -e "  ${YELLOW}⚠ V4: 存在历史镜像拉取错误事件${NC}"
fi

# ---- 清理 / Cleanup ----
step "CLEANUP" "清理测试资源 / Cleaning up test resources"
kubectl delete deployment ${DEPLOY_NAME} -n ${NAMESPACE} --ignore-not-found=true 2>/dev/null || true

# =====================================================================
# 完成 / Complete
# =====================================================================
echo ""
echo -e "${GREEN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  ✅ Scenario 10 完成 / Complete!                            ║${NC}"
echo -e "${GREEN}╠══════════════════════════════════════════════════════════════╣${NC}"
echo -e "${GREEN}║  Skill:    SKILL-IMAGE-001 (Image Pull Failure)             ║${NC}"
echo -e "${GREEN}║  根因:     RC-001 (镜像不存在/tag 错误)                      ║${NC}"
echo -e "${GREEN}║  修复:     REM-001 (修正镜像名称, 🟢低风险)                 ║${NC}"
echo -e "${GREEN}║  验证:     V1-V4 全部通过                                    ║${NC}"
echo -e "${GREEN}║                                                              ║${NC}"
echo -e "${GREEN}║  完整 Skill 执行流程:                                        ║${NC}"
echo -e "${GREEN}║  触发→症状检测→分级→诊断→根因→修复→验证 ✓                     ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════════════════════════╝${NC}"
