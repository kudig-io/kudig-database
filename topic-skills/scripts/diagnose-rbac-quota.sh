#!/usr/bin/env bash
# Skill 09: RBAC/配额失败诊断脚本
# Agent 执行模式: L1

set -euo pipefail
NS="${1:-default}"
SA="${2:-default}"

echo "=== RBAC/配额诊断 ==="
echo "Namespace: $NS | ServiceAccount: $SA"
echo ""

echo "--- 1. 权限检查 ---"
echo "ServiceAccount $SA 在 $NS 中的权限:"
kubectl auth can-i --list --as="system:serviceaccount:$NS:$SA" -n "$NS" 2>/dev/null | head -30

echo ""
echo "--- 2. RoleBinding ---"
kubectl get rolebindings -n "$NS" -o yaml 2>/dev/null | grep -A5 "$SA" | head -20

echo ""
echo "--- 3. ClusterRoleBinding ---"
kubectl get clusterrolebindings -o yaml 2>/dev/null | grep -A5 "$SA" | head -20

echo ""
echo "--- 4. ResourceQuota ---"
kubectl get resourcequota -n "$NS" 2>/dev/null
kubectl describe resourcequota -n "$NS" 2>/dev/null

echo ""
echo "--- 5. LimitRange ---"
kubectl get limitrange -n "$NS" 2>/dev/null
kubectl describe limitrange -n "$NS" 2>/dev/null

echo ""
echo "--- 6. 诊断建议 ---"
echo "Forbidden: 创建 Role/RoleBinding 授权"
echo "Quota exceeded: 清理资源或申请提高配额"
echo "LimitRange 违规: 调整 Pod resources 配置"
