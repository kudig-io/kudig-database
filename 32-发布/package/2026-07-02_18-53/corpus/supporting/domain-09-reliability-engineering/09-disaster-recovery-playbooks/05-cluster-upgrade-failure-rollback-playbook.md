---
title: K8s 集群升级失败回滚
description: 'Kubernetes 集群升级失败场景分类、etcd 降级保护、API Server 兼容性处理及组件配置恢复'
summary: 'Kubernetes 集群升级失败场景分类、etcd 降级保护、API Server 兼容性处理及组件配置恢复'
category: reliability-engineering
tags:
- disaster-recovery
- k8s
- sre
- upgrade
- rollback
- kubeadm
tier: critical
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- K8s 集群升级失败回滚 是什么
- 如何 K8s 集群升级失败回滚
- kubeadm upgrade 失败处理
trigger_keywords:
- upgrade
- rollback
- kubeadm
- etcd-downgrade
- api-server
prerequisites:
- kubectl-basics
- etcd-basics
- sre-practices
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# K8s 集群升级失败回滚

## 概述

Kubernetes 集群升级是高风险操作，涉及控制面组件（API Server、Controller Manager、Scheduler、etcd）和数据面组件（kubelet、kube-proxy）的版本变更。升级过程中可能遇到多种失败场景，若处理不当可能导致集群不可用。

本手册覆盖以下升级失败场景：

- **kubeadm upgrade 失败**：apply/plan/node 阶段的各类失败
- **etcd 降级保护**：etcd 版本升级失败时的安全回退策略
- **API Server 兼容性问题**：API 弃用、版本不匹配导致的启动失败
- **kubelet 版本回退**：节点组件升级失败的恢复
- **组件配置恢复**：升级过程中配置文件损坏的修复

### 升级阶段与风险

| 升级阶段 | 风险等级 | 失败影响 |
|----------|----------|----------|
| kubeadm upgrade apply | 高 | 控制面不可用 |
| etcd 版本升级 | 高 | 数据丢失风险 |
| kubelet 升级 | 中 | 单节点不可用 |
| CNI 升级 | 中 | Pod 网络中断 |
| kube-proxy 升级 | 低 | Service 路由短暂中断 |

## 详细步骤

### 第一阶段：升级失败诊断

#### 1.1 确认升级阶段

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 kubeadm 升级历史
kubeadm version
kubectl version

# 查看控制面组件版本
kubectl get nodes -o wide
kubectl get pods -n kube-system -o wide | grep -E "kube-apiserver|kube-controller|kube-scheduler|etcd"

# 检查 kubeadm ConfigMap 中的集群配置
kubectl get cm kubeadm-config -n kube-system -o yaml

# 查看升级相关的事件
kubectl get events --all-namespaces --sort-by='.lastTimestamp' | grep -i upgrade
```
#### 1.2 控制面组件状态检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 API Server 状态
kubectl get --raw='/healthz?verbose'
curl -k https://127.0.0.1:6443/healthz

# 检查 Controller Manager
kubectl get --raw='/metrics' | grep leader_election_master

# 检查 Scheduler
kubectl get endpoints kube-scheduler -n kube-system -o yaml

# 检查 etcd 健康
ETCDCTL_API=3 etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint health
```
#### 1.3 日志分析

```bash
# API Server 日志
journalctl -u kube-apiserver --since "1 hour ago" | grep -iE "error|fatal|warning"

# Controller Manager 日志
journalctl -u kube-controller-manager --since "1 hour ago" | grep -iE "error|fatal"

# Scheduler 日志
journalctl -u kube-scheduler --since "1 hour ago" | grep -iE "error|fatal"

# etcd 日志
journalctl -u etcd --since "1 hour ago" | grep -iE "error|fatal|alarm"
```

### 第二阶段：kubeadm upgrade 失败处理

#### 2.1 upgrade apply 失败

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 场景：kubeadm upgrade apply 失败，控制面部分组件已升级

# 查看当前升级状态
kubeadm upgrade plan

