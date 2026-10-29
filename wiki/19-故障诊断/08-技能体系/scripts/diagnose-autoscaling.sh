#!/usr/bin/env bash
# Skill 12: 自动伸缩故障诊断脚本
# Agent 执行模式: L1

set -euo pipefail
NS="${1:-default}"
DEPLOY="${2:?用法: $0 <namespace> <deployment-name>}"

echo "=== 自动伸缩故障诊断 ==="
echo "Namespace: $NS | Deployment: $DEPLOY"
echo ""

echo "--- 1. HPA 状态 ---"
kubectl get hpa -n "$NS" 2>/dev/null | grep -i "$DEPLOY" || echo "未找到 HPA"
kubectl describe hpa -n "$NS" 2>/dev/null | grep -A20 "$DEPLOY" || true

echo ""
echo "--- 2. 当前副本数 ---"
kubectl get deploy "$DEPLOY" -n "$NS" 2>/dev/null

echo ""
echo "--- 3. Metrics Server 状态 ---"
kubectl get pods -n kube-system | grep metrics-server

echo ""
echo "--- 4. 资源使用 ---"
kubectl top pods -n "$NS" -l "app=$DEPLOY" 2>/dev/null || echo "metrics 不可用"

echo ""
echo "--- 5. HPA Events ---"
kubectl describe hpa -n "$NS" 2>/dev/null | grep -A10 "Events"

echo ""
echo "--- 6. 诊断建议 ---"
echo "HPA 不伸缩: 检查 metrics-server 是否正常"
echo "指标缺失: 确认 Pod 有 resources.requests 配置"
echo "VPA 冲突: HPA 和 VPA 不要同时管理同一维度"
echo "集群节点不足: 检查 Cluster Autoscaler / Karpenter 状态"
