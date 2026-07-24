---
title: kubeadm 升级完整路径指南（含 rollback）
description: 'title: kubeadm 升级完整路径指南（含 rollback）'
summary: 'title: kubeadm 升级完整路径指南（含 rollback）'
category: general
tags:
- k8s
- control-plane
- deep-dive
- upgrade
- guide
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 32-kubeadm-upgrade-complete-guide的完整指南
- 如何全面掌握32-kubeadm-upgrade-complete-guide？
- 32-kubeadm-upgrade-complete-guide的从入门到精通
trigger_keywords:
- kubeadm
- 升级完整路径指南
- rollback
- cluster
- fundamentals
prerequisites:
- kubectl-basics
- kubernetes-concepts
- helm-basics
- prometheus-basics
- etcd-basics
- tls-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: kubeadm 升级完整路径指南（含 rollback）
description: '# kubeadm 升级完整路径指南（含 rollback）'
category: control-plane
tags:
- k8s
- control-plane
- [[etcd|etcd]]
- apiserver
- scheduler
- controller-manager
- [[kubelet|kubelet]]
- [[Prometheus|prometheus]]
- [[CoreDNS|coredns]]
- helm
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- kubeadm 升级完整路径指南（含 rollback） 是什么
- 如何 kubeadm 升级完整路径指南（含 rollback）
- Kubernetes 3 control plane 最佳实践
trigger_keywords:
- kubeadm
- 升级完整路径指南
- rollback
- control
- plane
cross_refs:
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../工作负载/
  label: '相关知识域: 工作负载'
- type: domain
  path: ../网络/
  label: '相关知识域: 网络'
- type: domain
  path: ../存储/
  label: '相关知识域: 存储'
- type: domain
  path: ../安全/
  label: '相关知识域: 安全'
- type: fta
  path: ../故障诊断/FTA故障树/list/kubeadm-fta.md
  label: '故障树: kubeadm'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/kubectl-scene-cheatsheet.md
  label: '速查卡: kubectl-scene-cheatsheet'
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# kubeadm 升级完整路径指南（含 rollback）

> **文档类型**: 升级操作手册 | **适用版本**: K8s 1.28 → 1.33 | **最后更新**: 2026-05
> **使用场景**: Agent 生成"如何将 K8s 集群从 X 版本升级到 Y 版本"的完整操作步骤

---

<!-- chunk: 1. 升级路径规则与前置条件 -->
## 1. 升级路径规则与前置条件

### 1.1 版本兼容性规则

Kubernetes **最多跳过 1 个次版本**（e.g., 1.27 → 1.28 → 1.29 可行，1.26 → 1.28 不可行）。

| 当前版本 | 可升级到的目标版本 |
|---------|-----------------|
| 1.27.x | 1.28.x, 1.29.x |
| 1.28.x | 1.29.x, 1.30.x |
| 1.29.x | 1.30.x, 1.31.x |
| 1.30.x | 1.31.x, 1.32.x |
| 1.31.x | 1.32.x, 1.33.x |
| 1.32.x | 1.33.x |
| 1.33.x | （无更高版本） |

> **注意**：强烈建议逐次升级（每次升 1 个次版本），不要跳级。如 1.28 → 1.30，应先 1.28 → 1.29，再 1.29 → 1.30。

### 1.2 升级前检查清单

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看当前集群版本
kubectl get nodes -o wide

# 2. 查看所有 Pod 状态（确保无异常 Pod）
kubectl get pods --all-namespaces | grep -v Running | grep -v Completed

# 3. 检查 etcd 健康状态
kubectl get pods -n kube-system -l component=etcd
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key endpoint health

# 4. 检查 API Server 证书过期时间
kubeadm alpha certs check-expiration

# 5. 备份 etcd 数据（重要！）
ETCDCTL_API=3 etcdctl snapshot save /backup/etcd-$(date +%Y%m%d).db
ETCDCTL_API=3 etcdctl snapshot status /backup/etcd-$(date +%Y%m%d).db --write-out=table

# 6. 确认 /var/lib/etcd 所在磁盘有足够空间（至少 2x 当前数据大小）
df -h /var/lib/etcd

# 7. 确认控制平面节点有至少 2GB 空闲内存
free -h

# 8. 确认 container runtime 版本兼容目标 K8s 版本
containerd --version
crictl version
```
---

<!-- chunk: 2. 升级计划与版本预览 -->
## 2. 升级计划与版本预览

### 2.1 生成升级计划

```bash
# 查看可升级版本（需要访问 K8s 官方镜像仓库）
kubeadm upgrade plan