# 查看已升级的组件版本
kubectl get pods -n kube-system kube-apiserver-<control-plane> -o jsonpath='{.spec.containers[0].image}'
```
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 回滚策略 1：重新执行 upgrade apply（修复配置问题后）
# 先修复导致失败的问题（如证书、配置文件等）
kubeadm upgrade apply v<target-version>

# 回滚策略 2：手动回退组件版本
# 停止当前版本的组件
systemctl stop kube-apiserver kube-controller-manager kube-scheduler

# 安装旧版本的 kubeadm
yum install -y kubeadm-<old-version> --allow-downgrade

# 使用 kubeadm 重新部署旧版本
kubeadm upgrade apply v<old-version>
```
#### 2.2 upgrade node 失败

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
# 场景：kubeadm upgrade node 在工作节点上失败

# 查看节点状态
kubectl get node <node-name> -o wide

# 回退 kubelet
yum install -y kubelet-<old-version> kubectl-<old-version> --allow-downgrade
systemctl daemon-reload
systemctl restart kubelet

# 验证节点状态
kubectl get node <node-name> -o wide
```
#### 2.3 配置文件损坏恢复

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
# 备份目录位置（升级前应存在）
ls -la /etc/kubernetes/backup-*/

# 从备份恢复控制面配置
BACKUP_DIR="/etc/kubernetes/backup-<timestamp>"

cp $BACKUP_DIR/kubeadm-config.yaml /etc/kubernetes/kubeadm-config.yaml
cp $BACKUP_DIR/kube-apiserver.yaml /etc/kubernetes/manifests/kube-apiserver.yaml
cp $BACKUP_DIR/kube-controller-manager.yaml /etc/kubernetes/manifests/kube-controller-manager.yaml
cp $BACKUP_DIR/kube-scheduler.yaml /etc/kubernetes/manifests/kube-scheduler.yaml

# 重启 kubelet 使配置生效
systemctl restart kubelet
```
### 第三阶段：etcd 降级保护

#### 3.1 etcd 版本兼容性检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看当前 etcd 版本
ETCDCTL_API=3 etcdctl version

# 查看 etcd 支持的降级范围
# etcd 官方规定：只能降级到上一个 minor 版本
# 例如：3.5.x → 3.4.x，但不能 3.5.x → 3.3.x

# 检查 etcd 数据格式版本
ETCDCTL_API=3 etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint status --write-out=json | jq '.[0].Status.db_version'
```
#### 3.2 etcd 升级失败回退

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ⚠️ etcd 降级是高风险操作，必须先创建备份

# 创建紧急备份
ETCDCTL_API=3 etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  snapshot save /tmp/etcd-pre-rollback.db

# 停止 etcd
systemctl stop etcd

# 安装旧版本 etcd
yum install -y etcd-<old-version> --allow-downgrade

# ⚠️ 如果数据格式已升级，需要从 snapshot 恢复
ETCDCTL_API=3 etcdctl snapshot restore \
  /tmp/etcd-pre-rollback.db \
  --data-dir=/var/lib/etcd \
  --name=etcd-1 \
  --initial-cluster=etcd-1=https://10.0.0.1:2380 \
  --initial-advertise-peer-urls=https://10.0.0.1:2380

chown -R etcd:etcd /var/lib/etcd
systemctl start etcd
```
#### 3.3 etcd 降级限制说明

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# etcd 版本降级矩阵（摘自官方文档）：
# 当前版本 → 可降级到
# 3.6.x   → 3.5.x（仅当数据格式版本未升级）
# 3.5.x   → 3.4.x
# 3.4.x   → 3.3.x
# 注意：如果已经执行了数据格式迁移，降级前必须从 snapshot 恢复

# 检查数据格式迁移状态
ETCDCTL_API=3 etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint status --write-out=json | jq '.[0].Status.storage_version'
```
### 第四阶段：API Server 兼容性问题处理

#### 4.1 API 弃用导致的启动失败

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 场景：新版 API Server 不再支持旧版 API 资源

# 查看 API Server 启动日志
journalctl -u kube-apiserver --no-pager | grep -iE "removed|deprecated|incompatible"

# 查看哪些资源使用了已弃用的 API
kubectl get <resource-type> -A -o yaml | grep "apiVersion:"

# 使用 kubectl-convert 将旧 API 资源转换为新版本
kubectl convert -f old-resource.yaml --output-version <new-api-version>
```
#### 4.2 API Server 启动参数不兼容

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
# 查看 API Server 静态 Pod 配置
cat /etc/kubernetes/manifests/kube-apiserver.yaml

