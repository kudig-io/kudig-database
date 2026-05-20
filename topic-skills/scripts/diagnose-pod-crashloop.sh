#!/usr/bin/env bash
# Skill 02: Pod CrashLoopBackOff / OOMKilled 诊断脚本
# 用法: ./diagnose-pod-crashloop.sh <namespace> <pod-name>
# Agent 执行模式: L2 (半自动 — 只读自动, 修复需人工确认)

set -euo pipefail
NS="${1:-default}"
POD="${2:?用法: $0 <namespace> <pod-name>}"

echo "=== Pod CrashLoopBackOff / OOMKilled 诊断 ==="
echo "Namespace: $NS | Pod: $POD"
echo ""

echo "--- 1. Pod 状态 ---"
kubectl get pod "$POD" -n "$NS" -o wide 2>/dev/null || { echo "ERROR: Pod 不存在"; exit 1; }

echo ""
echo "--- 2. 容器状态详情 ---"
kubectl get pod "$POD" -n "$NS" -o jsonpath='{range .status.containerStatuses[*]}容器: {.name}
  状态: {.state}
  重启次数: {.restartCount}
  最后状态: {.lastState}
  Ready: {.ready}
{"---"}{end}'

echo ""
echo "--- 3. 上次崩溃日志 (最多 100 行) ---"
kubectl logs "$POD" -n "$NS" --previous --tail=100 2>/dev/null || echo "(无 previous 日志)"

echo ""
echo "--- 4. 当前日志 (最多 50 行) ---"
kubectl logs "$POD" -n "$NS" --tail=50 2>/dev/null || echo "(无日志)"

echo ""
echo "--- 5. Events ---"
kubectl get events -n "$NS" --field-selector "involvedObject.name=$POD" --sort-by='.lastTimestamp' 2>/dev/null | tail -20

echo ""
echo "--- 6. 资源限制与使用 ---"
kubectl get pod "$POD" -n "$NS" -o jsonpath='{range .spec.containers[*]}容器: {.name}
  Requests: cpu={.resources.requests.cpu} mem={.resources.requests.memory}
  Limits:   cpu={.resources.limits.cpu} mem={.resources.limits.memory}
{end}' 2>/dev/null

echo ""
echo "--- 7. OOMKilled 检查 ---"
OOM=$(kubectl get pod "$POD" -n "$NS" -o jsonpath='{range .status.containerStatuses[*]}{.lastState.terminated.reason}{" "}{end}' 2>/dev/null)
if echo "$OOM" | grep -q "OOMKilled"; then
    echo "⚠️  检测到 OOMKilled! 建议增大 limits.memory"
fi

echo ""
echo "--- 8. 诊断建议 ---"
echo "1. 检查崩溃日志中的错误信息"
echo "2. 如果是 OOMKilled, 增大 resources.limits.memory"
echo "3. 如果是启动失败, 检查 command/args 和环境变量"
echo "4. 如果是依赖服务, 检查 init containers 和 readinessProbe"
echo "5. 修复后: kubectl rollout restart deployment/<deployment> -n $NS"
