#!/usr/bin/env bash
# Skill 04: DNS 解析失败诊断脚本
# Agent 执行模式: L2

set -euo pipefail
NS="${1:-default}"
TEST_SVC="${2:-kubernetes.default.svc.cluster.local}"

echo "=== DNS 解析失败诊断 ==="
echo "Namespace: $NS | 测试域名: $TEST_SVC"
echo ""

echo "--- 1. CoreDNS Pod 状态 ---"
kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide

echo ""
echo "--- 2. CoreDNS 日志 (最近 50 行) ---"
COREDNS_POD=$(kubectl get pods -n kube-system -l k8s-app=kube-dns -o jsonpath='{.items[0].metadata.name}' 2>/dev/null)
if [ -n "$COREDNS_POD" ]; then
    kubectl logs -n kube-system "$COREDNS_POD" --tail=50 2>/dev/null | tail -20
fi

echo ""
echo "--- 3. CoreDNS ConfigMap ---"
kubectl get cm coredns -n kube-system -o yaml 2>/dev/null | head -40

echo ""
echo "--- 4. DNS Service (ClusterIP) ---"
kubectl get svc -n kube-system -l k8s-app=kube-dns

echo ""
echo "--- 5. DNS 解析测试 (从 debug Pod) ---"
echo "运行以下命令测试:"
echo "  kubectl run dns-test --image=busybox:1.36 --rm -it --restart=Never -- nslookup $TEST_SVC"

echo ""
echo "--- 6. CoreDNS Endpoints ---"
kubectl get endpoints -n kube-system -l k8s-app=kube-dns 2>/dev/null

echo ""
echo "--- 7. 诊断建议 ---"
echo "CoreDNS CrashLoop: 检查 CoreDNS ConfigMap 语法"
echo "DNS Service 无 Endpoint: 检查 CoreDNS Pod 标签和 readinessProbe"
echo "部分域名解析失败: 检查 CoreDNS 的 forward 配置"
echo "解析延迟高: 检查 NodeLocal DNSCache 是否启用"
