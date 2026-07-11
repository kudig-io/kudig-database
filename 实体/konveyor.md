---
title: Konveyor (entities)
description: '## 概述'
summary: 'Konveyor 是一个应用现代化平台，帮助组织将传统应用（如 Java EE、Spring）迁移和重构到 Kubernetes 平台。它提供应用清单管理、依赖分析、迁移评估、自动化代码重构等能力。Konveyor 通过 AI 辅助分析识别迁移障碍，生成迁移路径建议，并提供 IDE 插件帮助开发者自动化完成代码变更。'
category: entities
tags:
- k8s
- cncf
- ci-cd
- konveyor
- crd
- operator
- kserve
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Konveyor 是什么
- 如何 Konveyor
trigger_keywords:
- Konveyor
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Konveyor

> **CNCF 状态**: Sandbox | **类别**: CI/CD | **主要语言**: Go, TypeScript

## 概述

Konveyor 是一个 CNCF 沙箱项目，由 Red Hat 主导，是一个应用现代化和迁移工具集。它帮助组织将传统应用（Java EE、虚拟机应用）迁移到 Kubernetes 和云原生架构。Konveyor 包含多个工具：Tackle（迁移项目管理）、Windup（代码分析）、Move2Kube（部署配置迁移）、Crane（K8s 集群间迁移）等。项目通过自动化分析和迁移建议，大幅降低应用现代化的工作量。

## Key Features（核心能力）

- **应用评估**：Tackle 提供应用现代化就绪度评估和迁移计划管理
- **代码分析**：Windup 分析应用源码，识别迁移到容器/K8s 的障碍和风险
- **Move2Kube**：自动将应用部署配置（如 docker-compose）转换为 K8s YAML
- **Crane**：K8s 集群间的资源和数据迁移工具
- **迁移路径建议**：基于分析结果推荐最佳迁移路径
- **多语言支持**：支持 Java、Python、Go、Node.js 等语言应用分析

## 架构与工作原理

Konveyor 是一个工具集而非单一系统：Tackle 提供项目管理 Web UI 和 API；Windup 通过静态代码分析识别迁移风险和依赖；Move2Kube 通过解析现有部署配置（如 docker-compose、Cloud Foundry manifest）生成 K8s 部署清单；Crane 通过 K8s API 迁移命名空间级别的资源。各工具可独立使用或通过 Tackle 统一管理。

## K8s 集成

Konveyor 本身可在 Kubernetes 上部署，通过 Operator 管理各组件。迁移工具通过 K8s API 连接到目标集群，执行资源迁移和配置转换。Move2Kube 生成的 K8s YAML 可直接 kubectl apply。Crane 支持跨集群的命名空间迁移，包括 PVC 数据迁移。

## 生产用例

- **应用现代化**：将传统 Java EE 应用迁移到 K8s 容器化架构
- **VM 到容器迁移**：将虚拟机应用容器化到 K8s
- **集群迁移**：跨 K8s 集群的资源和数据迁移
- **迁移评估**：评估应用组合的云原生就绪度

## 安装与快速开始

```bash
# Move2Kube CLI
pip3 install move2kube
# Tackle Operator
kubectl apply -f https://raw.githubusercontent.com/konveyor/tackle-operator/main/install/konveyor-operator.yaml
```

## 对比替代方案

相比手动迁移分析，Konveyor 提供自动化的代码分析和配置转换。相比 AWS Migration Hub（云厂商绑定），Konveyor 是开源且厂商中立的。

## Related

- [[network-service-mesh]] — [[实体/network-service-mesh.md|Network Service Mesh (NSM)]]]Service Mesh）|Service Mesh]] (NSM)
- [[kserve]] — KServe
- [[meshery]] — Meshery
- [[knative]] — Knative
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- konveyor
- [[实体/shipwright.md|Shipwright]]
- [[实体/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference


<!-- risk-assessed -->
