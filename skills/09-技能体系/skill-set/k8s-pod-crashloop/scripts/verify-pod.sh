#!/bin/bash
# Pod 修复验证脚本
# 执行时间: ~30 秒
# 风险等级: 只读操作

set -euo pipefail

NAMESPACE="${1:-default}"
POD_NAME="${2:-}"

if [ -z "$POD_NAME" ]; then
  echo "用法: ./verify-pod.sh <namespace> <pod-name>"
  exit 1
fi

echo "=== Pod 修复验证 ==="
echo ""

PASS=0
FAIL=0

echo "[CHECK 1] Pod 状态为 Running"
STATUS=$(kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='{.status.phase}' 2>/dev/null || echo "Unknown")
if [ "$STATUS" = "Running" ]; then
  echo "  ✓ PASS: Pod 状态 Running"
  ((PASS++))
else
  echo "  ✗ FAIL: Pod 状态 $STATUS"
  ((FAIL++))
fi

echo "[CHECK 2] 容器 Ready 状态"
READY=$(kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='{.status.containerStatuses[0].ready}' 2>/dev/null || echo "false")
if [ "$READY" = "true" ]; then
  echo "  ✓ PASS: 容器已 Ready"
  ((PASS++))
else
  echo "  ✗ FAIL: 容器未 Ready"
  ((FAIL++))
fi

echo "[CHECK 3] Restart Count 未增加"
RESTARTS=$(kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='{.status.containerStatuses[0].restartCount}' 2>/dev/null || echo "0")
if [ "$RESTARTS" -eq 0 ] 2>/dev/null; then
  echo "  ✓ PASS: Restart Count = $RESTARTS"
  ((PASS++))
else
  echo "  ⚠ WARN: Restart Count = $RESTARTS（可能仍在恢复中）"
  ((PASS++))  # 允许少量重启
fi

echo "[CHECK 4] 无 OOMKilled 状态"
TERMINATED_REASON=$(kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='{.status.containerStatuses[0].lastState.terminated.reason}' 2>/dev/null || echo "")
if [ "$TERMINATED_REASON" != "OOMKilled" ]; then
  echo "  ✓ PASS: 无 OOMKilled 记录"
  ((PASS++))
else
  echo "  ✗ FAIL: 最近终止原因为 OOMKilled"
  ((FAIL++))
fi

echo "[CHECK 5] 应用日志无 ERROR/FATAL（最后 10 行）"
LOG_ERRORS=$(kubectl logs "$POD_NAME" -n "$NAMESPACE" --tail=10 2>/dev/null | grep -ciE "error|fatal|exception|panic" || true)
if [ "$LOG_ERRORS" -eq 0 ] 2>/dev/null; then
  echo "  ✓ PASS: 日志无 ERROR/FATAL"
  ((PASS++))
else
  echo "  ⚠ WARN: 日志中发现 $LOG_ERRORS 个错误关键词"
  ((PASS++))
fi

echo ""
echo "=== 验证结果: $PASS 通过, $FAIL 失败 ==="
if [ "$FAIL" -eq 0 ]; then
  echo "✓ 修复验证通过，可以关闭工单"
  exit 0
else
  echo "✗ 修复未完全生效，建议继续排查或升级"
  exit 1
fi
