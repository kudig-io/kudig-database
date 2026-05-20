---
title: 节点升级 kubeadm upgrade node 源码分析
description: '## 概述'
category: functions
tags:
- k8s
- operations
- cluster-management
- etcd
- kubelet
- scheduler
- calico
- coredns
- daemonset
- ingress
last_updated: 2026-05-18
difficulty: advanced
reading_level: advanced
audience:
- Kubernetes 运维工程师
- 平台工程师
estimated_read_time: 5min
intent_queries:
- kubeadm upgrade node worker node upgrade
- Kubernetes node upgrade step by step
- kubelet upgrade version skew
- node OS upgrade drain uncordon
- rolling upgrade worker nodes
trigger_keywords:
- upgrade
- kubeadm
- kubelet
- node upgrade
- drain
- uncordon
- cordon
- version skew
- minor version
- patch version
- rolling upgrade
- upgrade node
- kubectl drain
related_domains:
- domain-3-control-plane
- domain-12-troubleshooting
related_topics:
- cluster-create/09-upgrade
- cluster-create/15-upgrade-advanced
- node-create/04-drain
---


# 节点升级 — kubeadm upgrade node 源码分析

## 概述

节点升级是 Kubernetes 集群版本管理中最重要的运维操作之一。Kubernetes 社区大约每三个月发布一个 minor 版本，每个版本都有约一年的维护支持期。保持集群版本的及时升级对于获取安全补丁、新功能和性能优化至关重要。

Kubernetes 的升级遵循严格的顺序：先升级控制面节点（API Server、Controller Manager、Scheduler、etcd），再升级工作节点。工作节点的升级包括 kubelet 二进制、kubeadm 配置文件和容器运行时组件的更新。升级过程中需要确保工作负载的连续性——通过 drain/uncordon 机制将 Pod 从待升级节点迁移到其他节点。

kubeadm 提供了 `kubeadm upgrade node` 命令来简化工作节点的升级过程。它自动处理配置文件更新、kubelet 服务重启等操作。本文档详细分析节点升级的完整流程、kubeadm 的源码实现、各组件的升级顺序以及常见故障的排查方法。

---

## 源码路径

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| kubeadm upgrade 命令 | `cmd/kubeadm/app/cmd/upgrade/` | 升级命令入口 |
| upgrade node 实现 | `cmd/kubeadm/app/cmd/upgrade/node.go` | 节点升级逻辑 |
| kubelet 升级 | `pkg/kubelet/` | kubelet 相关 |
| 配置更新 | `cmd/kubeadm/app/phases/kubelet/` | kubelet 配置更新 |
| 静态 Pod 更新 | `cmd/kubeadm/app/phases/controlplane/` | 控制面 manifest 更新 |

---

## 一、节点升级概述

### 1.1 升级类型

```
集群升级类型:
  ┌─────────────────────────────────────────────────────────────┐
  │  1. Minor 版本升级 (如 1.28 → 1.29)                         │
  │     - 新功能、API 变化、弃用警告                              │
  │     - 需要详细阅读 Release Notes                             │
  │     - 只能逐版本升级 (1.28 → 1.29，不能跳版本)              │
  ├─────────────────────────────────────────────────────────────┤
  │  2. Patch 版本升级 (如 1.28.0 → 1.28.3)                     │
  │     - Bug 修复、安全补丁                                     │
  │     - 无 API 变化                                            │
  │     - 风险较低                                               │
  ├─────────────────────────────────────────────────────────────┤
  │  3. kubelet 单独升级                                        │
  │     - 仅升级 kubelet 二进制                                  │
  │     - 不涉及控制面变更                                        │
  │     - 支持 ±1 个 minor 版本的偏差                            │
  ├─────────────────────────────────────────────────────────────┤
  │  4. OS/内核升级                                              │
  │     - 节点操作系统和内核更新                                  │
  │     - 需要重启节点                                           │
  │     - 不直接影响 Kubernetes 版本                              │
  └─────────────────────────────────────────────────────────────┘
```

### 1.2 版本兼容性规则

```bash
# kubelet 版本偏差:
# kubelet 版本可以比 API Server 旧最多 2 个 minor 版本
# 例如: API Server 1.29 → kubelet 可以是 1.29/1.28/1.27

# kubectl 版本偏差:
# kubectl 版本可以比 API Server 旧或新最多 1 个 minor 版本
# 例如: API Server 1.29 → kubectl 可以是 1.30/1.29/1.28
```

---

## 二、kubeadm upgrade node

### 2.1 升级前准备

```bash
# 1. 确认当前版本
kubectl version --short
kubeadm version
kubelet --version

# 2. 检查升级计划
kubeadm upgrade plan
# 辇出: 可以升级到的版本列表和组件版本

# 3. 查看节点状态
kubectl get nodes -o wide

# 4. 确认所有节点 Ready
kubectl get nodes
```

### 2.2 控制面节点升级

