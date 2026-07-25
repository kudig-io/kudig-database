#!/bin/bash
# Pod CrashLoopBackOff / OOMKilled 深度诊断脚本
# 执行时间: ~2-5 分钟
# 风险等级: 只读操作，零风险

set -euo pipefail

NAMESPACE="${1:-default}"
POD_NAME="${2:-}"

if [ -z "$POD_NAME" ]; then
  echo "用法: ./diagnose-deep.sh <namespace> <pod-name>"
  exit 1
fi

echo "=== Pod CrashLoopBackOff / OOMKilled 深度诊断 ==="
echo "Pod: $POD_NAME / Namespace: $NAMESPACE"
echo ""

echo "[DEEP 1] 完整 Pod Spec 分析"
kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o yaml | grep -E "restartPolicy:|imagePullPolicy:|terminationGracePeriodSeconds:|livenessProbe:|readinessProbe:|startupProbe:" | head -20
echo ""

echo "[DEEP 2] 容器资源使用趋势（如 metrics-server 可用）"
kubectl top pod "$POD_NAME" -n "$NAMESPACE" --containers 2>/dev/null || echo "  metrics-server 不可用，跳过资源趋势"
echo ""

echo "[DEEP 3] 节点资源压力"
NODE=$(kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='{.spec.nodeName}')
echo "Pod 所在节点: $NODE"
kubectl describe node "$NODE" | grep -E "Allocated resources:|Pressure|Condition" | head -15
echo ""

echo "[DEEP 4] 启动探针/健康检查配置"
kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='{range .spec.containers[*]}{"  容器: "}{.name}{"\n  livenessProbe: "}{.livenessProbe}{"\n  readinessProbe: "}{.readinessProbe}{"\n  startupProbe: "}{.startupProbe}{"\n\n"}{end}'
echo ""

echo "[DEEP 5] 完整日志分析（最后 200 行，含堆栈跟踪）"
kubectl logs "$POD_NAME" -n "$NAMESPACE" --tail=200 --previous 2>/dev/null | tail -50 || kubectl logs "$POD_NAME" -n "$NAMESPACE" --tail=200 2>/dev/null | tail -50 || echo "  无法获取日志"
echo ""

echo "[DEEP 6] 事件时间线"
kubectl get events -n "$NAMESPACE" --field-selector involvedObject.name="$POD_NAME" -o json | jq -r '.items[] | "\(.lastTimestamp) \(.reason): \(.message)"' 2>/dev/null | tail -20 || \
kubectl get events -n "$NAMESPACE" --field-selector involvedObject.name="$POD_NAME" --sort-by='.lastTimestamp'
echo ""

echo "[DEEP 7] 关联资源检查"
echo "  Deployment/Owner:"
kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='{.metadata.ownerReferences[0].name}' 2>/dev/null || echo "  无 owner"
echo ""
echo "  ServiceAccount:"
kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='{.spec.serviceAccountName}'
echo ""

echo "=== 深度诊断完成 ==="
