#!/usr/bin/env bash
# Skill 08: Deployment 发布失败诊断脚本
# Agent 执行模式: L2

set -euo pipefail
NS="${1:-default}"
DEPLOY="${2:?用法: $0 <namespace> <deployment-name>}"

echo "=== Deployment 发布失败诊断 ==="
echo "Namespace: $NS | Deployment: $DEPLOY"
echo ""

echo "--- 1. Deployment 状态 ---"
kubectl get deploy "$DEPLOY" -n "$NS" 2>/dev/null || { echo "ERROR: Deployment 不存在"; exit 1; }
kubectl rollout status deploy/"$DEPLOY" -n "$NS" --timeout=5s 2>/dev/null || true

echo ""
echo "--- 2. ReplicaSet 列表 ---"
kubectl get rs -n "$NS" -l "app=$DEPLOY" --sort-by='.metadata.creationTimestamp' 2>/dev/null

echo ""
echo "--- 3. 新 ReplicaSet Pod 状态 ---"
NEW_RS=$(kubectl get rs -n "$NS" -l "app=$DEPLOY" --sort-by='.metadata.creationTimestamp' -o jsonpath='{.items[-1].metadata.name}' 2>/dev/null)
if [ -n "$NEW_RS" ]; then
    echo "最新 ReplicaSet: $NEW_RS"
    kubectl get pods -n "$NS" -l "app=$DEPLOY" -o wide 2>/dev/null
fi

echo ""
echo "--- 4. Events ---"
kubectl get events -n "$NS" --sort-by='.lastTimestamp' 2>/dev/null | grep -i "$DEPLOY\|replicaset\|pod" | tail -15

echo ""
echo "--- 5. Rollout 历史 ---"
kubectl rollout history deploy/"$DEPLOY" -n "$NS" 2>/dev/null

echo ""
echo "--- 6. 诊断建议 ---"
echo "新 Pod CrashLoop: 参考 Skill 02 诊断"
echo "新 Pod Pending: 参考 Skill 03 诊断"
echo "ImagePullBackOff: 检查镜像名/tag 和 imagePullSecrets"
echo "回滚: kubectl rollout undo deploy/$DEPLOY -n $NS"
echo "暂停发布: kubectl rollout pause deploy/$DEPLOY -n $NS"
