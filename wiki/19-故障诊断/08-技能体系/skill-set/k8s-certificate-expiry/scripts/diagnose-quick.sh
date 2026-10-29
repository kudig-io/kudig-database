#!/bin/bash
# 证书过期故障快速诊断脚本
# 执行时间: ~15 秒
# 风险等级: 只读操作，零风险

set -euo pipefail

echo "=== 证书过期故障快速诊断 ==="
echo "时间: $(date -Iseconds)"
echo ""

echo "[STEP 1] kubeadm 证书过期检查（如可用）"
kubeadm certs check-expiration 2>/dev/null || echo "  kubeadm 不可用或不是 kubeadm 集群"
echo ""

echo "[STEP 2] API Server 证书详情"
kubectl get pod -n kube-system -l component=kube-apiserver -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}' 2>/dev/null | while read -r pod; do
  [ -n "$pod" ] && echo "  Pod: $pod" && kubectl exec "$pod" -n kube-system -- sh -c "openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -dates -subject" 2>/dev/null || echo "  无法读取 API Server 证书"
done
echo ""

echo "[STEP 3] 节点 kubelet 证书"
for node in $(kubectl get nodes -o jsonpath='{.items[*].metadata.name}'); do
  echo "  节点: $node"
  kubectl get node "$node" -o jsonpath='{.status.conditions[?(@.type=="Ready")].message}' 2>/dev/null | grep -i "cert" && echo "    ⚠ 证书相关错误" || echo "    ✓ 无证书错误"
done
echo ""

echo "[STEP 4] 集群事件中的证书告警"
kubectl get events --all-namespaces --field-selector reason=FailedToUpdateNodeStatus 2>/dev/null | grep -i "cert\|x509" | tail -5 || \
kubectl get events --all-namespaces 2>/dev/null | grep -i "x509\|certificate\|expired" | tail -5 || \
echo "  未发现证书相关事件"
echo ""

echo "[STEP 5] etcd 证书检查"
kubectl get pods -n kube-system -l component=etcd -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}' 2>/dev/null | while read -r pod; do
  [ -n "$pod" ] && kubectl exec "$pod" -n kube-system -- sh -c "openssl x509 -in /etc/kubernetes/pki/etcd/server.crt -noout -dates" 2>/dev/null && echo "" || echo "  无法读取 etcd 证书"
done
echo ""

echo "=== 快速诊断完成 ==="
echo ""
echo "常见根因:"
echo "  1. kubeadm 证书 1 年有效期到期 → kubeadm certs renew"
echo "  2. kubelet 客户端证书自动轮换失败 → 检查 CSR 批准"
echo "  3. 自定义 CA 证书过期 → 手动轮换 CA"
echo "  4. 系统时间不同步 → NTP 同步"
