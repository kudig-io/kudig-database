---
title: Kubean
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- etcd
- calico
- helm
- containerd
- job
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubean 是什么
- 如何 Kubean
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Kubean
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- cni-basics
- etcd-basics
---

title: Kubean
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- etcd
- calico
- helm
- containerd
- job
- crd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Kubean 是什么
- 如何 Kubean
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Kubean
- cncf
- landscape
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
# Kubean

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://kubean-io.github.io/kubean/ |
| **GitHub** | https://github.com/kubean-io/kubean |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Kubean 是一个基于 Kubespray 的 Kubernetes 集群生命周期管理 Operator。它将 Kubespray 的集群部署能力封装为 Kubernetes CRD，使用户可以通过声明式的方式在已有的 Kubernetes 集群（管理集群）上创建、升级和管理多个 Kubernetes 集群。Kubean 支持在线和离线部署，兼容多种 Linux 发行版和 CPU 架构。

### 核心特性

- **声明式管理**: 通过 CRD 声明式创建和管理 Kubernetes 集群
- **Kubespray 驱动**: 基于成熟的 Kubespray 项目，继承其广泛的兼容性
- **离线部署**: 内置离线包管理，支持完全离线环境下的集群部署
- **多集群管理**: 从一个管理集群管理多个工作集群的生命周期
- **集群升级**: 支持 Kubernetes 版本滚动升级
- **多架构**: 支持 AMD64 和 ARM64 架构

---

## 架构设计

```
┌──────────────────────────────────────────────────┐
│            管理集群 (Management Cluster)            │
│                                                    │
│  ┌──────────────────────────────────────────┐     │
│  │          Kubean Operator                  │     │
│  │  ┌──────────┐  ┌──────────────────────┐  │     │
│  │  │ Cluster  │  │ ClusterOperation     │  │     │
│  │  │Controller│  │ Controller           │  │     │
│  │  └────┬─────┘  └──────────┬───────────┘  │     │
│  └───────┼───────────────────┼───────────────┘     │
│          │                   │                      │
│  ┌───────▼───────────────────▼───────────────┐     │
│  │         Kubespray Job (Ansible)            │     │
│  │  (集群部署 / 升级 / 扩缩容 / 卸载)         │     │
│  └──────────────────┬────────────────────────┘     │
└─────────────────────┼──────────────────────────────┘
                      │ SSH / Ansible
         ┌────────────┼────────────┐
         │            │            │
    ┌────▼────┐  ┌────▼────┐  ┌───▼─────┐
    │ Node 1  │  │ Node 2  │  │ Node 3  │
    │ Master  │  │ Master  │  │ Worker  │
    │ etcd    │  │ etcd    │  │         │
    └─────────┘  └─────────┘  └─────────┘
         工作集群 (Workload Cluster)
```

---

## 快速开始

### 安装 Kubean Operator

```bash
helm repo add kubean https://kubean-io.github.io/kubean-helm-chart/
helm install kubean kubean/kubean \
  --namespace kubean-system \
  --create-namespace
```

### 定义集群

```yaml
# 主机清单
apiVersion: kubean.io/v1alpha1
kind: ClusterInventory
metadata:
  name: my-cluster-hosts
  namespace: kubean-system
spec:
  hostsConfRef:
    namespace: kubean-system
    name: my-cluster-hosts-conf

---
apiVersion: v1
kind: ConfigMap
metadata:
  name: my-cluster-hosts-conf
  namespace: kubean-system
data:
  hosts.yml: |
    all:
      hosts:
        master1:
          ansible_host: 192.168.1.10
          ip: 192.168.1.10
          ansible_user: root
        master2:
          ansible_host: 192.168.1.11
          ip: 192.168.1.11
          ansible_user: root
        worker1:
          ansible_host: 192.168.1.20
          ip: 192.168.1.20
          ansible_user: root
      children:
        kube_control_plane:
          hosts:
            master1:
            master2:
        kube_node:
          hosts:
            master1:
            master2:
            worker1:
        etcd:
          hosts:
            master1:
            master2:

---
# 集群配置
apiVersion: kubean.io/v1alpha1
kind: Cluster
metadata:
  name: my-cluster
spec:
  hostsConfRef:
    namespace: kubean-system
    name: my-cluster-hosts
  varsConfRef:
    namespace: kubean-system
    name: my-cluster-vars

---
apiVersion: v1
kind: ConfigMap
metadata:
  name: my-cluster-vars
  namespace: kubean-system
data:
  group_vars.yml: |
    kube_version: v1.28.0
    container_manager: containerd
    kube_network_plugin: calico
    kube_service_addresses: 10.233.0.0/18
    kube_pods_subnet: 10.233.64.0/18
    cluster_name: my-cluster
```

### 部署集群

```yaml
apiVersion: kubean.io/v1alpha1
kind: ClusterOperation
metadata:
  name: my-cluster-deploy
spec:
  cluster: my-cluster
  image: ghcr.io/kubean-io/spray-job:latest
  actionType: playbook
  action: cluster.yml
  preHook:
    - actionType: playbook
      action: ping.yml
  postHook:
    - actionType: playbook
      action: kubeconfig.yml
```

### 集群升级

```yaml
apiVersion: kubean.io/v1alpha1
kind: ClusterOperation
metadata:
  name: my-cluster-upgrade
spec:
  cluster: my-cluster
  image: ghcr.io/kubean-io/spray-job:latest
  actionType: playbook
  action: upgrade-cluster.yml
  extraArgs: "-e kube_version=v1.29.0"
```

---

## 与其他方案对比

| 特性 | Kubean | Kubespray (直接) | kubeadm | Cluster API |
|:---|:---|:---|:---|:---|
| 管理方式 | K8s CRD | Ansible CLI | CLI | K8s CRD |
| 底层工具 | Kubespray | Kubespray | kubeadm | Provider 适配 |
| 离线部署 | 内置支持 | 需自配 | 需自配 | 依赖 Provider |
| 多集群管理 | 原生 | 脚本 | 无 | 原生 |
| 学习曲线 | 中 | 高 (Ansible) | 低 | 高 |
| OS 兼容性 | 广泛 | 广泛 | 广泛 | 依赖 Provider |

---

## 最佳实践

1. **管理集群**: 使用独立的轻量级 K8s 集群作为管理集群
2. **离线镜像**: 预先准备离线包，确保在网络受限环境中可用
3. **SSH 密钥**: 使用 SSH 密钥认证而非密码，通过 Secret 管理私钥
4. **渐进升级**: 先升级 etcd，再升级控制平面，最后升级工作节点
5. **备份**: 升级前备份 etcd 数据，确保可以回滚

---

## 参考资源

- [Kubean 官方文档](https://kubean-io.github.io/kubean/)
- [Kubean GitHub](https://github.com/kubean-io/kubean)
- [Kubespray](https://kubespray.io/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/linux.md|linux]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/cncf-cicd|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/cluster-index|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
