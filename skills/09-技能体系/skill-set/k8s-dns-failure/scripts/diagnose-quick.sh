#!/bin/bash
# DNS 解析故障快速诊断脚本
# 执行时间: ~15 秒
# 风险等级: 只读操作，零风险

set -euo pipefail

NAMESPACE="${1:-default}"
POD_NAME="${2:-}"
DNS_NAME="${3:-kubernetes.default}"

echo "=== DNS 解析故障快速诊断 ==="
echo "命名空间: $NAMESPACE | 测试域名: $DNS_NAME"
echo "时间: $(date -Iseconds)"
echo ""

echo "[STEP 1] CoreDNS Pod 状态"
kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide 2>/dev/null || \
kubectl get pods -n kube-system -l app.kubernetes.io/name=coredns -o wide 2>/dev/null || \
echo "  无法获取 CoreDNS Pod 状态"
echo ""

echo "[STEP 2] CoreDNS Service 端点"
kubectl get endpoints coredns -n kube-system 2>/dev/null || \
kubectl get endpoints kube-dns -n kube-system 2>/dev/null || \
echo "  无法获取 CoreDNS endpoints"
echo ""

echo "[STEP 3] Cluster DNS Service IP"
kubectl get svc kube-dns -n kube-system -o jsonpath='{.spec.clusterIP}' 2>/dev/null || echo "N/A"
echo ""

echo "[STEP 4] 从测试 Pod 执行 DNS 查询"
if [ -n "$POD_NAME" ]; then
  echo "使用指定 Pod: $POD_NAME"
  kubectl exec "$POD_NAME" -n "$NAMESPACE" -- nslookup "$DNS_NAME" 2>/dev/null || \
  kubectl exec "$POD_NAME" -n "$NAMESPACE" -- dig +short "$DNS_NAME" 2>/dev/null || \
  echo "  无法从 Pod 执行 DNS 查询（可能无 nslookup/dig）"
else
  echo "未指定测试 Pod，跳过 Pod 内 DNS 测试"
  echo "  建议: ./diagnose-quick.sh <namespace> <pod-name> <dns-name>"
fi
echo ""

echo "[STEP 5] 节点级别 DNS 测试"
NODE_IP=$(kubectl get nodes -o jsonpath='{.items[0].status.addresses[?(@.type=="InternalIP")].address}')
echo "节点 IP: $NODE_IP (仅供参考)"
echo ""

echo "[STEP 6] CoreDNS 日志（最近 20 行错误）"
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=20 2>/dev/null | grep -iE "error|fail|timeout|refused" | tail -10 || \
kubectl logs -n kube-system -l app.kubernetes.io/name=coredns --tail=20 2>/dev/null | grep -iE "error|fail|timeout|refused" | tail -10 || \
echo "  无法获取 CoreDNS 日志"
echo ""

echo "[STEP 7] DNS Policy 检查"
if [ -n "$POD_NAME" ]; then
  kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='{.spec.dnsPolicy}{"\n"}{.spec.dnsConfig}{"\n"}'
fi
echo ""

echo "=== 快速诊断完成 ==="
echo ""
echo "常见根因:"
echo "  1. CoreDNS Pod 未运行/崩溃 → 检查资源限制和节点状态"
echo "  2. CoreDNS ConfigMap 错误 → 检查 Corefile 配置"
echo "  3. 网络策略阻断 UDP/53 → 检查 NetworkPolicy"
echo "  4. 节点 iptables/nftables 规则冲突 → 检查防火墙"
