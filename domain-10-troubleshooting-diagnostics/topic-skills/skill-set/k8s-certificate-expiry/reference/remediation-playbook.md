---
title: "certificate expiry Remediation Playbook"
category: remediation
skill_set: "k8s-certificate-expiry"
created: "2026-05-22"
updated: "2026-05-22"
---

# 证书过期问题修复手册

## ⚠️ 风险提示

所有证书操作均为高风险。执行前请：
1. 确认处于维护窗口
2. 备份 `/[[entities/kubernetes|kubernetes]]/pki`
3. 通知相关团队

## 修复步骤

### 修复 1：kubeadm 自动续期（推荐）

```bash
# 1. 检查过期时间
kubeadm certs check-expiration

# 2. 备份证书
cp -r /etc/kubernetes/pki /etc/kubernetes/pki.bak.$(date +%Y%m%d)

# 3. 续期所有证书
kubeadm certs renew all

# 4. 重启控制平面组件（静态 Pod 会自动重启）
# 对于 kubelet 证书，重启 kubelet：
systemctl restart kubelet

# 5. 更新 kubeconfig
kubeadm kubeconfig user --org system:masters --client-name admin > /tmp/admin.conf
# 或重新生成所有 kubeconfig
kubeadm init phase kubeconfig all

# 6. 验证
kubeadm certs check-expiration
kubectl get nodes
```

### 修复 2：kubelet 客户端证书自动轮换

```bash
# 检查 Pending CSR
kubectl get csr

# 批准所有 Pending CSR
kubectl get csr -o json | jq -r '.items[] | select(.status == {}) | .metadata.name' | xargs kubectl certificate approve

# 验证节点恢复
kubectl get nodes
```

### 修复 3：手动替换单张证书

```bash
# 以 API Server 证书为例
# 1. 生成新证书
kubeadm certs renew apiserver

# 2. 重启对应静态 Pod
mv /etc/kubernetes/manifests/kube-apiserver.yaml /tmp/
sleep 10
mv /tmp/kube-apiserver.yaml /etc/kubernetes/manifests/
```

## 回滚方案

```bash
# 如果续期后异常，恢复备份
cp -r /etc/kubernetes/pki.bak.20260101 /etc/kubernetes/pki
systemctl restart kubelet
```
