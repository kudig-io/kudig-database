---
title: Domain-1 架构基础 — 开源项目索引
description: '# Domain-1 架构基础 — 开源项目索引'
summary: '# Domain-1 架构基础 — 开源项目索引'
category: architecture-fundamentals
tags:
- k8s
- architecture
- kubernetes
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 5min
intent_queries:
- Domain-1 架构基础 — 开源项目索引 是什么
- 如何 Domain-1 架构基础 — 开源项目索引
- Kubernetes 1 architecture fundamentals 最佳实践
trigger_keywords:
- Domain-1
- 架构基础
- 开源项目索引
- architecture
- fundamentals
prerequisites:
- kubectl-basics
- kubernetes-concepts
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../domain-13-container-runtime/
  label: '相关知识域: domain-13-container-runtime'
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/kubectl-scene-cheatsheet.md
  label: '速查卡: kubectl-scene-cheatsheet'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Domain-1 架构基础 — 开源项目索引

> **最后更新**: 2026-04-24  
> **适用版本**: Kubernetes v1.33

---

<!-- chunk: 核心项目 -->
## 核心项目

| 项目 | 作用 | CNCF 状态 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **Kubernetes** | 容器编排平台核心 | Graduated | v1.33.0 | 115k+ | Apache-2.0 |
| **Minikube** | 本地单节点 K8s | 非 CNCF | v1.35.0 | 30k+ | Apache-2.0 |
| **kind** | Docker 内 K8s 集群 | 非 CNCF | v0.27.0 | 14k+ | Apache-2.0 |
| **k3s** | 轻量级 K8s 发行版 | CNCF (Rancher) | v1.32.0 | 28k+ | Apache-2.0 |
| **k3d** | k3s in Docker | 非 CNCF | v5.8.0 | 6k+ | MIT |
| **kubeadm** | 官方集群安装工具 | K8s SIG | v1.33.0 | - | Apache-2.0 |
| **kube-proxy** | 集群网络代理 | K8s 核心 | v1.33.0 | - | Apache-2.0 |
| **KubeEdge** | 边缘 K8s 方案 | Graduated | v1.20.0 | 7k+ | Apache-2.0 |
| **OpenYurt** | 阿里云边缘扩展 | Incubating | v1.5.0 | 2k+ | Apache-2.0 |
| **SuperEdge** | 腾讯边缘容器 | 非 CNCF | v0.8.0 | 1k+ | Apache-2.0 |
| **MicroK8s** | Canonical 轻量 K8s | Canonical | v1.32.0 | 9k+ | Apache-2.0 |
| ** sealos** | 集群镜像与生命周期 | 非 CNCF | v5.0.0 | 12k+ | Apache-2.0 |
| **Kubespray** | Ansible K8s 部署 | K8s SIG | v2.27.0 | 16k+ | Apache-2.0 |
| **kube-vip** | 高可用虚拟 IP | 社区 | v0.8.0 | 2k+ | Apache-2.0 |
| **Node Problem Detector** | 节点故障检测 | K8s SIG | v0.8.20 | 3k+ | Apache-2.0 |
| **Cluster Proportional Autoscaler** | DNS 等比例伸缩 | K8s SIG | v1.9.0 | 1k+ | Apache-2.0 |

---

<!-- chunk: K8s 发行版选型 -->
## K8s 发行版选型

| 场景 | 推荐 | 说明 |
|:---|:---|:---|
| 本地开发 | kind / k3d / Minikube | 快速启动，轻量 |
| CI/CD 测试 | kind / k3d | 并行创建销毁 |
| 边缘/IoT | k3s / KubeEdge / OpenYurt | 资源占用低 |
| 生产小规模 | k3s / MicroK8s | 运维简单 |
| 生产大规模 | 官方 kubeadm / 云厂商托管 | 完全控制 |
| 裸金属 | kube-vip + kubeadm / Cluster API | 高可用方案 |

---

<!-- chunk: 参考链接 -->
## 参考链接

- [Kubernetes 官方文档](https://kubernetes.io/docs/)
- [Minikube 文档](https://minikube.sigs.k8s.io/docs/)
- [kind 文档](https://kind.sigs.k8s.io/)
- [k3s 文档](https://docs.k3s.io/)
- [KubeEdge 文档](https://kubeedge.io/docs/)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-01-cluster-fundamentals MOC
- [[domain-01-cluster-fundamentals/README.md|Domain-1: Kubernetes架构基础]]
- Kubernetes 架构全景图
- Kubernetes 核心组件深度剖析
- 03 - 功能和API表
- 04 - Kubernetes 源码结构深度解析
- kubectl 命令完整参考
- 06 - 集群配置参数完全参考
- 07 - 升级路径与策略指南
- 08 - 多租户架构设计 (Multi-Tenancy Architecture)
- 09 - 边缘计算集成架构 (KubeEdge/OpenYurt)
- 10 - Windows 容器支持与集成指南


<!-- risk-assessed -->
