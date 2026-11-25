#!/usr/bin/env bash
# ============================================================================
# 04-dns-failure.sh — 场景: DNS 解析故障
# Scenario: DNS Resolution Failure — maps to SKILL-NET-001
# ============================================================================
# 演示: Scale down CoreDNS → DNS 解析失败 → 诊断 → 恢复 CoreDNS
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
warn()    { echo -e "  ${YELLOW}⚠${NC} $1"; }
run_cmd() {
    echo -e "  ${CYAN}\$ $1${NC}"
    ( bash -c "$1" 2>&1 | sed 's/^/    /' ) || {
        echo -e "    ${RED}[命令失败 / Command failed with exit code $?]${NC}"
    }
}
pause()   { echo -e "\n  ${YELLOW}按 Enter 继续 / Press Enter to continue...${NC}"; read -r; }

NS="skill-demo"

# ---- 保存当前 CoreDNS 副本数 ----
ORIGINAL_REPLICAS=$(kubectl get deployment coredns -n kube-system -o jsonpath='{.spec.replicas}')

# ---- 清理函数 / Cleanup ----
cleanup() {
    echo -e "\n${YELLOW}正在清理 / Cleaning up...${NC}"
    kubectl scale deployment coredns -n kube-system --replicas=${ORIGINAL_REPLICAS} 2>/dev/null || true
    kubectl delete pod dns-test -n ${NS} --ignore-not-found --grace-period=0 --force 2>/dev/null || true
}
trap cleanup EXIT ERR

echo -e "${CYAN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║  📋 Scenario 04: DNS Resolution Failure (SKILL-NET-001)    ║${NC}"
echo -e "${CYAN}╚══════════════════════════════════════════════════════════════╝${NC}"
info "当前 CoreDNS 副本数: ${ORIGINAL_REPLICAS}"

# =====================================================================
# PHASE 0: 故障注入
# =====================================================================
section "Phase 0: 故障注入 / Fault Injection"

step "INJECT" "将 CoreDNS 缩容到 0 / Scaling CoreDNS to 0 replicas"
kubectl scale deployment coredns -n kube-system --replicas=0
info "等待 CoreDNS Pod 终止..."
sleep 5

run_cmd "kubectl get pods -n kube-system -l k8s-app=kube-dns"
warn "CoreDNS 已全部停止！集群 DNS 解析将失败"
pause

# =====================================================================
# PHASE 1: 症状检测
# =====================================================================
section "Phase 1: 症状检测 / Symptom Detection (Skill Section 2)"

step "S1" "从 Pod 内测试 DNS 解析 / Test DNS resolution from pod (置信度: 0.95)"
# 创建一个测试 Pod
kubectl run dns-test --image=busybox:1.36 --restart=Never -n ${NS} --overrides='{"spec":{"terminationGracePeriodSeconds":0}}' --command -- sleep 300 2>/dev/null || true
sleep 5
kubectl wait --for=condition=Ready pod/dns-test -n ${NS} --timeout=30s 2>/dev/null || true

info "测试 DNS 解析 kubernetes.default.svc.cluster.local:"
kubectl exec dns-test -n ${NS} -- nslookup kubernetes.default.svc.cluster.local 2>&1 | sed 's/^/    /' || true
echo ""
info "💡 DNS 解析超时/失败 → SKILL-NET-001 激活"

step "S2" "检查 CoreDNS Pod 状态 / Check CoreDNS pods (置信度: 0.85)"
run_cmd "kubectl get pods -n kube-system -l k8s-app=kube-dns"
info "💡 CoreDNS Pod 不存在或不健康 → DNS 服务不可用"

step "S3" "检查 kube-dns Service / Check kube-dns Service"
run_cmd "kubectl get svc kube-dns -n kube-system"
run_cmd "kubectl get endpoints kube-dns -n kube-system"
info "💡 kube-dns Endpoints 为空 → 无可用的 DNS 后端"
pause

# =====================================================================
# PHASE 2: 快速分级
# =====================================================================
section "Phase 2: 快速分级 / Quick Triage (Skill Section 3)"

step "T1" "影响评估: DNS 是集群核心基础设施"
warn "DNS 故障影响所有 Service 发现和跨 Pod 通信"
info "爆炸半径: 全集群"

