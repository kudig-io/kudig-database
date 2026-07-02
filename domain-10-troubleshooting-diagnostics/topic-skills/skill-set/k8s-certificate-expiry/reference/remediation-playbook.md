---
title: certificate expiry Remediation Playbook
summary: certificate expiry Remediation Playbook：所有证书操作均为高风险。执行前请： 1. 确认处于维护窗口 2.
  备份 /entities/kubernetes.md/pki 3. 通知相关团队
category: remediation
tags:
- reference
- remediation
- playbook
- visibility/public
tier: supporting
created: '2026-05-22'
updated: '2026-05-22'
skill_set: k8s-certificate-expiry
last_updated: 2026-05-22
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 证书过期问题修复手册

## ⚠️ 风险提示

所有证书操作均为高风险。执行前请：
1. 确认处于维护窗口
2. 备份 `/[[entities/kubernetes.md|kubernetes]]/pki`
3. 通知相关团队

## 修复步骤

### 修复 1：kubeadm 自动续期（推荐）

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
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

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
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

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 如果续期后异常，恢复备份
cp -r /etc/kubernetes/pki.bak.20260101 /etc/kubernetes/pki
systemctl restart kubelet
```
## 参见

- [[remediation-playbook]] — reference 领域核心页面

## Related

- [[reference|#reference Hub]] — tag hub

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