# 对比新旧版本的参数差异
# 移除新版不支持的参数
# 添加新版必需的参数

# 常见不兼容参数：
# --experimental-encryption-provider-config → --encryption-provider-config
# --service-account-api-audiences → --api-audiences
# --requestheader-client-ca-file 路径变更

# 恢复旧版 API Server 配置
cp /etc/kubernetes/backup-*/kube-apiserver.yaml /etc/kubernetes/manifests/kube-apiserver.yaml
systemctl restart kubelet
```
#### 4.3 证书兼容性问题

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
# 检查证书过期时间
kubeadm certs check-expiration

# 检查证书 SAN 是否匹配
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -text | grep -A 1 "Subject Alternative Name"

# 如果证书不兼容，重新生成
kubeadm certs renew apiserver
kubeadm certs renew apiserver-kubelet-client

# 重启 API Server
systemctl restart kubelet
```
### 第五阶段：kubelet 版本回退

#### 5.1 kubelet 降级

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
# 查看当前 kubelet 版本
kubelet --version

# 安装旧版本 kubelet
yum install -y kubelet-<old-version> kubectl-<old-version> --allow-downgrade

# 重新加载配置并重启
systemctl daemon-reload
systemctl restart kubelet

# 验证版本
kubelet --version
kubectl get node <node-name> -o wide
```
#### 5.2 kubelet 配置回退

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
# 如果 kubelet 配置在升级过程中被修改

# 查看当前配置
cat /var/lib/kubelet/config.yaml

# 从备份恢复配置
cp /etc/kubernetes/backup-*/kubelet-config.yaml /var/lib/kubelet/config.yaml

# 或者从 kubeadm 重新生成
kubeadm init phase kubelet-config --node-name <node-name>

# 重启 kubelet
systemctl restart kubelet
```
#### 5.3 批量 kubelet 回退脚本

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
#!/bin/bash
# bulk-kubelet-rollback.sh
# 批量回退 kubelet 版本

NODES=("node-1" "node-2" "node-3")
OLD_VERSION="1.29.8"

for node in "${NODES[@]}"; do
  echo "=== Processing $node ==="

  # 检查 SSH 可达
  if ! ssh -o ConnectTimeout=5 root@$node "true" 2>/dev/null; then
    echo "  SSH unreachable, skipping"
    continue
  fi

  # 回退 kubelet 版本
  echo "  Installing kubelet $OLD_VERSION..."
  ssh root@$node "yum install -y kubelet-$OLD_VERSION kubectl-$OLD_VERSION --allow-downgrade"
  ssh root@$node "systemctl daemon-reload && systemctl restart kubelet"

  sleep 10

  # 验证版本
  VERSION=$(ssh root@$node "kubelet --version")
  STATUS=$(kubectl get node $node --no-headers 2>/dev/null | awk '{print $2}')
  echo "  kubelet version: $VERSION"
  echo "  Node status: $STATUS"
  echo ""
done
```
### 第六阶段：组件配置恢复

#### 6.1 控制面配置恢复

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
# 列出可用的备份
ls -la /etc/kubernetes/backup-*/

# 恢复所有控制面配置
BACKUP="/etc/kubernetes/backup-<timestamp>"

cp $BACKUP/kubeadm-config.yaml /etc/kubernetes/
cp $BACKUP/kube-apiserver.yaml /etc/kubernetes/manifests/
cp $BACKUP/kube-controller-manager.yaml /etc/kubernetes/manifests/
cp $BACKUP/kube-scheduler.yaml /etc/kubernetes/manifests/
cp $BACKUP/etcd.yaml /etc/kubernetes/manifests/

# 重启 kubelet 使配置生效
systemctl restart kubelet
```
#### 6.2 CNI 配置恢复

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看当前 CNI 配置
ls -la /etc/cni/net.d/

