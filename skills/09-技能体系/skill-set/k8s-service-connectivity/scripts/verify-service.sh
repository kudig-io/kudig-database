#!/bin/bash
# Service 连通性修复验证脚本
set -euo pipefail

NAMESPACE="${1:-default}"
SERVICE_NAME="${2:-}"

echo "=== Service 连通性修复验证 ==="
echo ""

PASS=0
FAIL=0

echo "[CHECK 1] Service 存在且配置正确"
if kubectl get svc "$SERVICE_NAME" -n "$NAMESPACE" >/dev/null 2>&1; then
  echo "  ✓ PASS: Service 存在"
  ((PASS++))
else
  echo "  ✗ FAIL: Service 不存在"
  ((FAIL++))
  exit 1
fi

echo "[CHECK 2] Endpoints 非空"
ENDPOINTS=$(kubectl get endpoints "$SERVICE_NAME" -n "$NAMESPACE" -o jsonpath='{.subsets[*].addresses[*].ip}' 2>/dev/null || echo "")
if [ -n "$ENDPOINTS" ]; then
  echo "  ✓ PASS: Endpoints 可用"
  ((PASS++))
else
  echo "  ✗ FAIL: Endpoints 为空"
  ((FAIL++))
fi

echo "[CHECK 3] 后端 Pod 全部 Running"
SELECTOR=$(kubectl get svc "$SERVICE_NAME" -n "$NAMESPACE" -o jsonpath='{.spec.selector}' 2>/dev/null | tr ', ' '\n' | grep -v '^$' | sed 's/:/=/' | paste -sd ',' -)
if [ -n "$SELECTOR" ]; then
  NOT_RUNNING=$(kubectl get pods -n "$NAMESPACE" -l "$SELECTOR" --field-selector status.phase!=Running -o name 2>/dev/null | wc -l)
  if [ "$NOT_RUNNING" -eq 0 ]; then
    echo "  ✓ PASS: 所有后端 Pod Running"
    ((PASS++))
  else
    echo "  ✗ FAIL: $NOT_RUNNING 个后端 Pod 未 Running"
    ((FAIL++))
  fi
else
  echo "  ⊘ SKIP: Service 无 Selector"
fi

echo "[CHECK 4] 集群内连通性测试"
TEST_POD=$(kubectl get pods -n "$NAMESPACE" --field-selector status.phase=Running -o jsonpath='{.items[0].metadata.name}' 2>/dev/null || echo "")
CLUSTER_IP=$(kubectl get svc "$SERVICE_NAME" -n "$NAMESPACE" -o jsonpath='{.spec.clusterIP}')
PORT=$(kubectl get svc "$SERVICE_NAME" -n "$NAMESPACE" -o jsonpath='{.spec.ports[0].port}')
if [ -n "$TEST_POD" ] && [ -n "$CLUSTER_IP" ] && [ "$CLUSTER_IP" != "None" ]; then
  RESULT=$(kubectl exec "$TEST_POD" -n "$NAMESPACE" -- sh -c "wget -qO- --timeout=3 http://$CLUSTER_IP:$PORT/ 2>/dev/null || curl -s --max-time 3 http://$CLUSTER_IP:$PORT/ 2>/dev/null || echo 'FAIL'" || echo "FAIL")
  if [ "$RESULT" != "FAIL" ] && [ "$RESULT" != "连接失败" ]; then
    echo "  ✓ PASS: 集群内可访问 Service"
    ((PASS++))
  else
    echo "  ✗ FAIL: 集群内无法访问 Service"
    ((FAIL++))
  fi
else
  echo "  ⊘ SKIP: 无法执行连通性测试"
fi

echo ""
echo "=== 验证结果: $PASS 通过, $FAIL 失败 ==="
[ "$FAIL" -eq 0 ] && exit 0 || exit 1
