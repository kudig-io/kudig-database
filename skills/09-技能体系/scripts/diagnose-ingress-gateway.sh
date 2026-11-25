#!/usr/bin/env bash
# Skill 13: Ingress/Gateway 故障诊断脚本
# Agent 执行模式: L2

set -euo pipefail
NS="${1:-default}"
INGRESS="${2:?用法: $0 <namespace> <ingress-name>}"

echo "=== Ingress/Gateway 故障诊断 ==="
echo "Namespace: $NS | Ingress: $INGRESS"
echo ""

echo "--- 1. Ingress 状态 ---"
kubectl get ingress "$INGRESS" -n "$NS" 2>/dev/null || { echo "ERROR: Ingress 不存在"; exit 1; }
kubectl describe ingress "$INGRESS" -n "$NS"

echo ""
echo "--- 2. Ingress Controller 状态 ---"
kubectl get pods -A | grep -i "ingress\|nginx\|traefik\|envoy" | head -10

echo ""
echo "--- 3. 后端 Service/Endpoints ---"
kubectl get ingress "$INGRESS" -n "$NS" -o jsonpath='{range .spec.rules[*].http.paths[*]}Path: {.path} -> Service: {.backend.service.name}:{.backend.service.port.number}
{end}' 2>/dev/null

echo ""
echo "--- 4. TLS 配置 ---"
kubectl get ingress "$INGRESS" -n "$NS" -o jsonpath='{.spec.tls[*].hosts}' 2>/dev/null
echo ""

echo ""
echo "--- 5. Ingress Controller 日志 ---"
IC_POD=$(kubectl get pods -A -l app.kubernetes.io/name=ingress-nginx -o jsonpath='{.items[0].metadata.name}' 2>/dev/null)
if [ -n "$IC_POD" ]; then
    kubectl logs -A "$IC_POD" --tail=30 2>/dev/null | grep -i "$INGRESS\|error\|502\|503"
fi

echo ""
echo "--- 6. 诊断建议 ---"
echo "502 Bad Gateway: 后端 Pod 未就绪或 Service 端口错误"
echo "404 Not Found: 检查 host 和 path 匹配规则"
echo "TLS 错误: 检查 Secret 是否存在且包含有效证书"
echo "Ingress Controller 异常: 检查 Controller Pod 状态和日志"
