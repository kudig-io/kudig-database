---
title: Kubernetes 版本演进
description: '| 标准化 | v1.14 - v1.17 | kubectl 成熟、kubeadm GA、拓扑感知调度 |'
category: concepts
tags:
- k8s
- release-notes
- version-history
- kubernetes
- etcd
- kubelet
- scheduler
- coredns
- docker
- hpa
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 版本演进 是什么
- 如何 Kubernetes 版本演进
trigger_keywords:
- Kubernetes
- 版本演进
prerequisites:
- kubectl-basics
- etcd-basics
created: "2026-05-23"
---

# Kubernetes 版本演进

> 本文档综合了 `domain-19-landscape-references/_archived-release-notes/kubernetes/` 目录下 55 个版本发布说明，提炼 Kubernetes 从早期版本到 v1.36 的核心演进轨迹 ^[inferred]

## 版本概览

| 阶段 | 版本范围 | 特征 |
|---|---|---|
| 孵化期 | v0.x - v0.10 | 原型验证，基础 Pod/Service 模型确立 |
| 生产就绪 | v1.0 - v1.2 | 首次 GA，生产环境可用 |
| 生态扩展 | v1.3 - v1.5 |  Federation、[[StatefulSet|StatefulSet]]、Initializers 引入 |
| 成熟期 | v1.6 - v1.9 | 自动扩缩、RBAC、[[NetworkPolicy|NetworkPolicy]] GA |
| 云原生平台 | v1.10 - v1.13 | CSI、[[kubelet|Kubelet]] 插件化、TTL 控制 |
| 标准化 | v1.14 - v1.17 | kubectl 成熟、kubeadm GA、拓扑感知调度 |
| 现代化 | v1.18 - v1.21 | [[CronJob|CronJob]] GA、Ephemeral Containers、ServiceAccount 自动创建 |
| 增强期 | v1.22 - v1.25 | 结构化日志、Sidecar Containers、Pod 安全准入 |
| 持续演进 | v1.26 - v1.36 | API 持续优化、调度增强、安全加固 |

## 里程碑版本详解

### v1.0 - 首次 GA（2015-07）

Kubernetes 第一个正式生产版本，确立了核心 API 对象模型：
- Pod、Service、ReplicationController 作为核心抽象
- 基础调度与 kubelet 节点管理
- 支持 GCE 和 AWS 云提供商 ^[inferred]

### v1.1 - 性能与服务发现（2015-11）

- 引入 Job 控制器用于批量任务
- 改进水平 Pod 自动扩缩（HPA）
- 支持第三方资源（ThirdPartyResource，后演化为 CRD）^[inferred]

### v1.2 - 多容器与服务编排（2016-03）

- 多容器 Pod 模式成熟
- 联邦集群（Federation）初版
- 支持容器探针（Liveness/Readiness）
- 改进 HPA 支持基于 CPU 的自动扩缩 ^[inferred]

### v1.3 - 企业级功能（2016-07）

- 引入 Annotations 和 Labels 的完善体系
- 集群 Federation v1 引入
- 改进调度器性能 ^[inferred]

### v1.5 - StatefulSet 与 Initializers（2016-12）

- StatefulSet 用于有状态应用管理
- Initializers（后演化为 MutatingWebhook）
- 基于角色的访问控制（RBAC）alpha
- kubeadm 工具初始版本 ^[inferred]

### v1.6 - 自动扩缩与动态供给（2017-03）

- Dynamic Provisioning 支持
- CronJob alpha
- 改进 HPA 支持自定义指标
- 扩展 CRD 功能（后取代 TPR）^[inferred]

### v1.8 - RBGA 与 NetworkPolicy（2017-10）

- RBAC GA
- NetworkPolicy GA
- 核心 Workload API（Deployments/DaemonSets/StatefulSets/ReplicaSets）GA
- CRI（Container Runtime Interface）beta ^[inferred]

### v1.9 - 版本管理与高级调度（2017-12）

- Apps API group GA
- 核心调度器增强：Pod 亲和性/反亲和性
- 引入 kustomize 概念 ^[inferred]

### v1.10 - CSI 与 Windows 支持（2018-03）

- CSI（Container Storage Interface）beta
- Windows 容器支持 beta
- CoreDNS 作为默认 DNS 服务选项
- IPVS 代理模式 beta ^[inferred]

### v1.11 - 调度增强（2018-06）

- CoreDNS GA（替代 kube-dns）
- kubectl 支持自定义列输出
- 改进调度器优先级和抢占 ^[inferred]

### v1.12 - kubeadm 与 CSR（2018-09）

- kubeadm GA
- CSR API GA
- 改进 Windows 支持 ^[inferred]

### v1.14 - 生态整合（2019-03）

- kubectl 成为 GA 命令行工具
- CronJob GA
- 本地持久卷支持
- Kustomize 集成到 kubectl ^[inferred]

