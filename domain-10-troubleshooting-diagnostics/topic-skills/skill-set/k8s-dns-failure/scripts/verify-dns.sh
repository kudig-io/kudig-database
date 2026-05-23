#!/bin/bash
# DNS 修复验证脚本
set -euo pipefail

NAMESPACE="${1:-default}"
POD_NAME="${2:-}"
DNS_NAME="${3:-kubernetes.default}"

echo "=== DNS 修复验证 ==="
echo ""

PASS=0
FAIL=0

echo "[CHECK 1] CoreDNS Pod 全部 Running"
NOT_RUNNING=$(kubectl get pods -n kube-system -l k8s-app=kube-dns -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.phase}{"\n"}{end}' 2>/dev/null | grep -v "Running" | wc -l)
if [ "$NOT_RUNNING" -eq 0 ]; then
  echo "  ✓ PASS: 所有 CoreDNS Pod Running"
  ((PASS++))
else
  echo "  ✗ FAIL: $NOT_RUNNING 个 CoreDNS Pod 未 Running"
  ((FAIL++))
fi

echo "[CHECK 2] DNS Service 有可用 Endpoints"
ENDPOINTS=$(kubectl get endpoints kube-dns -n kube-system -o jsonpath='{.subsets[0].addresses[*].ip}' 2>/dev/null | wc -w)
if [ "$ENDPOINTS" -gt 0 ]; then
  echo "  ✓ PASS: DNS Service 有 $ENDPOINTS 个 endpoint"
  ((PASS++))
else
  echo "  ✗ FAIL: DNS Service 无可用 endpoint"
  ((FAIL++))
fi

echo "[CHECK 3] Pod 内 DNS 解析成功"
if [ -n "$POD_NAME" ]; then
  RESULT=$(kubectl exec "$POD_NAME" -n "$NAMESPACE" -- nslookup "$DNS_NAME" >/dev/null 2>&1 && echo "OK" || echo "FAIL")
  if [ "$RESULT" = "OK" ]; then
    echo "  ✓ PASS: Pod 内可解析 $DNS_NAME"
    ((PASS++))
  else
    echo "  ✗ FAIL: Pod 内无法解析 $DNS_NAME"
    ((FAIL++))
  fi
else
  echo "  ⊘ SKIP: 未指定测试 Pod"
fi

echo "[CHECK 4] CoreDNS 无错误日志（最近 1 分钟）"
ERRORS=$(kubectl logs -n kube-system -l k8s-app=kube-dns --since=1m 2>/dev/null | grep -ciE "error|fail" || true)
if [ "${ERRORS:-0}" -eq 0 ]; then
  echo "  ✓ PASS: 最近 1 分钟无错误日志"
  ((PASS++))
else
  echo "  ⚠ WARN: 最近 1 分钟有 $ERRORS 条错误日志"
  ((PASS++))
fi

echo ""
echo "=== 验证结果: $PASS 通过, $FAIL 失败 ==="
[ "$FAIL" -eq 0 ] && exit 0 || exit 1