# 从备份恢复
cp /etc/kubernetes/backup-*/cni/* /etc/cni/net.d/

# 或重新部署 CNI DaemonSet
kubectl rollout restart daemonset -n kube-system -l k8s-app=calico-node
```
#### 6.3 kube-proxy 恢复

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 kube-proxy 状态
kubectl get pods -n kube-system -l k8s-app=kube-proxy

# 如果 kube-proxy 配置损坏，从 ConfigMap 恢复
kubectl get cm kube-proxy -n kube-system -o yaml > /tmp/kube-proxy-config.yaml
# 编辑修复后重新应用
kubectl apply -f /tmp/kube-proxy-config.yaml

# 重启所有 kube-proxy Pod
kubectl rollout restart daemonset kube-proxy -n kube-system
```
## 生产最佳实践

### 升级前准备

- **完整备份**：etcd snapshot + 控制面配置 + CNI 配置 + 证书
- **版本跳跃限制**：遵循 Kubernetes 官方的版本跳跃限制（最多升 1 个 minor 版本）
- **测试环境验证**：先在测试集群完成升级全流程
- **维护窗口**：选择业务低峰期执行

### 升级策略

- **逐节点升级**：一次只升级一个控制面节点，验证成功后再继续
- **滚动升级工作节点**：先 drain → 升级 → uncordon，逐批执行
- **版本锁定**：升级完成后锁定组件版本，防止意外升级

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# CentOS/RHEL 锁定版本
yum install -y yum-plugin-versionlock
yum versionlock kubelet kubectl kubeadm containerd.io

# Ubuntu 锁定版本
apt-mark hold kubelet kubectl kubeadm containerd
```
### 回滚预案

- 每次升级前编写详细的回滚步骤
- 准备好旧版本的 RPM/DEB 包
- etcd snapshot 必须在升级前创建并验证

## 故障排查

### 场景 1：kubeadm upgrade apply 卡住

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 API Server 是否响应
curl -k https://127.0.0.1:6443/healthz

# 检查 etcd 是否健康
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint health

# 如果 API Server 无响应，检查静态 Pod
crictl ps | grep kube-apiserver
# 或
docker ps | grep kube-apiserver
```
### 场景 2：升级后集群内部 DNS 解析失败

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 CoreDNS Pod 状态
kubectl get pods -n kube-system -l k8s-app=kube-dns

# 检查 CoreDNS 日志
kubectl logs -n kube-system -l k8s-app=kube-dns

# 如果 CoreDNS 配置损坏，重新部署
kubectl rollout restart deployment coredns -n kube-system
```
### 场景 3：升级后 Service 无法访问

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 kube-proxy 状态
kubectl get pods -n kube-system -l k8s-app=kube-proxy

# 检查 iptables 规则
iptables -t nat -L KUBE-SERVICES

# 重启 kube-proxy
kubectl rollout restart daemonset kube-proxy -n kube-system
```
### 场景 4：升级后 Pod 调度异常

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 Scheduler 日志
journalctl -u kube-scheduler --since "1 hour ago" | grep -i error

# 检查 Scheduler Leader 选举
kubectl get endpoints kube-scheduler -n kube-system

# 重启 Scheduler
# 通过修改静态 Pod 触发重启
touch /etc/kubernetes/manifests/kube-scheduler.yaml
```
## 参考链接

- [Kubernetes 官方文档 - 升级集群](https://kubernetes.io/docs/tasks/administer-cluster/cluster-management/#upgrading-a-cluster)
- [kubeadm 升级指南](https://kubernetes.io/docs/tasks/administer-cluster/kubeadm/kubeadm-upgrade/)
- [etcd 版本兼容性矩阵](https://etcd.io/docs/latest/upgrades/)
- [Kubernetes 版本偏差策略](https://kubernetes.io/releases/version-skew-policy/)
- [kubeadm 回滚操作](https://kubernetes.io/docs/reference/setup-tools/kubeadm/)

---

*本手册适用于 Kubernetes 1.28-1.32 版本。升级前请仔细阅读官方版本变更说明。*


<!-- risk-assessed -->
