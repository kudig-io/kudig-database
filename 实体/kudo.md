---
title: KUDO
description: '## 概述'
summary: 'KUDO 是一个构建 Kubernetes Operator 的声明式工具包，允许开发者仅使用 YAML 定义复杂的有状态应用生命周期管理逻辑，无需编写 Go 代码。'
category: entities
tags:
- k8s
- cncf
- orchestration
- kudo
- kubelet
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KUDO 是什么
- 如何 KUDO
trigger_keywords:
- KUDO
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# KUDO

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

KUDO（Kubernetes Universal Declarative Operator）是一个构建 Kubernetes Operator 的声明式工具包，由 D2iQ（原 Mesosphere）开发，2019 年加入 CNCF 沙箱。它允许开发者仅使用 YAML 定义复杂的有状态应用生命周期管理逻辑，无需编写 Go 代码。KUDO 将 Operator 的常见模式（安装、升级、备份、恢复、扩缩容等）抽象为声明式 Plan，每个 Plan 由有序的 Phase 和 Step 组成，支持参数化配置和模板渲染。这使得运维团队可以为 Kafka、Cassandra、Zookeeper 等复杂中间件快速构建生产级 Operator，而不需要掌握 Go 语言和 controller-runtime 框架。

## 核心能力

- **声明式 Operator**: 纯 YAML 定义 Operator 逻辑，无需编写 Go 代码
- **Plan/Phase/Step**: 结构化的生命周期操作模型，支持串行/并行执行
- **参数化模板**: 使用 Go template 参数化部署配置，支持参数覆盖和验证
- **内置任务**: 提供 Deploy、Pipe、Kubectl 等内置任务类型
- **状态管理**: 自动跟踪 Plan 执行进度和状态，支持恢复
- **Operator 仓库**: KUDO Manager 提供丰富的预制 Operator 目录

## 架构

KUDO 采用通用控制器 + 声明式 Operator 定义模式：

- **KUDO Manager**: 集群中运行的控制器，监听 Operator 和 Instance CRD
- **Operator CRD**: 定义 Operator 元数据（版本、参数 schema、Plan 定义）
- **Instance CRD**: 部署的实例，引用 Operator 并提供参数值
- **Plan**: 生命周期操作（deploy/upgrade/backup/restore），由 Phase 和 Step 组成
- **Phase**: Plan 中的有序阶段，Phase 之间串行执行
- **Step**: Phase 中的执行单元，Step 之间可并行或串行
- **Task**: 实际操作（创建资源、执行脚本等），由 Step 引用

执行模型：`Instance → Operator (Plan/Phase/Step) → Task → K8s 资源`

## K8s 集成

KUDO Manager 以 Deployment 运行在 Kubernetes 集群中，通过监听 Operator 和 Instance CRD 执行生命周期操作。每个 Operator 定义了 deploy、upgrade、backup、restore 等 Plan。当 Instance CRD 被创建时，KUDO Manager 触发 deploy Plan，按 Phase → Step 顺序执行任务（创建 Deployment、ConfigMap 等）。Step 中的资源模板渲染后通过 Kubernetes API 创建。KUDO 与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 CRD 机制完全兼容，Operator 定义以 YAML 存储在 Operator 仓库中。

## 生产场景

1. **中间件 Operator 开发**: 为 Kafka、Cassandra、Zookeeper 等快速构建生产级 Operator
2. **标准化运维流程**: 将备份、恢复、扩容等运维操作标准化为可重复执行的 Plan
3. **多版本管理**: 通过 Operator 版本管理实现平滑升级和回滚
4. **GitOps 集成**: 将 Operator 和 Instance YAML 存储在 Git 中，通过 GitOps 工具管理

## 安装

```bash
# 安装 KUDO CLI
brew install kudo-cli
# 或
curl -s https://kudo.dev/install.sh | bash

# 安装 KUDO Manager
kubectl kudo init

# 安装预制 Operator（以 Kafka 为例）
kubectl kudo install kafka --instance=my-kafka

# 查看安装进度
kubectl kudo plan status --instance=my-kafka

# 执行备份 Plan
kubectl kudo plan trigger backup --instance=my-kafka
```

## 对比

| 特性 | KUDO | Kubebuilder | Operator SDK | OLM |
|------|------|-------------|--------------|-----|
| 编程方式 | YAML 声明式 | Go 代码 | Go/Ansible/Helm | 元数据管理 |
| 学习曲线 | 低 | 高 | 中 | 低 |
| 灵活性 | ⚠️ 有限 | ✅ 最高 | ✅ 高 | ⚠️ 元数据层 |
| 适合场景 | 快速构建 | 复杂逻辑 | 多语言 | Operator 生命周期 |

## 架构定位

在 CNCF 生态中，KUDO 属于 **Orchestration** 类别，为云原生应用提供声明式 Operator 开发能力。

## 参考链接

- [[crossplane]]
- [[deployment]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[概念/controller-pattern.md|controller-pattern]]

## Related

- [[openfeature]] — OpenFeature
- tools]] — Podman Desktop
- [[k3s]] — k3s 轻量级 Kubernetes
- [[virtual-kubelet]] — Virtual Kubelet
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kudo
- [[实体/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
