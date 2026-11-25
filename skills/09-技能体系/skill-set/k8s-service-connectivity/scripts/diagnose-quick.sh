#!/bin/bash
# Service 连通性故障快速诊断脚本
# 执行时间: ~15 秒
# 风险等级: 只读操作，零风险

set -euo pipefail

NAMESPACE="${1:-default}"
SERVICE_NAME="${2:-}"

echo "=== Service 连通性故障快速诊断 ==="
echo "命名空间: $NAMESPACE"
echo "时间: $(date -Iseconds)"
echo ""

if [ -z "$SERVICE_NAME" ]; then
  echo "[STEP 1] 列出所有 Service 及其状态"
  kubectl get svc -n "$NAMESPACE" -o wide
  echo ""
  echo "请指定具体 Service: ./diagnose-quick.sh <namespace> <service-name>"
  exit 1
fi

echo "[STEP 1] Service 基本信息"
kubectl get svc "$SERVICE_NAME" -n "$NAMESPACE" -o wide
echo ""

echo "[STEP 2] Endpoints 状态（关键！）"
kubectl get endpoints "$SERVICE_NAME" -n "$NAMESPACE" -o wide
echo ""

echo "[STEP 3] Endpoints 详情"
ENDPOINTS=$(kubectl get endpoints "$SERVICE_NAME" -n "$NAMESPACE" -o jsonpath='{.subsets[*].addresses[*].ip}' 2>/dev/null || echo "")
if [ -z "$ENDPOINTS" ]; then
  echo "  ✗ 无可用 Endpoints！可能原因:"
  echo "    - Selector 标签不匹配"
  echo "    - 后端 Pod 未 Running"
  echo "    - 后端 Pod 未通过 readinessProbe"
else
  echo "  ✓ 可用 Endpoints: $ENDPOINTS"
fi
echo ""

echo "[STEP 4] 后端 Pod 状态"
SELECTOR=$(kubectl get svc "$SERVICE_NAME" -n "$NAMESPACE" -o jsonpath='{.spec.selector}' 2>/dev/null | tr ', ' '\n' | grep -v '^$' | sed 's/:/=/' | paste -sd ',' -)
if [ -n "$SELECTOR" ]; then
  echo "  Selector: $SELECTOR"
  kubectl get pods -n "$NAMESPACE" -l "$SELECTOR" -o wide 2>/dev/null || echo "  无法匹配 Pod"
else
  echo "  Service 无 Selector（可能是 ExternalName 或手动管理 Endpoints）"
fi
echo ""

echo "[STEP 5] 从集群内测试连通性"
TEST_POD=$(kubectl get pods -n "$NAMESPACE" --field-selector status.phase=Running -o jsonpath='{.items[0].metadata.name}' 2>/dev/null || echo "")
CLUSTER_IP=$(kubectl get svc "$SERVICE_NAME" -n "$NAMESPACE" -o jsonpath='{.spec.clusterIP}')
PORT=$(kubectl get svc "$SERVICE_NAME" -n "$NAMESPACE" -o jsonpath='{.spec.ports[0].port}')

if [ -n "$TEST_POD" ] && [ -n "$CLUSTER_IP" ] && [ "$CLUSTER_IP" != "None" ]; then
  echo "  测试 Pod: $TEST_POD → Service: $CLUSTER_IP:$PORT"
  kubectl exec "$TEST_POD" -n "$NAMESPACE" -- sh -c "wget -qO- --timeout=5 http://$CLUSTER_IP:$PORT/healthz 2>/dev/null || curl -s --max-time 5 http://$CLUSTER_IP:$PORT/healthz 2>/dev/null || echo '连接失败'" || echo "  测试命令执行失败"
else
  echo "  无法执行连通性测试（无可用测试 Pod 或 Service 类型不支持）"
fi
echo ""

echo "[STEP 6] 检查 NetworkPolicy"
kubectl get networkpolicy -n "$NAMESPACE" -o json | jq -r '.items[] | "  Policy: \(.metadata.name)"' 2>/dev/null || echo "  无 NetworkPolicy 或无法解析"
echo ""

echo "=== 快速诊断完成 ==="