# 输出示例：
# Upgrade plan applicable for 1.28.0 -> 1.29.0
# 
# Components that must be upgraded manually after rolling upgrade:
# kubeadm has been upgraded to v1.29.0. You must manually downgrade/upgrade
# the kubeadm binary on all control plane nodes using the tool.
#
# The following Kubernetes version will be upgraded:
# kubeadm version: v1.29.0
# kube-apiserver: 1.28.x -> 1.29.x
# kube-controller-manager: 1.28.x -> 1.29.x
# kube-scheduler: 1.28.x -> 1.29.x
# kube-proxy: 1.28.x -> 1.29.x
# CoreDNS: will be upgraded to the version equivalent to your kubernetes version
# etcd: 3.5.x -> 3.6.x
```

### 2.2 查看具体升级内容

```bash
# 升级计划输出会包含以下信息：
# 1. 可升级版本列表
# 2. 每个组件的当前版本和目标版本
# 3. 哪些组件需要手动干预
# 4. 升级顺序说明
```

---

<!-- chunk: 3. 控制平面组件升级（主控制平面节点） -->
## 3. 控制平面组件升级（主控制平面节点）

### 3.1 升级顺序

```
重要：升级顺序必须严格遵守
1. 升级 kubeadm 版本（主控制平面节点）
2. 备份 etcd（所有控制平面节点）
3. 升级 etcd（所有控制平面节点）
4. 升级 kube-apiserver（主控制平面节点）
5. 升级 kube-controller-manager（主控制平面节点）
6. 升级 kube-scheduler（主控制平面节点）
7. 升级 kube-proxy（所有控制平面节点）
8. 升级 kubelet（主控制平面节点）
9. 验证 API Server 健康
10. 升级其他控制平面节点
```

### 3.2 步骤 1: 升级 kubeadm 版本

```bash
# 在所有控制平面节点执行（使用 root/sudo）
# 确认当前 kubeadm 版本
kubeadm version

# Debian/Ubuntu
apt-get update && apt-get install -y kubeadm=1.XX.Y-1*

# RHEL/CentOS
yum install -y kubeadm-1.XX.Y-1.el7

# 确认安装的版本
kubeadm version
```

### 3.3 步骤 2: 升级 kube-apiserver

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 在主控制平面节点执行
# 查看当前的静态 Pod manifest
ls /etc/kubernetes/manifests/

# 查看当前 API Server 镜像版本
kubectl get pods -n kube-system kube-apiserver-<node-name> -o jsonpath='{.spec.containers[0].image}'

# 升级控制平面组件（应用新版本镜像）
kubeadm upgrade apply v1.XX.Y

# 如果是多控制平面主节点，添加 --etcd-upgrade=false（如果 etcd 已手动升级）
kubeadm upgrade apply v1.XX.Y --etcd-upgrade=false --certificate-renewal=false
```
### 3.4 步骤 3: 验证 API Server

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 验证 API Server 已重启成功
kubectl get pods -n kube-system kube-apiserver-<node-name>
# 确认 Running 状态且 Restart Count 正常

# 验证 API Server 健康
curl -k https://localhost:6443/healthz
# 期望输出: {"healthz":"ok"}

# 验证集群版本已更新
kubectl get nodes -o wide
# 确认 Master 列显示新版本
```
### 3.5 步骤 4: 升级 kube-controller-manager 和 kube-scheduler

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
# kubeadm upgrade apply 已自动升级 controller-manager 和 scheduler
# 如需手动刷新配置：
# 1. 编辑静态 Pod manifest 更新镜像版本
# 2. systemctl restart kubelet
# 3. 验证
kubectl get pods -n kube-system kube-controller-manager-<node-name>
kubectl get pods -n kube-system kube-scheduler-<node-name>
```
### 3.6 步骤 5: 升级 kubelet

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
# 在控制平面节点执行
# 升级 kubelet 包
apt-get install -y kubelet=1.XX.Y-1*  # Debian/Ubuntu
# 或
yum install -y kubelet-1.XX.Y-1.el7  # RHEL/CentOS

# 重启 kubelet
systemctl daemon-reload
systemctl restart kubelet

# 检查 kubelet 状态
systemctl status kubelet