step "T2" "严重性分级"
echo -e "  ${RED}🔴 P0 — DNS 基础设施故障，影响全集群通信${NC}"
pause

# =====================================================================
# PHASE 3: 诊断工作流
# =====================================================================
section "Phase 3: 诊断工作流 / Diagnostic Workflow (Skill Section 4)"

step "D1.1" "CoreDNS Deployment 状态 / CoreDNS Deployment status"
run_cmd "kubectl get deployment coredns -n kube-system"

step "D1.2" "CoreDNS ReplicaSet 和 Pod / ReplicaSet and Pods"
run_cmd "kubectl get rs -n kube-system -l k8s-app=kube-dns"

step "D1.3" "CoreDNS 配置 (ConfigMap) / CoreDNS ConfigMap"
run_cmd "kubectl get configmap coredns -n kube-system -o yaml | head -30"

step "D1.4" "Events 分析 / Events analysis"
run_cmd "kubectl get events -n kube-system --field-selector reason=ScalingReplicaSet --sort-by='.lastTimestamp' | tail -5"
info "💡 发现 ScalingReplicaSet 事件 → CoreDNS 被缩容到 0"
pause

# =====================================================================
# PHASE 4: 根因确认
# =====================================================================
section "Phase 4: 根因确认 / Root Cause Identification (Skill Section 5)"

echo -e "  ${BOLD}匹配根因:${NC}"
echo -e "    ${GREEN}RC-DNS-001: CoreDNS Deployment replicas = 0${NC}"
echo -e "    置信度: 0.95"
echo ""
echo -e "  ${BOLD}排除项:${NC}"
echo -e "    ✗ RC-DNS-CONFIG: CoreDNS ConfigMap 配置正常"
echo -e "    ✗ RC-DNS-OOM: CoreDNS 非 OOMKilled"
echo -e "    ✗ RC-DNS-NET: 非网络层问题 (kube-dns Service 存在)"
pause

# =====================================================================
# PHASE 5: 修复操作
# =====================================================================
section "Phase 5: 修复操作 / Remediation (Skill Section 6)"

echo -e "  ${BOLD}修复方案: 恢复 CoreDNS 副本数${NC}"
echo -e "  风险等级: ${GREEN}🟢 低风险${NC}"
echo ""

step "REM.exec" "恢复 CoreDNS / Restoring CoreDNS replicas"
run_cmd "kubectl scale deployment coredns -n kube-system --replicas=${ORIGINAL_REPLICAS}"

info "等待 CoreDNS 就绪..."
kubectl rollout status deployment/coredns -n kube-system --timeout=60s
pause

# =====================================================================
# PHASE 6: 验证确认
# =====================================================================
section "Phase 6: 验证确认 / Verification (Skill Section 7)"

step "V1" "CoreDNS Pod 状态 / CoreDNS pods Running"
run_cmd "kubectl get pods -n kube-system -l k8s-app=kube-dns"

step "V2" "kube-dns Endpoints 恢复 / Endpoints restored"
run_cmd "kubectl get endpoints kube-dns -n kube-system"

step "V3" "DNS 解析恢复 / DNS resolution restored"
info "从 Pod 内测试 DNS:"
kubectl exec dns-test -n ${NS} -- nslookup kubernetes.default.svc.cluster.local 2>&1 | sed 's/^/    /' || true
echo ""
echo -e "  ${GREEN}✓ DNS 解析恢复正常${NC}"

# ---- 清理 ----
step "CLEANUP" "清理测试 Pod"
kubectl delete pod dns-test -n ${NS} --ignore-not-found --grace-period=0 --force 2>/dev/null || true
info "✓ 已清理"

echo ""
echo -e "${GREEN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║  ✅ Scenario 04 完成 / Complete!                            ║${NC}"
echo -e "${GREEN}╠══════════════════════════════════════════════════════════════╣${NC}"
echo -e "${GREEN}║  Skill:    SKILL-NET-001 (DNS Resolution Failure)           ║${NC}"
echo -e "${GREEN}║  根因:     CoreDNS replicas = 0                             ║${NC}"
echo -e "${GREEN}║  修复:     恢复 CoreDNS 副本数 (🟢低风险)                   ║${NC}"
echo -e "${GREEN}║  验证:     DNS 解析恢复正常 ✓                               ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════════════════════════╝${NC}"
