#!/usr/bin/env bash
# Skill 18: 安全事件响应诊断脚本
# Agent 执行模式: L1

set -euo pipefail
NS="${1:-default}"

echo "=== 安全事件响应诊断 ==="
echo "Namespace: $NS"
echo ""

echo "--- 1. 异常 Pod 检查 ---"
echo "特权容器:"
kubectl get pods -n "$NS" -o json 2>/dev/null | python3 -c "
import sys, json
data = json.load(sys.stdin)
for pod in data['items']:
    for c in pod['spec'].get('containers', []):
        sc = c.get('securityContext', {})
        if sc.get('privileged') or sc.get('runAsUser') == 0:
            print(f\"  ⚠️  {pod['metadata']['name']}: 容器 {c['name']} 运行特权/Root\")
" 2>/dev/null

echo ""
echo "--- 2. 最近创建的 Pod ---"
kubectl get pods -n "$NS" --sort-by='.metadata.creationTimestamp' 2>/dev/null | tail -10

echo ""
echo "--- 3. 异常 ServiceAccount ---"
kubectl get serviceaccounts -n "$NS" 2>/dev/null

echo ""
echo "--- 4. NetworkPolicy ---"
kubectl get networkpolicy -n "$NS" 2>/dev/null

echo ""
echo "--- 5. 审计日志 (最近可疑操作) ---"
echo "检查 API Server 审计日志:"
echo "  /var/log/kubernetes/audit/audit.log"
echo "  或 kubectl get events -n $NS --sort-by='.lastTimestamp'"

echo ""
echo "--- 6. RBAC 过度授权检查 ---"
kubectl get rolebindings -n "$NS" -o json 2>/dev/null | python3 -c "
import sys, json
data = json.load(sys.stdin)
for rb in data['items']:
    role = rb.get('roleRef', {}).get('name', '')
    if role in ('cluster-admin', 'admin'):
        subjects = rb.get('subjects', [])
        for s in subjects:
            print(f\"  ⚠️  {rb['metadata']['name']}: {s.get('name')} -> {role}\")
" 2>/dev/null

echo ""
echo "--- 7. 安全建议 ---"
echo "发现特权容器: 评估是否必要, 否则移除 privileged 权限"
echo "无 NetworkPolicy: 添加默认 deny-all 策略"
echo "cluster-admin 绑定: 收敛为最小权限 Role"
echo "可疑 Pod: kubectl get pod <pod> -n $NS -o yaml 审查完整配置"
echo "响应流程: 隔离 → 取证 → 修复 → 复盘"
