---
title: Flatcar Container Linux (entities)
description: '## 概述'
summary: 'Flatcar Container Linux 是为容器优化的不可变 Linux 发行版，是 CoreOS Container Linux 的延续和替代品。它提供最小化、自动更新、安全的容器运行环境。'
category: entities
tags:
- k8s
- cncf
- runtime
- flatcar
- etcd
- containerd
- crd
- operator
- serverless
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Flatcar Container Linux 是什么
- 如何 Flatcar Container Linux
trigger_keywords:
- Flatcar
- Container
- Linux
prerequisites:
- kubectl-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Flatcar Container Linux

> **CNCF 状态**: Incubating | **类别**: Runtime | **主要语言**: Shell, Go

## 概述

Flatcar Container Linux 是为容器优化的不可变 Linux 发行版，是 CoreOS Container Linux 的延续和替代品（CoreOS 被 Red Hat 收购后停止维护）。Flatcar 由 Kinvolk（现 Microsoft）维护，2019 年加入 CNCF Sandbox，后晋升为 Incubating。它提供最小化、自动更新、安全的容器运行环境，是运行 Kubernetes 节点操作系统的理想选择之一。

## 核心特性

- **不可变基础设施**: 只读根文件系统（/usr），配置通过 Ignition 声明式管理
- **自动更新**: 内置 A/B 分区原子更新机制，回滚只需重启
- **最小化设计**: 只包含运行容器必需的组件，无包管理器
- **安全加固**: SELinux、只读 rootfs、自动安全补丁、内核模块签名
- **多平台支持**: AWS、Azure、GCP、VMware、裸金属、Equnix Metal
- **CoreOS 兼容**: 完全兼容 CoreOS Container Linux 的使用模式

## 架构

Flatcar 采用不可变 OS 设计理念。系统分区以只读方式挂载（/usr 为只读），用户配置和数据存储在 /etc 和 /var 中。Ignition（替代 cloud-init）在首次启动时从 JSON 配置（user-data）中配置用户、网络、systemd 服务和文件。自动更新使用 update-engine（后台检查更新）和 locksmith（协调重启），采用 A/B 分区方案实现原子更新——新系统写入备用分区，重启时切换。所有更新通过 Omaha 协议从 Flatcar 更新服务器拉取。

## Kubernetes 集成

Flatcar 是运行 Kubernetes 节点的理想 OS。只读 rootfs 消除了操作系统层面的配置漂移。Ignition 配置文件声明式定义节点初始化（网络、Docker/containerd、kubelet 参数）。自动更新确保安全补丁及时安装，配合 Kured 协调节点重启。容器运行时（containerd）预装或通过 Ignition 安装。在裸金属集群中，Flatcar + Ignition + Matchbox 实现完全自动化的 PXE 部署。

## 生产使用场景

1. **裸金属 Kubernetes**: 在自建数据中心使用 Flatcar 作为节点 OS，实现不可变基础设施
2. **安全合规**: 自动安全更新和只读 rootfs 满足等保和 SOC2 合规要求
3. **大规模部署**: 通过 Ignition + Matchbox 实现 PXE 批量部署
4. **边缘 IoT**: 在资源受限的边缘设备上运行轻量级容器

## 安装

```bash
# Ignition 配置示例（config.ign）
{
  "ignition": { "version": "3.3.0" },
  "systemd": { "units": [{ "name": "docker.service", "enabled": true }] },
  "passwd": { "users": [{ "name": "core", "sshAuthorizedKeys": ["ssh-ed25519 AAA..."] }] }
}
# 在云平台使用 Flatcar 镜像并传入 Ignition 配置作为 user-data
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Flatcar** | CoreOS 延续、自动更新 | 社区较小 |
| Talos Linux | API 驱动、K8s 专属 | 新项目、生态小 |
| Bottlerocket | AWS 支持、自动更新 | AWS 生态绑定 |
| Ubuntu + kubeadm | 最广泛使用 | 需手动维护和加固 |

## 架构定位

在 CNCF 生态中，Flatcar 属于 **Runtime / Container OS** 类别，是容器专用操作系统的代表性项目。它与 Kubernetes、containerd、Kured 等项目深度协同。

## 参考链接

- [[etcd]]
- [[containerd]]
- [[概念/container-runtime-comparison.md|container-runtime-comparison]]
- [[概念/storage-model.md|storage-model]]
- [[pod-lifecycle]]

## Related

- [[serverless-devs]] — [[实体/serverless-devs.md|Serverless Devs]]
- [[sermant]] — Sermant
- [[loxilb]] — LoxiLB
- [[kube-ovn]] — Kube-OVN
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- flatcar
- [[实体/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/node-index.md|Node 知识图谱索引]]


<!-- risk-assessed -->
