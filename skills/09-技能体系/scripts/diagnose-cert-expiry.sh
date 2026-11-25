#!/usr/bin/env bash
# Skill 06: 证书过期诊断脚本
# Agent 执行模式: L2

set -euo pipefail

echo "=== 证书过期诊断 ==="
echo ""

echo "--- 1. K8s 集群证书 (kubeadm) ---"
if command -v kubeadm &>/dev/null; then
    kubeadm certs check-expiration 2>/dev/null || echo "(非 kubeadm 集群)"
else
    echo "kubeadm 未安装, 手动检查:"
    for cert in /etc/kubernetes/pki/*.crt; do
        if [ -f "$cert" ]; then
            EXPIRY=$(openssl x509 -in "$cert" -noout -enddate 2>/dev/null)
            echo "  $(basename $cert): $EXPIRY"
        fi
    done
fi

echo ""
echo "--- 2. etcd 证书 ---"
ETCD_CERT="/etc/kubernetes/pki/etcd/server.crt"
if [ -f "$ETCD_CERT" ]; then
    openssl x509 -in "$ETCD_CERT" -noout -subject -dates
fi

echo ""
echo "--- 3. Kubeconfig 证书 ---"
for kc in /etc/kubernetes/*.conf; do
    if [ -f "$kc" ]; then
        CERT=$(kubectl --kubeconfig="$kc" config view --raw -o jsonpath='{.users[0].user.client-certificate-data}' 2>/dev/null | base64 -d 2>/dev/null | openssl x509 -noout -enddate 2>/dev/null)
        echo "  $(basename $kc): $CERT"
    fi
done

echo ""
echo "--- 4. TLS Secrets (集群内) ---"
kubectl get secrets -A --field-selector type=kubernetes.io/tls -o custom-columns='NAMESPACE:.metadata.namespace,NAME:.metadata.name,AGE:.metadata.creationTimestamp' 2>/dev/null | head -20

echo ""
echo "--- 5. cert-manager Certificate 资源 ---"
kubectl get certificates -A 2>/dev/null | head -20

echo ""
echo "--- 6. 诊断建议 ---"
echo "证书即将过期 (< 30天): kubeadm certs renew all"
echo "cert-manager 证书: 检查 Issuer/ClusterIssuer 状态"
echo "自动续期: 确认 kubelet --rotate-certificates=true"
echo "etcd 证书: 与控制面证书同步更新"
