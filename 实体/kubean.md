---
title: Kubean (entities)
description: '## 概述'
summary: 'Kubean 是一个基于 Kubespray 的 Kubernetes 集群生命周期管理 Operator。'
category: entities
tags:
- k8s
- cncf
- runtime
- kubean
- etcd
- containerd
- crd
- operator
- ebpf
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubean 是什么
- 如何 Kubean
trigger_keywords:
- Kubean
prerequisites:
- kubectl-basics
- ebpf-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Kubean

> **CNCF 状态**: Sandbox | **类别**: Platform/Runtime | **主要语言**: Go

## 概述

Kubean 是一个基于 Kubespray 的 Kubernetes 集群生命周期管理 Operator，由DaoCloud（道客）开发，2023 年加入 CNCF 沙箱。它将 Kubespray 的集群部署能力封装为 Kubernetes CRD，使用户可以通过声明式的方式在已有的 Kubernetes 集群（管理集群）上创建、升级和管理多个 Kubernetes 集群。Kubean 支持在线和离线部署，兼容多种 Linux 发行版（CentOS、Ubuntu、Rocky、麒麟等）和 CPU 架构（x86_64、ARM64）。它采用 Cluster API 兼容的生命周期管理理念，但底层使用经过大规模验证的 Kubespray（Ansible）作为执行引擎，降低了学习曲线和部署风险。

## 核心能力

- **声明式集群管理**: 通过 Cluster CRD 定义集群期望状态，Operator 自动执行部署/升级
- **基于 Kubespray**: 底层使用经过大规模验证的 Kubespray（Ansible）引擎
- **离线部署**: 预打包离线镜像和二进制，支持完全断网环境部署
- **多架构/多OS**: 支持 x86_64/ARM64 架构和 CentOS/Ubuntu/Rocky/麒麟等发行版
- **全生命周期**: 创建、扩缩容、升级、组件安装、卸载
- **ComponentManifest**: 版本化组件清单，管理 CNI/CSI/容器运行时版本

## 架构

Kubean 采用管理集群 + 目标集群架构：

- **Kubean Controller**: 部署在管理集群中的 Operator，监听 Cluster 和 ClusterOperation CRD
- **Cluster CRD**: 定义目标集群的期望状态（节点列表、K8s 版本、网络配置）
- **ClusterOperation CRD**: 触发具体操作（部署/升级/扩容/备份）
- **Job Runner**: Kubean 创建的 Ansible 执行 Pod，运行 Kubespray playbook
- **LocalArtifactSet CRD**: 离线包管理，定义节点上需要预置的镜像和二进制
- **ConfigMap**: SSH 凭据和节点配置

执行流程：`Cluster CRD → Controller → ClusterOperation → Job (Ansible) → 目标节点`

## K8s 集成

Kubean 的管理面是一个已有的 Kubernetes 集群。Cluster CRD 定义目标集群的节点和配置，ClusterOperation CRD 触发具体运维操作。Kubean Controller 监听这些 CRD，创建 Kubernetes Job 运行 Kubespray Ansible playbook。Job Pod 通过 SSH 连接到目标节点执行安装操作（部署 etcd、控制面、工作节点、CNI 等）。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的标准 CRD+Controller+Job 模式完全一致，可以利用 GitOps 管理 Cluster CRD。

## 生产场景

1. **多集群管理**: 在管理集群上统一创建和管理开发/测试/生产多个 Kubernetes 集群
2. **离线环境部署**: 在没有公网的机房环境中批量部署 Kubernetes 集群
3. **版本升级管理**: 声明式升级多个集群的 K8s 版本，自动执行 etcd 备份
4. **国产化部署**: 在麒麟 OS + 鲲鹏 ARM 架构上部署 Kubernetes

## 安装

```bash
# 安装 Kubean Operator
helm repo add kubean-io https://kubean-io.github.io/kubean-helm-chart/
helm install kubean kubean-io/kubean -n kubean-system --create-namespace

# 创建目标集群（3 master + 3 worker）
kubectl apply -f - <<EOF
apiVersion: kubean.io/v1alpha1
kind: Cluster
metadata:
  name: my-cluster
spec:
  hostsConf:
    confs:
    - address: 192.168.1.10
      port: 22
      user: root
      roles: [control_plane, etcd]
    - address: 192.168.1.11
      port: 22
      user: root
      roles: [control_plane, etcd]
    - address: 192.168.1.20
      port: 22
      user: root
      roles: [worker]
  sshAuthRef:
    name: my-ssh-secret
  kubeconfRef:
    name: my-kubeconf
  imageInfo:
    image: ghcr.io/kubean-io/spray-job:v2.23.0
  kubesprayVars:
    kube_version: v1.28.5
    container_runtime: containerd
    kube_network_plugin: calico
EOF

# 触发集群部署操作
kubectl apply -f - <<EOF
apiVersion: kubean.io/v1alpha1
kind: ClusterOperation
metadata:
  name: my-cluster-install
spec:
  cluster: my-cluster
  image: ghcr.io/kubean-io/spray-job:v2.23.0
  actionType: playbook
  action: cluster.yml
EOF

# 查看部署进度
kubectl logs job/my-cluster-install -n kubean-system
```

## 对比

| 特性 | Kubean | KubeClipper | Kubespray | Cluster API |
|------|--------|-------------|-----------|-------------|
| 声明式 | ✅ CRD | ✅ CRD | ❌ Ansible | ✅ CRD |
| 离线部署 | ✅ | ✅ | ⚠️ | ❌ |
| 管理面 | K8s Operator | 自研 | 无 | K8s Operator |
| CNCF 状态 | Sandbox | Sandbox | 非 CNCF | 非 CNCF |

## 架构定位

在 CNCF 生态中，Kubean 属于 **Platform/Runtime** 类别，为云原生应用提供基于 Kubespray 的集群生命周期管理能力。

## 参考链接

- [[etcd]]
- [[containerd]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[概念/controller-pattern.md|controller-pattern]]

## Related

- [[实体/k8s-advanced-ecosystem.md|k8s-advanced-ecosystem]] — 硬件知识体系、CNCF 全景生态与 eBPF 平台工程
- observability.md|cncf-observability]] — CNCF 可观测性项目全景
- [[chaos-mesh]] — Chaos Mesh
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kubean
- [[实体/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[生态参考/领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