# 验证节点状态
kubectl get nodes -o wide
# 确认节点 Ready 且版本已更新
```
---

<!-- chunk: 4. 控制平面组件升级（其他控制平面节点） -->
## 4. 控制平面组件升级（其他控制平面节点）

### 4.1 单节点依次升级

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
# SSH 到第二个控制平面节点（node-2）

# 1. 升级 kubeadm
apt-get install -y kubeadm=1.XX.Y-1*

# 2. 上传证书（从主节点获取）
# 在主节点执行：
kubeadm init phase upload-certs --upload-certs

# 在 node-2 执行（使用新 token 和 certificate-key）：
kubeadm join <api-server-ip>:6443 \
  --control-plane \
  --token <token> \
  --discovery-token-ca-cert-hash sha256:<hash> \
  --control-plane \
  --certificate-key <certificate-key>

# 3. 升级 kubelet
apt-get install -y kubelet=1.XX.Y-1*
systemctl daemon-reload
systemctl restart kubelet

# 4. 验证
kubectl get nodes -o wide
```
### 4.2 控制平面组件版本验证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 在所有控制平面节点执行以下命令确认组件版本
# 手动检查每个组件的镜像版本
kubectl get pods -n kube-system -l component=kube-apiserver -o jsonpath='{.items[*].spec.containers[0].image}'
kubectl get pods -n kube-system -l component=kube-controller-manager -o jsonpath='{.items[*].spec.containers[0].image}'
kubectl get pods -n kube-system -l component=kube-scheduler -o jsonpath='{.items[*].spec.containers[0].image}'
kubectl get pods -n kube-system -l k8s-app=kube-proxy -o jsonpath='{.items[*].spec.containers[0].image}'
```
---

<!-- chunk: 5. 升级 worker 节点 -->
## 5. 升级 worker 节点

### 5.1 升级 worker 节点顺序

每个 worker 节点依次升级，不能同时升级所有 worker 节点（避免服务中断）。

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响
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
# 升级 worker 节点（逐节点执行）

# 1. 升级 kubeadm
apt-get install -y kubeadm=1.XX.Y-1*

# 2. 驱逐节点上的 Pod（维护模式）
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data --wait-for-grace-period=30

# 3. 升级 kubelet
apt-get install -y kubelet=1.XX.Y-1*
systemctl daemon-reload
systemctl restart kubelet

# 4. 验证节点状态
kubectl get nodes -o wide
# 确认 Ready

# 5. 解除 cordon（自动解除，如需手动）
# kubectl uncordon <node-name>
```
### 5.2 升级多个 worker 节点（滚动）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 获取所有 worker 节点列表
kubectl get nodes -l node-role.kubernetes.io/worker

# 逐个节点升级（每个节点按 5.1 步骤执行）
# 建议一次只升级 1-2 个节点，等待 Pod 重新调度完成后再继续

# 监控 Pod 调度情况
kubectl get pods --all-namespaces -o wide -w
```
---

<!-- chunk: 6. 升级后验证 -->
## 6. 升级后验证

### 6.1 完整验证清单

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 检查所有节点版本
kubectl get nodes -o wide

# 2. 检查所有 Pod 状态
kubectl get pods --all-namespaces | grep -v Running | grep -v Completed

# 3. 检查 etcd 健康（每个控制平面节点）
for node in node-1 node-2 node-3; do
  echo "=== $node ==="
  ETCDCTL_API=3 etcdctl --endpoints=https://$node:2379 --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
    --key=/etc/kubernetes/pki/etcd/healthcheck-client.key endpoint health
done

# 4. 检查 API Server 健康
curl -sk https://localhost:6443/healthz
curl -sk https://localhost:6443/api/v1/namespaces

# 5. 检查控制平面组件日志
kubectl logs -n kube-system kube-apiserver-<node-name> --tail=20
kubectl logs -n kube-system kube-controller-manager-<node-name> --tail=20
kubectl logs -n kube-system kube-scheduler-<node-name> --tail=20

# 6. 检查 CoreDNS（升级后 CoreDNS 会自动升级）
kubectl get deployment -n kube-system coredns -o jsonpath='{.spec.template.spec.containers[0].image}'

# 7. 检查 kube-proxy
kubectl get daemonset -n kube-system kube-proxy -o jsonpath='{.spec.template.spec.containers[0].image}'

# 8. 测试创建 Pod（基本功能测试）
kubectl run test-pod --image=nginx:1.25 --restart=Never
kubectl get pod test-pod
kubectl delete pod test-pod

# 9. 检查证书过期时间
kubeadm alpha certs check-expiration
```
### 6.2 验证命令输出参考

