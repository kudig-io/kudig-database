#!/usr/bin/env bash
# Skill 10: 镜像拉取失败诊断脚本
# Agent 执行模式: L1

set -euo pipefail
NS="${1:-default}"
POD="${2:?用法: $0 <namespace> <pod-name>}"

echo "=== 镜像拉取失败诊断 ==="
echo "Namespace: $NS | Pod: $POD"
echo ""

echo "--- 1. Pod 状态 ---"
kubectl get pod "$POD" -n "$NS" -o wide 2>/dev/null

echo ""
echo "--- 2. 镜像信息 ---"
kubectl get pod "$POD" -n "$NS" -o jsonpath='{range .spec.containers[*]}容器: {.name}
  镜像: {.image}
{end}' 2>/dev/null

echo ""
echo "--- 3. Events (镜像相关) ---"
kubectl get events -n "$NS" --field-selector "involvedObject.name=$POD" 2>/dev/null | grep -i "pull\|image\|auth\|denied\|not found" | tail -10

echo ""
echo "--- 4. imagePullSecrets ---"
kubectl get pod "$POD" -n "$NS" -o jsonpath='{.spec.imagePullSecrets[*].name}' 2>/dev/null
echo ""

echo ""
echo "--- 5. 节点镜像缓存 ---"
NODE=$(kubectl get pod "$POD" -n "$NS" -o jsonpath='{.spec.nodeName}' 2>/dev/null)
echo "所在节点: $NODE"

echo ""
echo "--- 6. 诊断建议 ---"
echo "镜像不存在: 检查镜像名和 tag 拼写"
echo "认证失败: 创建 docker-registry secret 并配置 imagePullSecrets"
echo "网络不通: 检查节点到 registry 的网络连通性"
echo "速率限制: 使用私有 registry 镜像或配置镜像代理"
