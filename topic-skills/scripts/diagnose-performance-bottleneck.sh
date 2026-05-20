#!/usr/bin/env bash
# Skill 17: 性能瓶颈诊断脚本
# Agent 执行模式: L2

set -euo pipefail
NS="${1:-default}"
POD="${2:?用法: $0 <namespace> <pod-name>}"

echo "=== 性能瓶颈诊断 ==="
echo "Namespace: $NS | Pod: $POD"
echo ""

echo "--- 1. Pod 资源使用 ---"
kubectl top pod "$POD" -n "$NS" 2>/dev/null || echo "metrics 不可用"

echo ""
echo "--- 2. 资源限制 ---"
kubectl get pod "$POD" -n "$NS" -o jsonpath='{range .spec.containers[*]}容器: {.name}
  Requests: cpu={.resources.requests.cpu} mem={.resources.requests.memory}
  Limits:   cpu={.resources.limits.cpu} mem={.resources.limits.memory}
{end}' 2>/dev/null

echo ""
echo "--- 3. CPU 节流检查 ---"
NODE=$(kubectl get pod "$POD" -n "$NS" -o jsonpath='{.spec.nodeName}' 2>/dev/null)
echo "所在节点: $NODE"

echo ""
echo "--- 4. 节点资源使用 ---"
kubectl top node "$NODE" 2>/dev/null || echo "metrics 不可用"

echo ""
echo "--- 5. Pod QoS 类 ---"
QOS=$(kubectl get pod "$POD" -n "$NS" -o jsonpath='{.status.qosClass}' 2>/dev/null)
echo "QoS Class: $QOS"

echo ""
echo "--- 6. HPA 状态 ---"
kubectl get hpa -n "$NS" 2>/dev/null

echo ""
echo "--- 7. 诊断建议 ---"
echo "CPU 被节流: 增大 CPU limits 或优化应用 CPU 使用"
echo "内存接近 limits: 增大 limits.memory 或排查内存泄漏"
echo "节点资源不足: 扩容节点或调整 Pod 调度策略"
echo "QoS BestEffort: 设置 requests 提升优先级"
echo "磁盘 IO 慢: 检查存储类型和 PV 性能"