```bash
# 期望的节点版本输出
NAME     STATUS   ROLES           AGE   VERSION   INTERNAL-IP
node-1   Ready    control-plane   1y    v1.29.0   192.168.1.10
node-2   Ready    control-plane   1y    v1.29.0   192.168.1.11
node-3   Ready    control-plane   1y    v1.29.0   192.168.1.12
node-4   Ready    worker          300d  v1.29.0   192.168.1.20

# 期望的 etcd 健康输出（每个节点）
https://127.0.0.1:2379 is healthy: true

# 期望的 API Server 健康检查
{"healthz":"ok"}
```

---

<!-- chunk: 7. 升级失败回滚 -->
## 7. 升级失败回滚

### 7.1 回滚场景判断

| 场景 | 判定 | 回滚方案 |
|------|------|---------|
| API Server 无法启动 | `kubeadm upgrade apply` 失败 | 手动回滚到旧版本 |
| etcd 数据损坏 | etcd 日志报错 + 集群不可用 | 从快照恢复 etcd |
| 控制平面组件启动失败 | Pod 卡在 CrashLoopBackOff | 恢复旧镜像 |
| 单个组件升级失败 | kubectl logs 显示具体错误 | 重启组件或恢复旧镜像 |

### 7.2 API Server 回滚步骤

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
# 1. 停止 kubelet
systemctl stop kubelet

# 2. 编辑 API Server 静态 Pod manifest，将镜像改回旧版本
vi /etc/kubernetes/manifests/kube-apiserver.yaml
# 修改 image 字段，例如从 k8s.gcr.io/kube-apiserver:v1.29.0 改为 v1.28.0

# 3. 重启 kubelet
systemctl start kubelet

# 4. 检查 API Server 是否恢复
sleep 30
curl -sk https://localhost:6443/healthz
kubectl get pods -n kube-system kube-apiserver-<node-name>
```
### 7.3 etcd 回滚步骤

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `etcdctl snapshot restore`：用快照覆盖 etcd 数据目录，集群状态强制回退
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 停止所有控制平面节点上的 etcd
systemctl stop etcd

# 2. 在所有 etcd 节点上从快照恢复
ETCDCTL_API=3 etcdctl snapshot restore /backup/etcd-20260518.db \
  --name=<node-name> \
  --initial-cluster=<cluster-init> \
  --initial-cluster-token=<token> \
  --initial-advertise-peer-urls=https://<node-ip>:2380 \
  --data-dir=/var/lib/etcd

# 3. 重启 etcd
systemctl start etcd

# 4. 验证 etcd 健康
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 endpoint health

# 5. 验证 API Server 能连接 etcd
curl -sk https://localhost:6443/healthz
```
### 7.4 kubelet 版本回滚

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
# 在出问题的节点上
# 1. 降级 kubelet 包
apt-get install -y kubelet=1.XX.Y-1*  # 回退到之前版本

# 或
yum install -y kubelet-1.XX.Y-1.el7

# 2. 重启 kubelet
systemctl restart kubelet

# 3. 检查节点状态
kubectl get nodes -o wide
```
---

<!-- chunk: 8. 特殊场景 -->
## 8. 特殊场景

### 8.1 跳过 Pre-flight 检查

```bash
# 如需跳过版本检查（不推荐）
kubeadm upgrade apply v1.XX.Y --allow-release-missing

# 如需跳过已过期证书检查
kubeadm upgrade apply v1.XX.Y --certificate-renewal=false
```

### 8.2 离线升级（无公网镜像）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 在有网络的机器上下载所需镜像
kubeadm config images pull --kubernetes-version=v1.XX.Y

# 2. 保存镜像到文件
docker save $(kubeadm config images list --kubernetes-version=v1.XX.Y) -o /tmp/k8s-images.tar

# 3. 传输到目标机器
scp /tmp/k8s-images.tar <node>:/tmp/

# 4. 在目标机器加载镜像
docker load -i /tmp/k8s-images.tar

# 5. 执行升级（使用 --image-repository 指向内网仓库）
kubeadm upgrade apply v1.XX.Y --image-repository=internal-registry.example.com
```
### 8.3 etcd 单独升级（不需要滚动升级）

```bash
# etcd 可以单独升级，与 API Server 版本独立
# 但需要注意兼容性：K8s 1.29 要求 etcd 3.5.9+

# 查看当前 etcd 版本
etcd --version

# 升级 etcd（手动替换二进制并重启）
# 注意：etcd 数据格式在 major version 之间不可降级
```

