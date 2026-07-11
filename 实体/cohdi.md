---
title: Cohdi
description: '## 概述'
summary: 'CoHDI（Composable Hyperconverged Disaggregated Infrastructure）是一个 Kubernetes Operator，用于在分解式基础设施中动态组合和管理硬件资源。'
category: entities
tags:
- k8s
- cncf
- orchestration
- cohdi
- crd
- operator
- gpu
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cohdi 是什么
- 如何 Cohdi
trigger_keywords:
- Cohdi
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Cohdi

> **CNCF 状态**: Sandbox | **类别**: Orchestration | **主要语言**: Go

## 概述

Cohdi 是一个 CNCF 沙箱项目，旨在提供 Kubernetes 上的轻量级工作负载编排和部署自动化能力。它专注于简化应用从开发到生产的多环境部署流程，通过声明式配置管理多集群应用分发。Cohdi 特别关注边缘计算和混合云场景下的工作负载编排，提供低资源占用的 Agent 和灵活的部署策略。

## Key Features（核心能力）

- **多集群部署**：将应用工作负载分发到多个 K8s 集群
- **环境差异管理**：通过 Overlay 和 Patch 管理不同环境的配置差异
- **渐进式发布**：支持跨集群的蓝绿部署和金丝雀发布
- **轻量级 Agent**：边缘节点上的低资源占用代理
- **GitOps 集成**：基于 Git 仓库的应用配置管理
- **策略引擎**：部署位置和时机的策略控制

## 架构与工作原理

Cohdi 采用 Hub-Spoke 架构：Hub 组件运行在中心集群，管理应用部署配置和分发策略；Spoke Agent 运行在目标集群（包括边缘节点），接收部署指令并协调本地工作负载。部署配置通过声明式 YAML 定义，支持环境 Overlay、健康检查和回滚策略。

## K8s 集成

Cohdi 通过 CRD 与 Kubernetes 集成：DeploymentPolicy CRD 定义应用的部署目标和策略；ClusterSet CRD 定义目标集群集合。Hub Controller 管理这些 CRD 并分发工作负载清单到各目标集群。Spoke Agent 在目标集群中以 Deployment 部署，监听 Hub 的部署指令。

## 生产用例

- **边缘应用分发**：将应用部署到大量边缘节点
- **多环境管理**：统一管理 dev/staging/prod 的应用部署
- **混合云部署**：跨本地数据中心和公有云的应用分发
- **渐进式发布**：跨集群的金丝雀发布

## 安装与快速开始

```bash
kubectl apply -f https://github.com/cohdi/cohdi/releases/latest/download/cohdi.yaml
```

## 对比替代方案

相比 KubeFed v2（已归档），Cohdi 更轻量且专注于应用分发。相比 ArgoCD（单集群 GitOps），Cohdi 支持多集群应用编排。

## Related

- [[kube-burner]] — Kube-burner
- [[eraser]] — Eraser
- [[kubewarden]] — Kubewarden
- [[devfile]] — Devfile
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- cohdi
- index/etcd-index|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