```bash
# 在第一个控制面节点上执行
# 1. 升级 kubeadm
apt-get update && apt-get install -y kubeadm=1.29.0-*

# 2. 验证 kubeadm 版本
kubeadm version

# 3. 执行升级
kubeadm upgrade apply v1.29.0

# 4. 升级 kubelet 和 kubectl
apt-get install -y kubelet=1.29.0-* kubectl=1.29.0-*

# 5. 重启 kubelet
systemctl restart kubelet
```

### 2.3 工作节点升级

```bash
# 在工作节点上执行
# 1. 升级 kubeadm
apt-get update && apt-get install -y kubeadm=1.29.0-*

# 2. 执行节点升级 (不含 --certificate-key)
kubeadm upgrade node

# 内部流程:
# a. 从 ConfigMap 读取最新 ClusterConfiguration
# b. 备份 /var/lib/kubelet/config.yaml
# c. 备份 /etc/kubernetes/kubelet.conf
# d. 更新 kubelet 配置文件
# e. 更新 kubeconfig 文件

# 3. 升级 kubelet 和 kubectl
apt-get install -y kubelet=1.29.0-* kubectl=1.29.0-*

# 4. 重启 kubelet
systemctl restart kubelet
```

### 2.4 kubeadm upgrade node 源码分析

```go
// cmd/kubeadm/app/cmd/upgrade/node.go
func NewCmdNode() *cobra.Command {
    // kubeadm upgrade node 命令
    // 执行以下阶段:
    // 1. preflight        - 预检
    // 2. config           - 读取配置
    // 3. kubelet-config   - 更新 kubelet 配置
    // 4. kubeconfig       - 更新 kubeconfig
}

func runNodeUpgrade(data *upgradeData) error {
    // 1. 从 kubeadm-config ConfigMap 获取最新配置
    // 2. 验证当前状态
    // 3. 执行各阶段:
    //    - 更新 /var/lib/kubelet/config.yaml
    //    - 更新 /etc/kubernetes/kubelet.conf
    //    - 重启 kubelet 服务
}
```

---

## 三、kubelet 二进制升级

### 3.1 apt (Debian/Ubuntu)

```bash
# 查看可升级版本
apt-cache policy kubelet
apt-cache madison kubelet

# 升级到指定版本
sudo apt-get update
sudo apt-get install -y kubelet=1.29.0-*

# 查看当前版本
kubelet --version
# Kubernetes v1.29.0

# 锁定版本（防止意外升级）
sudo apt-mark hold kubelet kubeadm kubectl
```

### 3.2 yum/dnf (RHEL/CentOS/Rocky)

```bash
# 查看可升级版本
yum list --showduplicates kubelet

# 升级到指定版本
sudo yum install -y kubelet-1.29.0 --disableexcludes=kubernetes

# 锁定版本
sudo yum versionlock add kubelet kubeadm kubectl
```

### 3.3 升级后验证

```bash
# 检查 kubelet 状态
systemctl status kubelet

# 检查 kubelet 版本
kubelet --version

# 检查节点状态
kubectl get nodes
# NAME      STATUS   ROLES           AGE    VERSION
# node-1    Ready    control-plane   100d   v1.29.0

# 检查 kubelet 日志
journalctl -u kubelet --no-pager -n 50
```

---

## 四、滚动升级策略

### 4.1 升级顺序

```
推荐升级顺序:
  ┌─────────────────────────────────────────────────────────────┐
  │  1. 升级 kubeadm (所有节点)                                  │
  │     └── apt-get install kubeadm=1.29.0-*                    │
  ├─────────────────────────────────────────────────────────────┤
  │  2. 升级主控制面节点                                          │
  │     └── kubeadm upgrade apply v1.29.0                       │
  │     └── apt-get install kubelet=1.29.0-* kubectl=1.29.0-*  │
  ├─────────────────────────────────────────────────────────────┤
  │  3. 升级其他控制面节点                                        │
  │     └── kubeadm upgrade node (在每个控制面节点)              │
  │     └── apt-get install kubelet=1.29.0-* kubectl=1.29.0-*  │
  ├─────────────────────────────────────────────────────────────┤
  │  4. 升级工作节点（可并行，建议逐个）                          │
  │     └── kubectl drain <node>                                 │
  │     └── kubeadm upgrade node                                 │
  │     └── apt-get install kubelet=1.29.0-*                    │
  │     └── kubectl uncordon <node>                              │
  ├─────────────────────────────────────────────────────────────┤
  │  5. 验证集群状态                                             │
  │     └── kubectl get nodes                                    │
  │     └── kubectl get pods --all-namespaces                    │
  ├─────────────────────────────────────────────────────────────┤
  │  6. 升级 CNI 插件                                            │
  │     └── kubectl apply -f calico-v3.27.yaml                   │
  ├─────────────────────────────────────────────────────────────┤
  │  7. 升级其他集群组件                                         │
  │     └── metrics-server, CoreDNS, Ingress Controller         │
  └─────────────────────────────────────────────────────────────┘
```

### 4.2 工作节点滚动升级脚本

