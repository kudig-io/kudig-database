---
title: CNCF 编排与应用管理项目全景
description: '# CNCF 编排与应用管理项目全景'
category: entities
tags:
- k8s
- cncf
- orchestration
- multi-cluster
- operator
- package-management
- kubelet
- prometheus
- helm
- kafka
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CNCF 编排与应用管理项目全景 是什么
- 如何 CNCF 编排与应用管理项目全景
trigger_keywords:
- CNCF
- 编排与应用管理项目全景
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- iac-basics
- kafka-basics
- gpu-scheduling-basics
created: "2026-05-23"
---

# CNCF 编排与应用管理项目全景

> 聚合页面 | 涵盖 27 个 CNCF 编排和管理项目

## 概述

CNCF 编排生态覆盖 **包管理**、**多集群编排**、**Operator 框架**、**应用定义**、**弹性伸缩** 五大领域。

---

## 包管理与配置

### [[helm]] — 毕业项目

Helm 是 K8s 应用包管理器（Chart 格式）。

- Chart 模板化 K8s 资源
- 版本管理和回滚
- 依赖管理
- K8s 生态事实标准

### [[kpt]] — 沙箱项目

kpt 是 Google 的 K8s 资源配置管理工具。

### [[cdk8s]] — 沙箱项目

cdk8s 用编程语言（TypeScript/Python/Java）定义 K8s 资源。

### [[kcl]] — 沙箱项目

KCL 是面向云原生配置的策略语言。

### [[opentofu]] — 沙箱项目

OpenTofu 是 Terraform 的开源分叉。

---

## 多集群编排

### [[karmada]] — 孵化项目

Karmada 是 K8s 多集群管理平台。

- 跨集群工作负载调度
- 统一 API 和策略
- 跨云、跨区域管理

### [[open-cluster-management]] — 沙箱项目

OCM（[[entities/open-cluster-management.md|Open Cluster Management]]）提供多集群生命周期管理。

### [[clusternet]] — 沙箱项目

ClusterNet 提供多集群网络和管理。

### [[Clusterpedia]] — 沙箱项目

clusterpedia 提供跨集群资源查询。

### [[kubefleet]] — 沙箱项目

KubeFleet 管理 K8s 集群舰队。

### [[kubestellar]] — 沙箱项目

Kubestellar 提供多集群工作负载编排。

### [[kcp]] — 沙箱项目

KCP 提供 K8s API 的多租户控制平面。

---

## Operator 框架

### [[operator-framework]] — 孵化项目

Operator Framework 构建 K8s Operator 的工具链。

- **Operator SDK**: 快速生成 Operator 代码
- **Operator Lifecycle Manager (OLM)**: 管理 Operator 生命周期
- **Operator Hub**: Operator 发现和分发

### [[kudo]] — 沙箱项目

KUDO 是声明式的 Operator 框架（无需编写 Go 代码）。

---

## 应用定义与部署

### [[crossplane]] — 毕业项目

Crossplane 将云基础设施资源暴露为 K8s CRD。

- 声明式管理云资源（数据库、VPC、IAM）
- Composition 定义自定义抽象
- 与 GitOps 工具集成

### [[dapr]] — 毕业项目

Dapr 是分布式应用运行时。

- 服务间通信（发布/订阅、服务调用）
- 状态管理、密钥管理
- 语言无关的 sidecar 架构

### [[kubevela]] — 孵化项目

KubeVela 基于 OAM 的应用交付平台。

### [[capsule]] — 沙箱项目

Capsule 提供 K8s 多租户管理。

### [[kusionstack]] — 沙箱项目

KusionStack 是云原生可编程基础设施管理平台。

### [[virtual-kubelet]] — 沙箱项目

Virtual Kubelet 将外部计算资源虚拟为 K8s 节点。

### [[armada]] — 沙箱项目

Armada 是多集群批处理作业调度器。

---

## 弹性伸缩

### [[keda]] — 毕业项目

KEDA 是 K8s 事件驱动自动伸缩器。

- 70+ 事件源（Kafka、RabbitMQ、Prometheus 等）
- 缩容到零（scale to zero）
- 与 HPA 兼容

### [[knative]] — 毕业项目

Knative 是 K8s 上的 Serverless 平台。

- **Serving**: 自动伸缩、流量分割
- **Eventing**: 事件驱动架构
- 缩容到零

### [[volcano]] — 孵化项目

Volcano 是 K8s 批处理和 AI/ML 工作负载调度器。

- Gang scheduling、队列管理
- GPU 共享和拓扑感知

---

## 基础设施即代码

### [[opentofu]] — 沙箱项目

OpenTofu 是 Terraform 的开源替代。

### [[kubeedge]] — 毕业项目

KubeEdge 将 K8s 扩展到边缘计算。

- 边缘节点离线自治
- 边缘设备管理
- 云端协同

---

## 架构选型建议

| 场景 | 推荐方案 |
|---|---|
| 应用包管理 | Helm |
| 云资源声明式管理 | Crossplane |
| 多集群管理 | Karmada 或 OCM |
| Operator 开发 | Operator Framework |
| 事件驱动伸缩 | KEDA |
| Serverless | Knative |
| 批处理/AI 调度 | Volcano |
| 边缘计算 | KubeEdge |

---

## 相关页面

- [[entities/cncf-cicd.md|cncf-cicd]] — CI/CD 与发布管理
- [[entities/cncf-networking.md|cncf-networking]] — 网络与服务网格
- [[entities/cncf-security.md|cncf-security]] — 安全与合规

## Related

- [[headlamp]] — Headlamp
- [[prometheus]] — Prometheus
- [[virtual-kubelet]] — Virtual Kubelet
- [[open-cluster-management]] — Open Cluster Management (OCM)
- [[operator-framework]] — Operator Framework

- [[entities/cohdi.md|Cohdi]]
- [[entities/kubefleet.md|KubeFleet]]
- [[entities/clusternet.md|Clusternet]]
- [[entities/kured.md|Kured (KUbernetes REboot Daemon)]]
- [[entities/kubevela.md|KubeVela]]
- [[entities/kubestellar.md|KubeStellar]]
- [[entities/microcks.md|Microcks]]
- [[entities/kudo.md|KUDO]]