### 8.4 升级时配置变化（K8s 1.28 → 1.29）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1.29 中 kube-proxy 默认使用 iptables 还是 ipvs？
# 查看当前模式：
kubectl get configmap -n kube-system kube-proxy -o yaml | grep mode

# 1.29 中调度器默认行为可能有变化
# 查看调度器配置：
kubectl get configmap -n kube-system kube-scheduler -o yaml
```
---

<!-- chunk: 9. 升级完成后的收尾工作 -->
## 9. 升级完成后的收尾工作

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器
> - `helm upgrade/install`：部署/升级 release

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
# 1. 更新 kubectl 客户端（如使用外部 kubectl）
apt-get install -y kubectl=1.XX.Y-1*  # Debian/Ubuntu

# 2. 更新 kubeadm 配置（移除过时的配置项）
kubeadm config migrate --old-config /etc/kubernetes/kubeadm.conf --new-config /etc/kubernetes/kubeadm.conf.new

# 3. 证书自动续期（已在 upgrade apply 中完成）
# 如需手动触发：
kubeadm alpha certs renew all
systemctl restart kubelet

# 4. 清理旧的 Docker 镜像（释放磁盘空间）
docker image prune -a

# 5. 更新集群内其他工具版本（根据需要）
# 如 Helm, Prometheus Operator, cert-manager 等
helm repo update
helm upgrade <release> <chart> --namespace <namespace>
```
---

<!-- chunk: 附录：关键命令速查 -->
## 附录：关键命令速查

| 操作 | 命令 |
|------|------|
| 查看当前版本 | `kubectl get nodes -o wide` |
| 升级计划 | `kubeadm upgrade plan` |
| 升级控制平面 | `kubeadm upgrade apply v1.XX.Y` |
| 升级单节点 | `kubeadm upgrade node` |
| 升级 kubelet | `apt-get install kubelet=1.XX.Y-1*` |
| 备份 etcd | `etcdctl snapshot save /backup/etcd.db` |
| 恢复 etcd | `etcdctl snapshot restore /backup/etcd.db` |
| 驱逐节点 | `kubectl drain <node> --ignore-daemonsets --delete-emptydir-data` |
| 验证证书 | `kubeadm alpha certs check-expiration` |
| 续期证书 | `kubeadm alpha certs renew all` |
| 生成 join 命令 | `kubeadm token create --print-join-command` |

---

```yaml
---
id: KUBEADM-UPGRADE-GUIDE-001
domain: control-plane
type: operation-guide
tags: [kubeadm, upgrade, rollback, cluster-lifecycle, k8s-1.28-1.33, agent-corpus]
intent_queries:
  - "如何升级 Kubernetes 集群版本"
  - "kubeadm upgrade 失败怎么回滚"
  - "K8s 1.28 升级到 1.29 步骤"
  - "etcd 备份和恢复命令"
  - "控制平面升级顺序"
difficulty: advanced
target_roles: [sre, ops-engineer]
k8s_versions: ["1.28", "1.29", "1.30", "1.31", "1.32", "1.33"]
related:
  - 集群基础/32-kubeadm-cluster-bootstrap.md
  - 故障诊断/FTA故障树/list/etcd-fta.md
  - 故障诊断/FTA故障树/list/kubeadm-fta.md
---
```

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 集群基础 MOC
- [[集群基础/README.md|Domain-3: Kubernetes控制平面]]
- Domain-3 控制平面 — 开源项目索引
- Kubernetes 控制平面架构总览 (Control Plane Architecture Overview)
- 控制平面组件交互详解 (Control Plane Components Interaction Deep Dive)
- 控制平面高可用部署模式 (Control Plane High Availability Deployment Patt...
- 控制平面安全加固指南 (Control Plane Security Hardening Guide)
- 控制平面监控与可观测性 (Control Plane Monitoring & Observability)
- 控制平面故障排查手册 (Control Plane Troubleshooting Handbook)
- 控制平面升级与迁移策略 (Control Plane Upgrade & Migration Strategy)
- 控制平面性能基准测试 (Control Plane Performance Benchmarking)
- 控制平面扩缩容指南 (Control Plane Scalability Guide)

## Related

- [[deep-dive|#deep-dive Hub]] — tag hub

- 12-demo-env-guide
- 21-platform-selection-guide

## See Also

- 31-kubectl-complete-reference
- 32-kubeadm-cluster-lifecycle
- 33-kubelet-eviction-thresholds
- final-completion-check

```

<!-- risk-assessed -->
