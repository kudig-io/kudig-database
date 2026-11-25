#!/usr/bin/env bash
# Skill 05: Service 连通性诊断脚本
# Agent 执行模式: L1

set -euo pipefail
NS="${1:-default}"
SVC="${2:?用法: $0 <namespace> <service-name>}"

echo "=== Service 连通性诊断 ==="
echo "Namespace: $NS | Service: $SVC"
echo ""

echo "--- 1. Service 详情 ---"
kubectl get svc "$SVC" -n "$NS" -o wide 2>/dev/null || { echo "ERROR: Service 不存在"; exit 1; }
kubectl describe svc "$SVC" -n "$NS"

echo ""
echo "--- 2. Endpoints ---"
kubectl get endpoints "$SVC" -n "$NS" 2>/dev/null
EP_COUNT=$(kubectl get endpoints "$SVC" -n "$NS" -o jsonpath='{.subsets[0].addresses}' 2>/dev/null | python3 -c "import sys,json; print(len(json.load(sys.stdin)))" 2>/dev/null || echo "0")
echo "Endpoint 数量: $EP_COUNT"
if [ "$EP_COUNT" = "0" ]; then
    echo "⚠️  无可用 Endpoint! 检查 selector 匹配和 Pod readinessProbe"
fi

echo ""
echo "--- 3. 后端 Pod 状态 ---"
SELECTOR=$(kubectl get svc "$SVC" -n "$NS" -o jsonpath='{.spec.selector}' 2>/dev/null)
echo "Selector: $SELECTOR"
if [ -n "$SELECTOR" ]; then
    LABELS=$(echo "$SELECTOR" | python3 -c "import sys,json; d=json.load(sys.stdin); print(','.join(f'{k}={v}' for k,v in d.items()))" 2>/dev/null)
    kubectl get pods -n "$NS" -l "$LABELS" -o wide 2>/dev/null
fi

echo ""
echo "--- 4. NetworkPolicy 检查 ---"
kubectl get networkpolicy -n "$NS" 2>/dev/null | head -10

echo ""
echo "--- 5. 诊断建议 ---"
echo "Endpoints 为空: 检查 selector 是否匹配 Pod 标签"
echo "Pod 非 Ready: 检查 readinessProbe 配置"
echo "端口不匹配: 确认 targetPort 与容器监听端口一致"
echo "NetworkPolicy 阻断: 检查 ingress/egress 规则"
