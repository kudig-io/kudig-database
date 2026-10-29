#!/usr/bin/env bash
# Skill 03: Pod Pending 诊断脚本
# 用法: ./diagnose-pod-pending.sh <namespace> <pod-name>
# Agent 执行模式: L2

set -euo pipefail
NS="${1:-default}"
POD="${2:?用法: $0 <namespace> <pod-name>}"

echo "=== Pod Pending 诊断 ==="
echo "Namespace: $NS | Pod: $POD"
echo ""

echo "--- 1. Pod 状态 ---"
kubectl get pod "$POD" -n "$NS" -o wide 2>/dev/null || { echo "ERROR: Pod 不存在"; exit 1; }

echo ""
echo "--- 2. 调度失败原因 ---"
kubectl get pod "$POD" -n "$NS" -o jsonpath='{range .status.conditions[*]}类型: {.type}
  状态: {.status}
  原因: {.reason}
  消息: {.message}
{"---"}{end}' 2>/dev/null

echo ""
echo "--- 3. Events (调度相关) ---"
kubectl get events -n "$NS" --field-selector "involvedObject.name=$POD" --sort-by='.lastTimestamp' 2>/dev/null | grep -i "schedul\|pending\|insufficient\|affinity\|taint\|node" | tail -10

echo ""
echo "--- 4. 节点资源状况 ---"
echo "节点 CPU/Memory 可分配资源:"
kubectl get nodes -o custom-columns='NAME:.metadata.name,CPU_ALLOC:.status.allocatable.cpu,MEM_ALLOC:.status.allocatable.memory,PODS:.status.allocatable.pods'

echo ""
echo "--- 5. 节点状态 ---"
kubectl get nodes -o custom-columns='NAME:.metadata.name,STATUS:.status.conditions[-1].type,READY:.status.conditions[-1].status'

echo ""
echo "--- 6. Pod 资源请求 ---"
kubectl get pod "$POD" -n "$NS" -o jsonpath='Requests: cpu={.spec.containers[0].resources.requests.cpu} mem={.spec.containers[0].resources.requests.memory}' 2>/dev/null
echo ""

echo ""
echo "--- 7. 诊断建议 ---"
echo "Insufficient cpu/memory: 降低 requests 或扩容节点"
echo "NodeAffinity: 检查 nodeSelector/nodeAffinity 是否有匹配节点"
echo "Taint/Toleration: 检查节点污点和 Pod 容忍配置"
echo "PVC Pending: kubectl get pvc -n $NS 检查存储卷绑定状态"
echo "ResourceQuota: kubectl describe resourcequota -n $NS 检查配额"
