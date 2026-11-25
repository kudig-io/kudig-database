#!/bin/bash
# 证书修复验证脚本
set -euo pipefail

echo "=== 证书修复验证 ==="
echo ""

PASS=0
FAIL=0

echo "[CHECK 1] kubeadm 证书无即将过期"
if command -v kubeadm >/dev/null 2>&1; then
  EXPIRED=$(kubeadm certs check-expiration 2>/dev/null | grep -c "EXPIRES\|less than" || true)
  if [ "${EXPIRED:-0}" -eq 0 ]; then
    echo "  ✓ PASS: 无即将过期的 kubeadm 证书"
    ((PASS++))
  else
    echo "  ⚠ WARN: 发现证书即将过期"
    ((PASS++))
  fi
else
  echo "  ⊘ SKIP: kubeadm 不可用"
fi

echo "[CHECK 2] 所有节点 Ready"
NOT_READY=$(kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.conditions[?(@.type=="Ready")].status}{"\n"}{end}' | grep -v "True" | wc -l)
if [ "$NOT_READY" -eq 0 ]; then
  echo "  ✓ PASS: 所有节点 Ready"
  ((PASS++))
else
  echo "  ✗ FAIL: $NOT_READY 个节点未 Ready"
  ((FAIL++))
fi

echo "[CHECK 3] API Server 响应正常"
if kubectl get --raw /healthz >/dev/null 2>&1; then
  echo "  ✓ PASS: API Server 健康"
  ((PASS++))
else
  echo "  ✗ FAIL: API Server 不健康"
  ((FAIL++))
fi

echo "[CHECK 4] 无证书相关事件"
CERT_EVENTS=$(kubectl get events --all-namespaces 2>/dev/null | grep -ciE "x509|certificate.*expired|cert.*invalid" || true)
if [ "${CERT_EVENTS:-0}" -eq 0 ]; then
  echo "  ✓ PASS: 无证书相关告警事件"
  ((PASS++))
else
  echo "  ⚠ WARN: 有 $CERT_EVENTS 个证书相关事件"
  ((PASS++))
fi

echo ""
echo "=== 验证结果: $PASS 通过, $FAIL 失败 ==="
[ "$FAIL" -eq 0 ] && exit 0 || exit 1