```bash
#!/bin/bash
# 滚动升级工作节点

NODES=$(kubectl get nodes -l node-role.kubernetes.io/worker -o jsonpath='{.items[*].metadata.name}')
VERSION="1.29.0"

for NODE in $NODES; do
    echo "=== Upgrading node: $NODE ==="
    
    # 1. Drain 节点
    kubectl drain $NODE --ignore-daemonsets --delete-emptydir-data --timeout=300s
    
    # 2. 在节点上执行升级 (通过 SSH)
    ssh $NODE "apt-get update && apt-get install -y kubeadm=${VERSION}-*"
    ssh $NODE "kubeadm upgrade node"
    ssh $NODE "apt-get install -y kubelet=${VERSION}-*"
    ssh $NODE "systemctl restart kubelet"
    
    # 3. Uncordon 节点
    kubectl uncordon $NODE
    
    # 4. 等待节点就绪
    echo "Waiting for node $NODE to be ready..."
    while ! kubectl get nodes $NODE | grep -q " Ready"; do
        sleep 5
    done
    
    echo "=== Node $NODE upgraded successfully ==="
done
```

---

## 五、节点 OS 升级

### 5.1 OS 升级流程

```bash
# 1. Drain 节点
kubectl drain <node> --ignore-daemonsets --delete-emptydir-data

# 2. 升级 OS 包
sudo apt-get update && sudo apt-get upgrade -y

# 3. 如果需要升级内核
sudo apt-get install linux-image-5.15.0-xxx-generic
sudo reboot

# 4. 节点重启后，等待 kubelet 自动恢复
# kubelet 作为 systemd 服务会自动启动

# 5. 验证节点状态
kubectl get nodes
kubectl describe node <node> | grep -A 5 "Conditions"

# 6. Uncordon 节点
kubectl uncordon <node>
```

---

## 六、升级后验证

### 6.1 集群级验证

```bash
# 所有节点版本一致
kubectl get nodes -o wide

# 所有 Pod 运行正常
kubectl get pods --all-namespaces | grep -v Running

# 检查系统 Pod
kubectl get pods -n kube-system

# 检查 API Server 健康
kubectl get --raw /healthz
kubectl get --raw /livez
kubectl get --raw /readyz

# 检查 etcd 健康
kubectl get --raw /healthz?verbose

# 检查组件状态
kubectl get componentstatuses
```

### 6.2 节点级验证

```bash
# 检查 kubelet 版本
kubelet --version

# 检查 kubelet 日志
journalctl -u kubelet --no-pager -n 50

# 检查节点资源
kubectl top node <node>

# 检查节点 Conditions
kubectl describe node <node> | grep -A 10 "Conditions"

# 检查容器运行时
crictl info
crictl ps
```

---

## 七、回滚（不推荐）

```bash
# 降级 kubelet（仅限紧急情况）
sudo apt-get install kubelet=1.28.0-*
sudo systemctl restart kubelet

# 降级 kubeadm
sudo apt-get install kubeadm=1.28.0-*

# 注意:
# - 降级可能导致 API 兼容性问题
# - etcd 数据格式可能不兼容
# - 仅在升级失败且无法前进时使用
# - 建议从 etcd 备份恢复而非降级
```

---

## 八、常见错误与排查

| 错误 | 原因 | 排查命令 | 解决方案 |
|------|------|---------|---------|
| kubelet 启动失败 | cgroup driver 不匹配 | `journalctl -u kubelet -p err` | 检查 `/var/lib/kubelet/config.yaml` 中 cgroupDriver |
| 证书续期失败 | API Server 连接失败 | `curl -k https://<api-server>:6443/healthz` | 检查 API Server 是否就绪 |
| 镜像拉取失败 | 权限或网络问题 | `crictl pull <image>` | 检查 imagePullSecrets 和网络 |
| 节点 NotReady | kubelet 版本与 API Server 偏差过大 | `kubectl get nodes -o wide` | 确保版本偏差 ≤ 2 个 minor 版本 |
| `kubeadm upgrade` 卡住 | 静态 Pod 未就绪 | `crictl ps` | 检查控制面容器状态 |
| ConfigMap 不存在 | kubeadm-config 被误删 | `kubectl get configmap -n kube-system` | 从备份恢复 ConfigMap |
| 升级后 Pod CrashLoopBackOff | API 版本变更 | `kubectl logs <pod>` | 更新应用 YAML 中的 API 版本 |

---

## 相关函数

| 函数/组件 | 源码位置 | 说明 |
|----------|---------|------|
| `NewCmdApply` | `cmd/kubeadm/app/cmd/upgrade/apply.go` | `upgrade apply` 命令 |
| `NewCmdNode` | `cmd/kubeadm/app/cmd/upgrade/node.go` | `upgrade node` 命令 |
| `runNodeUpgrade` | `cmd/kubeadm/app/cmd/upgrade/node.go` | 节点升级逻辑 |
| `performUpgrade` | `cmd/kubeadm/app/cmd/upgrade/apply.go` | 执行升级 |
| `WriteKubeletConfig` | `cmd/kubeadm/app/phases/kubelet/` | 写入 kubelet 配置 |
| `UpdateKubeletConfig` | `cmd/kubeadm/app/phases/kubelet/` | 更新 kubelet 配置 |