### v1.15 - 扩展机制（2019-05）

- CRD 支持 validation/defaulting/ conversion webhooks
- 拓扑管理器（Topology Manager）alpha
- 结构化日志初版 ^[inferred]

### v1.16 - 多租户与安全（2019-09）

- 新增 15 个 GA API
- EndpointSlices alpha
- 改进 RBAC 聚合角色 ^[inferred]

### v1.18 - 临时容器与入口（2020-03）

- Ephemeral Containers alpha
- Ingress GA
- 改进 ServiceAccount 自动创建
- kubectl apply 支持服务器端应用（Server-Side Apply）beta ^[inferred]

### v1.20 - 运行时变更（2020-12）

- Docker 弃用警告（dockershim 将在后续版本移除）
- CSI 迁移进展
- Probes 支持 startup probe GA ^[inferred]

### v1.21 - Pod 安全与拓扑（2021-04）

- Pod 安全策略（PSP）弃用
- EndpointSlices GA
- 改进 CronJob 时区支持
- 移除 dockershim 的第一步 ^[inferred]

### v1.22 - PSP 移除与结构化日志（2021-08）

- PSP（PodSecurityPolicy）移除
- 结构化日志覆盖主要控制平面组件
- 准入控制 v1 GA
- kubectl annotate --overwrite 改进 ^[inferred]

### v1.23 - 自动扩缩增强（2021-12）

- 结构化日志进一步推进
- TTL 用于完成的 Job
- HPA 支持内存指标 ^[inferred]

### v1.25 - Pod 安全准入（2022-08）

- Pod 安全准入（PSA）GA，替代 PSP
- CSI 存储容量跟踪 GA
- 移除 dockershim ^[inferred]

### v1.26 - Sidecar 容器（2022-12）

- Sidecar Containers alpha
- Job 排序和并行控制增强
- 节点特性 API ^[inferred]

### v1.27 - 调度与存储（2023-04）

- 改进调度器插件框架
- 读写一次卷（ReadWriteOncePod）
- 验证规则增强 ^[inferred]

### v1.28 - 资源管理与安全（2023-08）

- 资源健康检查（Resource Health）
- 改进了 Secret 的 RBAC 保护
- 支持多网络策略 ^[inferred]

### v1.29 - 结构化配置（2023-12）

- CEL（Common Expression Language）验证规则 GA
- 改进了组件标准配置
- 持续安全增强 ^[inferred]

### v1.30 - 调度优化（2024-04）

- 改进的调度插件
- 增强了对大规模集群的支持
- API 服务器性能优化 ^[inferred]

### v1.31 - 安全与可观测性（2024-08）

- 改进的审计日志
- API 优先级与公平性增强
- 组件标准持续完善 ^[inferred]

### v1.32 - 存储与网络（2024-12）

- 存储迁移改进
- 网络策略增强
- 组件标准继续推进 ^[inferred]

### v1.33 - v1.36 - 持续现代化（2025-2026）

- 进一步的结构化日志覆盖
- 改进的调度算法
- 安全加固与 API 标准化 ^[inferred]

## 版本兼容性要点

### 弃用与移除时间线

| 特性 | 弃用版本 | 移除版本 |
|---|---|---|
| extensions/v1beta1 (Deployments) | v1.8 | v1.16 |
| PodSecurityPolicy | v1.21 | v1.25 |
| dockershim | v1.20 | v1.24 |
| Beta APIs (批量) | 各版本 | 通常 3 个版本后 |
| Legacy Scheduler API | v1.23 | v1.26+ |

### 升级建议

1. **跳版本升级**：Kubernetes 支持相邻小版本升级（如 v1.28 -> v1.29），不支持跨多版本直接升级
2. **API 迁移**：升级前使用 `kubectl api-resources` 检查已弃用的 API
3. **组件兼容**：确保 etcd、coredns、kube-proxy 等核心组件版本与 [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]] 版本兼容

## 来源文档

本文档基于以下源文件综合编写：

- domain-19-landscape-references/_archived-release-notes/kubernetes/CHANGELOG-1.2.md ~ CHANGELOG-1.36.md（35 个 CHANGELOG 文件）
- domain-19-landscape-references/_archived-release-notes/kubernetes/RELEASE-NOTES-0.4.md ~ RELEASE-NOTES-1.1.md（19 个 RELEASE-NOTES 文件）

## Related

- [[entities/statefulset.md|statefulset]] — StatefulSet
- [[entities/kubelet.md|kubelet]] — kubelet
- [[coredns]] — CoreDNS
- [[etcd]] — etcd
- [[kubernetes]] — [[kubernetes|Kubernetes (CNCF Graduated)]]

- [[CHANGELOG|CHANGELOG]]
- [[entities/networkpolicy.md|networkpolicy]]