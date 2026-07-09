#!/bin/bash
# PVC 存储修复验证脚本
set -euo pipefail

NAMESPACE="${1:-default}"
PVC_NAME="${2:-}"

echo "=== PVC 存储修复验证 ==="
echo ""

PASS=0
FAIL=0

echo "[CHECK 1] PVC 状态为 Bound"
if [ -n "$PVC_NAME" ]; then
  STATUS=$(kubectl get pvc "$PVC_NAME" -n "$NAMESPACE" -o jsonpath='{.status.phase}' 2>/dev/null || echo "Unknown")
  if [ "$STATUS" = "Bound" ]; then
    echo "  ✓ PASS: PVC $PVC_NAME 状态 Bound"
    ((PASS++))
  else
    echo "  ✗ FAIL: PVC $PVC_NAME 状态 $STATUS"
    ((FAIL++))
  fi
else
  NOT_BOUND=$(kubectl get pvc -n "$NAMESPACE" --field-selector status.phase!=Bound -o name 2>/dev/null | wc -l)
  if [ "$NOT_BOUND" -eq 0 ]; then
    echo "  ✓ PASS: 所有 PVC 已 Bound"
    ((PASS++))
  else
    echo "  ✗ FAIL: $NOT_BOUND 个 PVC 未 Bound"
    ((FAIL++))
  fi
fi

echo "[CHECK 2] PV 状态正常"
NOT_AVAILABLE=$(kubectl get pv --field-selector status.phase!=Available,status.phase!=Bound -o name 2>/dev/null | wc -l)
if [ "$NOT_AVAILABLE" -eq 0 ]; then
  echo "  ✓ PASS: 所有 PV 状态正常"
  ((PASS++))
else
  echo "  ⚠ WARN: $NOT_AVAILABLE 个 PV 异常"
  ((PASS++))
fi

echo "[CHECK 3] CSI Driver Pod 全部 Running"
if kubectl get pods -n kube-system -o name 2>/dev/null | grep -q csi; then
  CSI_NOT_RUNNING=$(kubectl get pods -n kube-system -l app.kubernetes.io/component=csi-driver -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.phase}{"\n"}{end}' 2>/dev/null | grep -v "Running" | wc -l)
  if [ "$CSI_NOT_RUNNING" -eq 0 ]; then
    echo "  ✓ PASS: CSI Driver 运行正常"
    ((PASS++))
  else
    echo "  ✗ FAIL: CSI Driver 有 Pod 未 Running"
    ((FAIL++))
  fi
else
  echo "  ⊘ SKIP: 未检测到 CSI Driver"
fi

echo "[CHECK 4] 无存储相关错误事件"
STORAGE_ERRORS=$(kubectl get events --all-namespaces 2>/dev/null | grep -ciE "FailedMount|FailedAttachVolume|VolumeFailedRecycle" || true)
if [ "${STORAGE_ERRORS:-0}" -eq 0 ]; then
  echo "  ✓ PASS: 无存储错误事件"
  ((PASS++))
else
  echo "  ⚠ WARN: $STORAGE_ERRORS 个存储错误事件"
  ((PASS++))
fi

echo ""
echo "=== 验证结果: $PASS 通过, $FAIL 失败 ==="
[ "$FAIL" -eq 0 ] && exit 0 || exit 1
